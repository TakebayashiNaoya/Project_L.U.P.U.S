/**
 * @file MonitorThread.cpp
 * @brief 監視スレッド統括クラス実装
 */
#include "stdafx.h"
#include "MonitorThread.h"
#include "IMonitor.h"
#include "Source/StateMachine/StateMachine.h"


namespace app
{
	MonitorThread::MonitorThread()
	{}


	MonitorThread::~MonitorThread()
	{
		Stop();
	}


	void MonitorThread::Init(StateMachine& stateMachine, int intervalMs)
	{
		m_stateMachine = &stateMachine;
		m_interval = std::chrono::milliseconds(intervalMs);
	}


	void MonitorThread::AddMonitor(std::unique_ptr<IMonitor> monitor)
	{
		m_monitors.push_back(std::move(monitor));
	}


	void MonitorThread::Start()
	{
		if (m_isRunning) return;

		m_isRunning = true;
		m_thread = std::thread(&MonitorThread::ThreadLoop, this);
		std::cout << "[MonitorThread] 監視スレッドを開始しました" << std::endl;
	}


	void MonitorThread::Stop()
	{
		// 既に停止済みの場合は何もしない
		if (!m_isRunning) return;

		m_isRunning = false;
		if (m_thread.joinable())
		{
			m_thread.join();
		}
		std::cout << "[MonitorThread] 監視スレッドを停止しました" << std::endl;
	}


	void MonitorThread::ThreadLoop()
	{
		while (m_isRunning)
		{
			// パス1: RequiresNetwork() == false のモニターを先に実行し、
			//        m_context.m_isConnected を確定させる
			for (auto& monitor : m_monitors)
			{
				if (monitor->RequiresNetwork()) continue;
				monitor->Observe(m_context);
			}

			// パス2: RequiresNetwork() == true のモニターを実行する。
			//        パス1で m_isConnected が確定しているためスキップ判定が正確になる
			for (auto& monitor : m_monitors)
			{
				if (!monitor->RequiresNetwork()) continue;
				if (!m_context.m_isConnected) continue;
				monitor->Observe(m_context);
			}

			// 全モニターの観測結果をもとに StateMachine に遷移判断を委ねる
			m_stateMachine->Evaluate(m_context);

			std::this_thread::sleep_for(m_interval);
		}
	}


} // namespace app