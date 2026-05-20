/**
 * @file MonitorThread.h
 * @brief 監視スレッド統括クラス
 */
#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include "Source/Monitor/SystemContext.h"


namespace app
{


	/** 前方宣言 */
	class IMonitor;
	class StateMachine;


	/**
	 * @brief 複数の IMonitor を別スレッドで定期実行し、SystemContext を更新して
	 *        StateMachine::Evaluate() に渡すクラス
	 */
	class MonitorThread
	{
	public:
		MonitorThread();
		~MonitorThread();

		/**
		 * @brief 監視スレッドを初期化する
		 * @param stateMachine 遷移判断先のステートマシン
		 * @param intervalMs 監視間隔（ミリ秒）
		 */
		void Init(StateMachine& stateMachine, int intervalMs);

		/**
		 * @brief 監視モジュールを追加する
		 * @param monitor 追加する監視モジュール
		 */
		void AddMonitor(std::unique_ptr<IMonitor> monitor);

		/** @brief 監視スレッドを開始する */
		void Start();

		/** @brief 監視スレッドを停止し、スレッドの終了を待機する */
		void Stop();


	private:
		/** @brief スレッドのメインループ */
		void ThreadLoop();


	private:
		/** 遷移判断先のステートマシンへのポインタ */
		StateMachine* m_stateMachine = nullptr;
		/** 監視モジュールのリスト */
		std::vector<std::unique_ptr<IMonitor>> m_monitors;
		/** 全モニターの結果を集約する共有コンテキスト */
		SystemContext m_context;
		/** 監視スレッド */
		std::thread m_thread;
		/** 監視スレッドの実行状態 */
		std::atomic<bool> m_isRunning{ false };
		/** 監視間隔 */
		std::chrono::milliseconds m_interval{ 1000 };
	};


} // namespace app