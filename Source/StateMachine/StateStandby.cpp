/**
 * @file StateStandby.cpp
 * @brief 待機状態クラス実装
 */
#include "stdafx.h"
#include "StateStandby.h"


namespace app
{


	void StateStandby::OnEnter(const nlohmann::json& profile)
	{
		std::cout << "[StateStandby] OnEnter" << std::endl;
	}


	void StateStandby::OnUpdate()
	{
		// TODO: Standby状態の定期処理を実装する
	}


	void StateStandby::OnExit()
	{
		std::cout << "[StateStandby] OnExit" << std::endl;
	}


	const char* StateStandby::GetName() const
	{
		return "Standby";
	}


} // namespace app