/**
 * @file StateTaskFocus.h
 * @brief 集中状態クラス
 */
#pragma once
#include "stdafx.h"
#include "IState.h"


namespace app
{


	/**
	 * @brief 未完了タスクあり・規約違反・姿勢悪化を検知した際の集中促進状態
	 */
	class StateTaskFocus : public IState
	{
	public:
		void OnEnter(const nlohmann::json& profile) override;
		void OnUpdate() override;
		void OnExit() override;
		const char* GetName() const override;
	};


} // namespace app
