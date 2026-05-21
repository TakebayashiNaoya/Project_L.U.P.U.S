/**
 * @file StateStandby.cpp
 * @brief 待機状態クラス実装
 */
#include "stdafx.h"
#include "StateStandby.h"
#include "Source/Audio/IAudioPipeline.h"


namespace app
{
	StateStandby::StateStandby(
		const std::string& assistantName,
		const std::string& standbyMessage,
		IAudioPipeline* audioPipeline
	)
		: m_assistantName(assistantName)
		, m_standbyMessage(standbyMessage)
		, m_audioPipeline(audioPipeline)
	{}


	void StateStandby::OnEnter()
	{
		// 遷移のたびに初回出力フラグをリセットする
		m_isFirstUpdate = true;

		std::cout << "[StateStandby] OnEnter" << std::endl;
	}


	void StateStandby::OnUpdate()
	{
		// 待機状態では初回のみメッセージを出力し、以降は何もしない
		if (!m_isFirstUpdate)
		{
			return;
		}

		m_isFirstUpdate = false;
		NotifyUser(m_standbyMessage);
	}


	void StateStandby::OnExit()
	{
		std::cout << "[StateStandby] OnExit" << std::endl;
	}


	const char* StateStandby::GetName() const
	{
		return "Standby";
	}


	void StateStandby::NotifyUser(const std::string& message) const
	{
		std::cout << "[" << m_assistantName << "] " << message << std::endl;

		if (m_audioPipeline)
		{
			m_audioPipeline->Speak(message);
		}
	}


} // namespace app