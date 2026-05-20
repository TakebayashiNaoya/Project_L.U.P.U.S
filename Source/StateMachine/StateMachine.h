/**
 * @file StateMachine.h
 * @brief ステートマシン管理クラス
 */
#pragma once
#include "stdafx.h"


namespace app
{


	/** 前方宣言 */
	class IState;


	/**
	 * @brief システムの状態を保持し、スレッドセーフに遷移を管理するクラス
	 */
	class StateMachine
	{
	public:
		StateMachine();
		~StateMachine();

		/**
		 * @brief ステートマシンを初期化する
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		void Init(const nlohmann::json& profile);

		/**
		 * @brief 現在の状態の OnUpdate を呼び出す
		 */
		void Update();

		/**
		 * @brief 指定した状態名に遷移する（スレッドセーフ）
		 * @param stateName 遷移先の状態名（"Standby" / "TaskFocus" / "TaskCompleted"）
		 */
		void Transition(const std::string& stateName);

		/**
		 * @brief 現在の状態名を返す
		 * @return 状態名文字列
		 */
		std::string GetCurrentStateName() const;


	private:
		/**
		 * @brief 状態名からIStateインスタンスを生成して返す
		 * @param stateName 状態名
		 * @return IState のユニークポインタ
		 */
		std::unique_ptr<IState> CreateState(const std::string& stateName) const;


	private:
		/** 読み込み済みのシステムプロファイル */
		nlohmann::json m_profile;
		/** 現在の状態 */
		std::unique_ptr<IState> m_currentState;
		/** 状態遷移の排他制御用ミューテックス */
		mutable std::mutex m_mutex;
	};


} // namespace app
