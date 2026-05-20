/**
 * @file StateMachine.cpp
 * @brief ステートマシン管理クラス実装
 */
#include "stdafx.h"
#include "StateMachine.h"
#include "IState.h"
#include "StateStandby.h"
#include "StateTaskFocus.h"
#include "StateTaskCompleted.h"


namespace app
{
	StateMachine::StateMachine()
	{}


	StateMachine::~StateMachine()
	{}


	void StateMachine::Init(const nlohmann::json& profile)
	{
		m_profile = profile;

		// 初期状態は Standby
		m_currentState = CreateState("Standby");
		m_currentState->OnEnter(m_profile);
	}


	void StateMachine::Update()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_currentState)
		{
			m_currentState->OnUpdate();
		}
	}


	void StateMachine::Transition(const std::string& stateName)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// 同じ状態への遷移は無視する
		if (m_currentState && std::string(m_currentState->GetName()) == stateName)
		{
			return;
		}

		// 遷移先が有効でなければ弾く
		auto nextState = CreateState(stateName);
		if (!nextState)
		{
			std::cerr << "[StateMachine] 未知の状態名: " << stateName << std::endl;
			return;
		}

		if (m_currentState)
		{
			m_currentState->OnExit();
		}

		m_currentState = std::move(nextState);
		std::cout << "[StateMachine] 状態遷移: " << stateName << std::endl;
		m_currentState->OnEnter(m_profile);
	}


	std::string StateMachine::GetCurrentStateName() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_currentState)
		{
			// GetName() の結果を std::string にコピーしてからロックを解放する
			return std::string(m_currentState->GetName());
		}
		return "None";
	}


	std::unique_ptr<IState> StateMachine::CreateState(const std::string& stateName) const
	{
		switch (Hash32(stateName.c_str()))
		{
		case Hash32("Standby"):
			return std::make_unique<StateStandby>();
		case Hash32("TaskFocus"):
			return std::make_unique<StateTaskFocus>();
		case Hash32("TaskCompleted"):
			return std::make_unique<StateTaskCompleted>();
		default:
			return nullptr;
		}
	}


} // namespace app
