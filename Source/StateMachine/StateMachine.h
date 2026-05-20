/**
 * @file StateMachine.h
 * @brief ステートマシン管理クラス
 */
#pragma once
#include <memory>
#include <mutex>
#include <string>
#include "external/json.hpp"
#include "Source/Monitor/SystemContext.h"


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
		 * @brief SystemContext を参照して自律的に遷移先を判断し、必要なら遷移する
		 * @details センサー駆動による状態遷移はこのメソッドを経由する
		 * @param context 各 Monitor が更新した共有コンテキスト
		 */
		void Evaluate(const SystemContext& context);

		/**
		 * @brief 指定した状態名に強制遷移する（スレッドセーフ）
		 * @details 音声コマンドや UI など、外部からの明示的な遷移要求に使用する
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

		/**
		 * @brief SystemContext から遷移すべき状態名を算出して返す
		 * @param context 各 Monitor が更新した共有コンテキスト
		 * @return 遷移先の状態名
		 */
		std::string ResolveStateName(const SystemContext& context) const;


	private:
		/** 読み込み済みのシステムプロファイル */
		nlohmann::json m_profile;
		/** 現在の状態 */
		std::unique_ptr<IState> m_currentState;
		/** 状態遷移の排他制御用ミューテックス */
		mutable std::mutex m_mutex;
	};


} // namespace app