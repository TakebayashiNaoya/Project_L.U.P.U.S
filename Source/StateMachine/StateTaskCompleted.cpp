/**
 * @file StateTaskCompleted.cpp
 * @brief タスク完了状態クラス実装
 */
#include "stdafx.h"
#include "StateTaskCompleted.h"
#include "Source/Audio/IAudioPipeline.h"


namespace app
{
	StateTaskCompleted::StateTaskCompleted(
		const std::string& assistantName,
		const std::string& completionMessage,
		IAudioPipeline* audioPipeline
	)
		: m_assistantName(assistantName)
		, m_completionMessage(completionMessage)
		, m_audioPipeline(audioPipeline)
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
		std::cout << "[" << m_assistantName << "] " << message << std::endl;

		if (m_audioPipeline)
		{
			m_audioPipeline->Speak(message);
		}
	}


} // namespace app