/**
 * @file StateTaskFocus.cpp
 * @brief 集中状態クラス実装
 */
#include "stdafx.h"
#include "StateTaskFocus.h"
#include "Source/Audio/IAudioPipeline.h"
#include "Source/LLM/ILLMClient.h"
#include "Source/Util/TextSplitter.h"


namespace app
{
	StateTaskFocus::StateTaskFocus(
		SystemContext& context,
		const std::string& systemPrompt,
		const std::string& assistantName,
		const std::string& instantWarningMessage,
		ILLMClient* llmClient,
		IAudioPipeline* audioPipeline
	)
		: m_context(context)
		, m_systemPrompt(systemPrompt)
		, m_assistantName(assistantName)
		, m_instantWarningMessage(instantWarningMessage)
		, m_llmClient(llmClient)
		, m_audioPipeline(audioPipeline)
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
		// 完了済みの LLM レスポンスがあれば先に出力・再生する
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
		// これにより ILLMClient / IAudioPipeline の use-after-free を防止する。
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
		std::cout << "[" << m_assistantName << "] " << message << std::endl;

		if (m_audioPipeline)
		{
			m_audioPipeline->Speak(message);
		}
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

		// AI レスポンスをチャンクに分割してから順次キューに積む。
		// 分割された短いチャンクを1件ずつ Speak() に渡すことで、
		// TTS API の長文エラーを回避し、文頭から順次再生を開始できる。
		// m_audioPipeline は OnExit() の wait() によって
		// 非同期処理完了前に破棄されないことが保証される。
		// "[GeminiClient]" で始まる文字列は LLM クライアントのエラーメッセージのため、
		// ログ出力のみ行い音声再生には渡さない。
		constexpr std::string_view kLlmErrorPrefix = "[GeminiClient]";
		const bool isLlmError = aiResponse.rfind(kLlmErrorPrefix, 0) == 0;

		if (m_audioPipeline && !isLlmError)
		{
			const std::vector<std::string> chunks = TextSplitter::SplitIntoChunks(aiResponse);
			for (const auto& chunk : chunks)
			{
				m_audioPipeline->Speak(chunk);
			}
		}
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
		// IAudioPipeline* の寿命は LupusApp が保証し、同様に OnExit() の wait() で担保する。
		// どちらのポインタも StateTaskFocus の生存期間中は不変のため、ラムダキャプチャは安全。
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
		std::string prompt = m_systemPrompt + "\n\n";

		// ---------------------------------------------------------------
		// セクション2: 警告リスト
		// ---------------------------------------------------------------
		if (!warnings.empty())
		{
			prompt += "=== CURRENT WARNINGS ===\n";
			for (const auto& warning : warnings)
			{
				prompt += "- " + warning + "\n";
			}
			prompt += "\n";
		}

		return prompt;
	}


} // namespace app