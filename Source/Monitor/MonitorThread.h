/**
 * @file MonitorThread.h
 * @brief 監視スレッド統括クラス
 */
#pragma once
#include "stdafx.h"
#include <vector>
#include <chrono>


namespace app
{


	/** 前方宣言 */
	class IMonitor;
	class StateMachine;


	/**
	 * @brief 複数の IMonitor を別スレッドで定期実行し、StateMachine に遷移を通知するクラス
	 */
	class MonitorThread
	{
	public:
		MonitorThread();
		~MonitorThread();

		/**
		 * @brief 監視スレッドを初期化する
		 * @param stateMachine 遷移通知先のステートマシン
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
		/** 遷移通知先のステートマシンへのポインタ */
		StateMachine* m_stateMachine = nullptr;
		/** 監視モジュールのリスト */
		std::vector<std::unique_ptr<IMonitor>> m_monitors;
		/** 監視スレッド */
		std::thread m_thread;
		/** 監視スレッドの実行状態 */
		std::atomic<bool> m_isRunning{ false };
		/** 監視間隔 */
		std::chrono::milliseconds m_interval{ 1000 };
	};


} // namespace app
