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
#include "Source/LLM/ILLMClient.h"


namespace app
{
	StateMachine::StateMachine()
	{}


	StateMachine::~StateMachine()
	{}


	void StateMachine::Init(
		SystemContext& context,
		const std::string& systemPrompt,
		const std::string& assistantName,
		const std::string& instantWarningMessage,
		const std::string& standbyMessage,
		const std::string& completionMessage,
		std::unique_ptr<ILLMClient> llmClient,
		IAudioPipeline* audioPipeline
	)
	{
		m_context = &context;
		m_systemPrompt = systemPrompt;
		m_assistantName = assistantName;
		m_instantWarningMessage = instantWarningMessage;
		m_standbyMessage = standbyMessage;
		m_completionMessage = completionMessage;
		m_llmClient = std::move(llmClient);
		m_audioPipeline = audioPipeline;

		// 初期状態は Standby
		m_currentState = CreateState("Standby");
		m_currentState->OnEnter();
	}


	void StateMachine::Update()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_currentState)
		{
			m_currentState->OnUpdate();
		}
	}


	void StateMachine::Evaluate(const SystemContext& context)
	{
		const std::string nextStateName = ResolveStateName(context);
		Transition(nextStateName);
	}


	void StateMachine::Transition(const std::string& stateName)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_currentState && std::string(m_currentState->GetName()) == stateName)
		{
			return;
		}

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
		m_currentState->OnEnter();
	}


	std::string StateMachine::GetCurrentStateName() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_currentState)
		{
			return std::string(m_currentState->GetName());
		}
		return "None";
	}


	std::unique_ptr<IState> StateMachine::CreateState(const std::string& stateName) const
	{
		switch (Hash32(stateName.c_str()))
		{
		case Hash32("Standby"):
			return std::make_unique<StateStandby>(
				m_assistantName,
				m_standbyMessage,
				m_audioPipeline
			);
		case Hash32("TaskFocus"):
			// SystemContext / systemPrompt / assistantName / instantWarningMessage / llmClient / audioPipeline を注入して生成する
			return std::make_unique<StateTaskFocus>(
				*m_context,
				m_systemPrompt,
				m_assistantName,
				m_instantWarningMessage,
				m_llmClient.get(),
				m_audioPipeline
			);
		case Hash32("TaskCompleted"):
			return std::make_unique<StateTaskCompleted>(
				m_assistantName,
				m_completionMessage,
				m_audioPipeline
			);
		default:
			return nullptr;
		}
	}


	std::string StateMachine::ResolveStateName(const SystemContext& context) const
	{
		// 完全に未接続の状態(オフライン)の時だけ Standby にする。
		// 外出先でもネットワークにつながっているなら作業モードへ遷移可能にする。
		if (!context.m_isConnected)
		{
			return "Standby";
		}

		// ネットワークにつながっているなら、在宅・外出に関わらずタスクや違反を確認する
		bool hasCodeViolations = false;
		{
			std::lock_guard<std::mutex> lock(context.m_codeViolationsMutex);
			hasCodeViolations = !context.m_codeViolations.empty();
		}

		bool hasDocumentWarnings = false;
		{
			std::lock_guard<std::mutex> lock(context.m_documentWarningsMutex);
			hasDocumentWarnings = !context.m_documentWarnings.empty();
		}

		if (context.m_hasPendingTasks || context.m_isSlouching || hasCodeViolations || hasDocumentWarnings)
		{
			return "TaskFocus";
		}

		return "TaskCompleted";
	}


} // namespace app