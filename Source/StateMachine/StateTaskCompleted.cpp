/**
 * @file StateTaskCompleted.cpp
 * @brief タスク完了状態クラス実装
 */
#include "stdafx.h"
#include "StateTaskCompleted.h"


namespace app
{


	void StateTaskCompleted::OnEnter(const nlohmann::json& profile)
	{
		std::cout << "[StateTaskCompleted] OnEnter" << std::endl;
	}


	void StateTaskCompleted::OnUpdate()
	{
		// TODO: TaskCompleted状態の定期処理を実装する
		//       （称賛メッセージの表示、休憩促進通知など）
	}


	void StateTaskCompleted::OnExit()
	{
		std::cout << "[StateTaskCompleted] OnExit" << std::endl;
	}


	const char* StateTaskCompleted::GetName() const
	{
		return "TaskCompleted";
	}


} // namespace app