/**
 * @file StateTaskFocus.cpp
 * @brief 集中状態クラス実装
 */
#include "stdafx.h"
#include "StateTaskFocus.h"


namespace app
{


	void StateTaskFocus::OnEnter(const nlohmann::json& profile)
	{
		const std::string prompt = profile.value(
			nlohmann::json::json_pointer("/states/TaskFocus"), "");
		std::cout << "[StateTaskFocus] OnEnter | プロンプト: " << prompt << std::endl;
	}


	void StateTaskFocus::OnUpdate()
	{
		// TODO: TaskFocus状態の定期処理を実装する
		//       （コーディング規約違反の警告、姿勢悪化通知など）
	}


	void StateTaskFocus::OnExit()
	{
		std::cout << "[StateTaskFocus] OnExit" << std::endl;
	}


	const char* StateTaskFocus::GetName() const
	{
		return "TaskFocus";
	}


} // namespace app
