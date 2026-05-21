/**
 * @file StateMachine.h
 * @brief ステートマシン管理クラス
 */
#pragma once
#include <memory>
#include <mutex>
#include <string>
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
		 * @param context               各 Monitor が更新する共有コンテキスト(State に DI する)
		 * @param systemPrompt          LLM 連携用のシステムプロンプト文字列(StateTaskFocus に DI する)
		 * @param assistantName         アシスタント名(全 State に DI する)
		 * @param instantWarningMessage Level 1 即時警告メッセージ(StateTaskFocus に DI する)
		 * @param standbyMessage        Standby 状態のメッセージ(StateStandby に DI する)
		 * @param completionMessage     TaskCompleted 状態のメッセージ(StateTaskCompleted に DI する)
		 */
		void Init(
			SystemContext& context,
			const std::string& systemPrompt,
			const std::string& assistantName,
			const std::string& instantWarningMessage,
			const std::string& standbyMessage,
			const std::string& completionMessage
		);

		/**
		 * @brief 現在の状態の OnUpdate を呼び出す
		 */
		void Update();

		/**
		 * @brief SystemContext を参照して自律的に遷移先を判断し、必要なら遷移する
		 * @param context 各 Monitor が更新した共有コンテキスト
		 */
		void Evaluate(const SystemContext& context);

		/**
		 * @brief 指定した状態名に強制遷移する(スレッドセーフ)
		 * @param stateName 遷移先の状態名("Standby" / "TaskFocus" / "TaskCompleted")
		 */
		void Transition(const std::string& stateName);

		/**
		 * @brief 現在の状態名を返す
		 * @return 状態名文字列
		 */
		std::string GetCurrentStateName() const;


	private:
		/**
		 * @brief 状態名から IState インスタンスを生成して返す
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
		/** LLM 連携用のシステムプロンプト文字列(StateTaskFocus に DI する) */
		std::string m_systemPrompt;
		/** アシスタント名(全 State のログ冒頭ラベルに使用) */
		std::string m_assistantName;
		/** Level 1 即時警告メッセージ(StateTaskFocus に DI する) */
		std::string m_instantWarningMessage;
		/** Standby 状態のメッセージ(StateStandby に DI する) */
		std::string m_standbyMessage;
		/** TaskCompleted 状態のメッセージ(StateTaskCompleted に DI する) */
		std::string m_completionMessage;
		/** 各 Monitor が更新する共有コンテキストへのポインタ(非所有) */
		SystemContext* m_context = nullptr;
		/** 現在の状態 */
		std::unique_ptr<IState> m_currentState;
		/** 状態遷移の排他制御用 Mutex */
		mutable std::mutex m_mutex;
	};


} // namespace app