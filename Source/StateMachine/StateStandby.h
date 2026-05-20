/**
 * @file StateStandby.h
 * @brief 待機状態クラス
 */
#pragma once
#include "Source/StateMachine/IState.h"


namespace app
{


	/**
	 * @brief 外出判定時のバックグラウンド待機状態
	 */
	class StateStandby : public IState
	{
	public:
		void OnEnter(const nlohmann::json& profile) override;
		void OnUpdate() override;
		void OnExit() override;
		const char* GetName() const override;
	};


} // namespace app