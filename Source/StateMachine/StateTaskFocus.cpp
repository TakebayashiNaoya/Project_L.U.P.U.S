/**
 * @file StateTaskFocus.cpp
 * @brief 集中状態クラス実装
 */
#include "stdafx.h"
#include "StateTaskFocus.h"
#include "Source/LLM/ILLMClient.h"


namespace app
{
	StateTaskFocus::StateTaskFocus(
		SystemContext& context,
		const std::string& systemPrompt,
		const std::string& assistantName,
		const std::string& instantWarningMessage,
		ILLMClient* llmClient
	)
		: m_context(context)
		, m_systemPrompt(systemPrompt)
		, m_assistantName(assistantName)
		, m_instantWarningMessage(instantWarningMessage)
		, m_llmClient(llmClient)
	{}


	void StateTaskFocus::OnEnter()
	{
		std::cout << "[StateTaskFocus] OnEnter" << std::endl;
		NotifyUser(m_instantWarningMessage);

		// 遷移時点の警告リストでプロンプトを組み立ててキャッシュに格納する。
		// こうすることで OnUpdate() のデバウンス判定が「遷移後の差分」のみを対象にできる。
		const std::vector<std::string> currentWarnings = m_context.GetAllWarnings();
		m_lastPrompt = BuildPrompt(currentWarnings);

		if (!currentWarnings.empty())
		{
			std::cout << "[StateTaskFocus] LLM プロンプト組み立て完了" << std::endl;
			std::cout << m_lastPrompt << std::endl;
			LaunchLLMRequest(m_lastPrompt);
		}
	}


	void StateTaskFocus::OnUpdate()
	{
		// 完了済みの LLM レスポンスがあれば先に出力する
		FlushPendingResponse();

		// 全モニターの警告を統合取得する
		const std::vector<std::string> currentWarnings = m_context.GetAllWarnings();

		// 現在の状態でプロンプトを組み立てる
		const std::string newPrompt = BuildPrompt(currentWarnings);

		// 前回とプロンプトの内容が完全に同じ場合(タスクも警告も変化なし)はスキップする(デバウンス)
		if (newPrompt == m_lastPrompt)
		{
			return;
		}

		// 差分があった場合はキャッシュを更新する
		m_lastPrompt = newPrompt;

		std::cout << "[StateTaskFocus] LLM プロンプト再組み立て完了(差分検出)" << std::endl;
		std::cout << newPrompt << std::endl;

		NotifyUser(m_instantWarningMessage);
		LaunchLLMRequest(newPrompt);
	}


	void StateTaskFocus::OnExit()
	{
		// 未完了の非同期リクエストが残っている場合は完了を待機してから抜ける。
		// これにより ILLMClient の use-after-free を防止する。
		if (m_pendingResponse.valid())
		{
			std::cout << "[StateTaskFocus] 非同期リクエストの完了を待機中..." << std::endl;
			m_pendingResponse.wait();
		}

		m_lastPrompt.clear();
		std::cout << "[StateTaskFocus] OnExit" << std::endl;
	}


	const char* StateTaskFocus::GetName() const
	{
		return "TaskFocus";
	}


	void StateTaskFocus::NotifyUser(const std::string& message) const
	{
		// フェーズ2: std::cout へのログ出力のみ
		// フェーズ3: ここに TTS / AudioPipeline への連携を追加する
		std::cout << "[" << m_assistantName << "] " << message << std::endl;
	}


	void StateTaskFocus::FlushPendingResponse()
	{
		if (!m_pendingResponse.valid())
		{
			return;
		}

		// ノンブロッキングで ready チェックする(wait_for(0) はブロッキングしない)
		if (m_pendingResponse.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		{
			return;
		}

		// 結果を取り出してログ出力する
		const std::string aiResponse = m_pendingResponse.get();
		std::cout << "\n--- [" << m_assistantName << " の回答] ---" << std::endl;
		std::cout << aiResponse << std::endl;
		std::cout << "---------------------------------\n" << std::endl;
	}


	void StateTaskFocus::LaunchLLMRequest(const std::string& prompt)
	{
		if (!m_llmClient)
		{
			return;
		}

		// 前のリクエストがまだ処理中の場合は新規起動をスキップする
		if (m_pendingResponse.valid() &&
			m_pendingResponse.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		{
			std::cout << "[StateTaskFocus] 前のリクエストが処理中のため新規リクエストをスキップします" << std::endl;
			return;
		}

		std::cout << "[StateTaskFocus] LLM リクエストを非同期で送信中..." << std::endl;

		// std::async で非同期起動する。Future で寿命を管理するため detach 不要。
		// ILLMClient* の寿命は StateMachine が保証し、OnExit() の wait() で完了を担保する。
		ILLMClient* client = m_llmClient;
		m_pendingResponse = std::async(std::launch::async, [client, prompt]() -> std::string {
			return client->GenerateResponse(prompt);
			});
	}


	std::string StateTaskFocus::BuildPrompt(const std::vector<std::string>& warnings) const
	{
		// ---------------------------------------------------------------
		// セクション1: システムプロンプト(アシスタントのペルソナ定義)
		// ---------------------------------------------------------------
		std::string prompt = std::string("=== SYSTEM PROMPT ===\n")
			+ m_systemPrompt
			+ "\n\n";

		// ---------------------------------------------------------------
		// セクション2: 現在の環境情報
		// m_isAtHome の値に基づき、LLM が口調・距離感を切り替えるための
		// コンテキスト情報を注入する。このフラグが profile.md の振る舞い定義と連動する。
		// ---------------------------------------------------------------
		prompt += std::string("=== CURRENT ENVIRONMENT ===\n");

		if (m_context.m_isAtHome)
		{
			prompt += std::string("場所: 自宅 (At Home) - プライベート空間\n");
			prompt += std::string("=> profile.md の「■ 場所が自宅の場合」の指示に従って応答してください。\n");
		}
		else
		{
			prompt += std::string("場所: 外出先 (Outside) - 他者の目あり\n");
			prompt += std::string("=> profile.md の「■ 場所が外出先の場合」の指示に従って応答してください。\n");
		}

		prompt += "\n";

		// ---------------------------------------------------------------
		// セクション3: 規約違反・ドキュメント変更の警告リスト
		// ---------------------------------------------------------------
		prompt += std::string("=== LEVEL 1 WARNINGS (")
			+ std::to_string(warnings.size())
			+ " 件) ===\n";

		for (const auto& warning : warnings)
		{
			prompt += std::string("- ") + warning + "\n";
		}

		prompt += "\n";

		// ---------------------------------------------------------------
		// セクション4: Notion 未完了タスクリスト
		// ---------------------------------------------------------------
		std::vector<NotionTask> pendingTasks;
		{
			std::lock_guard<std::mutex> lock(m_context.m_taskMutex);
			pendingTasks = m_context.m_pendingTasks;
		}

		prompt += std::string("=== PENDING TASKS (")
			+ std::to_string(pendingTasks.size())
			+ " 件) ===\n";

		for (const auto& task : pendingTasks)
		{
			prompt += std::string("- [") + task.m_status + "] " + task.m_title;

			if (!task.m_dueDate.empty())
			{
				prompt += std::string(" (期限: ") + task.m_dueDate + ")";
			}

			prompt += "\n";
		}

		return prompt;
	}


} // namespace app