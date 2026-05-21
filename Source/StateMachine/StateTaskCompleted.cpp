/**
 * @file StateTaskCompleted.cpp
 * @brief タスク完了状態クラス実装
 */
#include "stdafx.h"
#include "StateTaskCompleted.h"


namespace app
{
	StateTaskCompleted::StateTaskCompleted(
		const std::string& assistantName,
		const std::string& completionMessage
	)
		: m_assistantName(assistantName)
		, m_completionMessage(completionMessage)
	{}


	void StateTaskCompleted::OnEnter()
	{
		// 遷移のたびに初回出力フラグをリセットする
		m_isFirstUpdate = true;

		std::cout << "[StateTaskCompleted] OnEnter" << std::endl;
	}


	void StateTaskCompleted::OnUpdate()
	{
		// 完了状態では初回のみ称賛メッセージを出力し、以降は何もしない
		if (!m_isFirstUpdate)
		{
			return;
		}

		m_isFirstUpdate = false;
		NotifyUser(m_completionMessage);
	}


	void StateTaskCompleted::OnExit()
	{
		std::cout << "[StateTaskCompleted] OnExit" << std::endl;
	}


	const char* StateTaskCompleted::GetName() const
	{
		return "TaskCompleted";
	}


	void StateTaskCompleted::NotifyUser(const std::string& message) const
	{
		// フェーズ2: std::cout へのログ出力のみ
		// フェーズ3: ここに TTS / AudioPipeline への連携を追加する
		std::cout << "[" << m_assistantName << "] " << message << std::endl;
	}


} // namespace app