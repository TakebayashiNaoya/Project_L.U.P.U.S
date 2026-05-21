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

		// 遷移時点の警告リストを即時取得して出力し、キャッシュに格納する。
		// こうすることで OnUpdate() のデバウンス判定が「遷移後の差分」のみを対象にできる。
		const std::vector<std::string> currentWarnings = m_context.GetAllWarnings();
		m_lastPrompt = BuildPrompt(currentWarnings);

		std::cout << "[StateTaskFocus] LLM リクエスト送信中..." << std::endl;

		if (!currentWarnings.empty())
		{
			const std::string prompt = BuildPrompt(currentWarnings);
			std::cout << "[StateTaskFocus] LLM プロンプト組み立て完了" << std::endl;
			std::cout << prompt << std::endl;

			// 実際に API を呼び出して LUPUS の言葉を動的に生成！
			if (m_llmClient)
			{
				std::cout << "[StateTaskFocus] LLM リクエストを非同期で送信中..." << std::endl;

				// 安全にバックグラウンドスレッドへ渡すため、必要な値だけをコピー(キャプチャ)する
				std::string assistantName = m_assistantName;
				ILLMClient* client = m_llmClient;

				// 非同期スレッドを起動してHTTP通信を完全に分離する (detach で投げっぱなしにする)
				std::thread([client, prompt, assistantName]() {
					const std::string aiResponse = client->GenerateResponse(prompt);
					std::cout << "\n--- [" << assistantName << " の回答] ---" << std::endl;
					std::cout << aiResponse << std::endl;
					std::cout << "---------------------------------\n" << std::endl;
					}).detach();
			}
		}
	}


	void StateTaskFocus::OnUpdate()
	{
		// 全モニターの警告を統合取得する
		const std::vector<std::string> currentWarnings = m_context.GetAllWarnings();

		// 現在の状態でプロンプトを組み立てる
		const std::string newPrompt = BuildPrompt(currentWarnings);

		// 前回とプロンプトの内容が完全に同じ場合（タスクも警告も変化なし）はスキップする(デバウンス)
		if (newPrompt == m_lastPrompt)
		{
			return;
		}

		// 差分があった場合はキャッシュを更新する
		m_lastPrompt = newPrompt;

		std::cout << "[StateTaskFocus] LLM プロンプト再組み立て完了(差分検出)" << std::endl;
		std::cout << newPrompt << std::endl;

		NotifyUser(m_instantWarningMessage);

		if (m_llmClient)
		{
			std::cout << "[StateTaskFocus] 状況の変化を検出。再リクエストを非同期で送信中..." << std::endl;

			std::string assistantName = m_assistantName;
			ILLMClient* client = m_llmClient;
			std::string promptForThread = newPrompt;

			std::thread([client, promptForThread, assistantName]() {
				const std::string aiResponse = client->GenerateResponse(promptForThread);
				std::cout << "\n--- [" << assistantName << " の回答(更新)] ---" << std::endl;
				std::cout << aiResponse << std::endl;
				std::cout << "-------------------------------------\n" << std::endl;
				}).detach();
		}
	}


	void StateTaskFocus::OnExit()
	{
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


	std::string StateTaskFocus::BuildPrompt(const std::vector<std::string>& warnings) const
	{
		// ---------------------------------------------------------------
		// セクション1: システムプロンプト(アシスタントのペルソナ定義)
		// ---------------------------------------------------------------
		std::string prompt = std::string("=== SYSTEM PROMPT ===\n")
			+ m_systemPrompt
			+ "\n\n";

		// ---------------------------------------------------------------
		// セクション2: 規約違反・ドキュメント変更の警告リスト
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
		// セクション3: Notion 未完了タスクリスト
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

		// ---------------------------------------------------------------
		// フェーズ3: ここで API 呼び出しを行い m_deepReviewResult に格納する
		// ---------------------------------------------------------------

		return prompt;
	}


} // namespace app