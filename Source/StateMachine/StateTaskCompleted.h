/**
 * @file StateTaskCompleted.h
 * @brief タスク完了状態クラス
 */
#pragma once
#include "Source/StateMachine/IState.h"


namespace app
{


	/**
	 * @brief 自宅かつ全タスク完了時のリラックス状態
	 */
	class StateTaskCompleted : public IState
	{
	public:
		void OnEnter(const nlohmann::json& profile) override;
		void OnUpdate() override;
		void OnExit() override;
		const char* GetName() const override;
	};


} // namespace app