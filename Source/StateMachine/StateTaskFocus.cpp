/**
 * @file StateTaskFocus.cpp
 * @brief 集中状態クラス実装
 */
#include "stdafx.h"
#include "StateTaskFocus.h"


namespace app
{
	StateTaskFocus::StateTaskFocus(
		SystemContext& context,
		const std::string& systemPrompt,
		const std::string& assistantName,
		const std::string& instantWarningMessage
	)
		: m_context(context)
		, m_systemPrompt(systemPrompt)
		, m_assistantName(assistantName)
		, m_instantWarningMessage(instantWarningMessage)
	{}


	void StateTaskFocus::OnEnter()
	{
		std::cout << "[StateTaskFocus] OnEnter" << std::endl;
		NotifyUser(m_instantWarningMessage);

		// 遷移時点の警告リストを即時取得して出力し、キャッシュに格納する。
		// こうすることで OnUpdate() のデバウンス判定が「遷移後の差分」のみを対象にできる。
		const std::vector<std::string> currentWarnings = m_context.GetAllWarnings();
		m_lastPrompt = BuildPrompt(currentWarnings);

		if (!currentWarnings.empty())
		{
			const std::string prompt = BuildPrompt(currentWarnings);
			std::cout << "[StateTaskFocus] LLM プロンプト組み立て完了" << std::endl;
			std::cout << prompt << std::endl;
		}
	}


	void StateTaskFocus::OnUpdate()
	{
		// 全モニターの警告を統合取得する
		const std::vector<std::string> currentWarnings = m_context.GetAllWarnings();
		const std::string newPrompt = BuildPrompt(currentWarnings);

		// プロンプトの内容（警告やタスク）に変化がない場合は出力をスキップする(完全なデバウンス)
		if (newPrompt == m_lastPrompt)
		{
			return;
		}

		m_lastPrompt = newPrompt;

		// 警告内容が変化したため LLM プロンプトを再組み立てして出力する
		std::cout << "[StateTaskFocus] LLM プロンプト再組み立て完了(差分検出)" << std::endl;
		std::cout << m_lastPrompt << std::endl;

		// ユーザーへの即時通知
		NotifyUser(m_instantWarningMessage);
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