/**
 * @file LupusApp.cpp
 * @brief アプリケーション統括クラス実装
 */
#include "stdafx.h"
#include "LupusApp.h"
#include "Source/Profile/ProfileLoader.h"
#include "Source/StateMachine/StateMachine.h"
#include "Source/Monitor/MonitorThread.h"
#include "Source/Monitor/NetworkMonitor/NetworkMonitor.h"
#include "Source/Audio/AudioPipeline.h"


namespace app
{
	LupusApp::LupusApp()
	{}


	LupusApp::~LupusApp()
	{
		Shutdown();
	}


	bool LupusApp::Init()
	{
		std::cout << "[LupusApp] 初期化を開始します" << std::endl;

		// プロファイルの読み込み
		m_profileLoader = std::make_unique<ProfileLoader>();
		if (!m_profileLoader->Load())
		{
			std::cerr << "[LupusApp] プロファイルの読み込みに失敗しました" << std::endl;
			return false;
		}

		const nlohmann::json& profile = m_profileLoader->GetProfile();

		// ステートマシンの初期化
		m_stateMachine = std::make_unique<StateMachine>();
		m_stateMachine->Init(profile);

		// 監視スレッドの初期化
		const int intervalMs = profile.value("monitor_interval_ms", 1000);
		m_monitorThread = std::make_unique<MonitorThread>();
		m_monitorThread->Init(*m_stateMachine, intervalMs);

		// 監視モジュールの登録
		m_monitorThread->AddMonitor(std::make_unique<NetworkMonitor>(profile));

		// 音声パイプラインの初期化
		m_audioPipeline = std::make_unique<AudioPipeline>();

		std::cout << "[LupusApp] 初期化が完了しました" << std::endl;
		return true;
	}


	void LupusApp::Run()
	{
		// アプリケーションの実行状態を true に設定する
		m_isRunning = true;

		// 監視スレッドと音声パイプラインを開始する
		m_monitorThread->Start();
		m_audioPipeline->Start();

		std::cout << "[LupusApp] メインループを開始します。終了するには Enter キーを押してください" << std::endl;

		// メインループ
		while (m_isRunning)
		{
			// 現在の状態の Update を呼び出す
			m_stateMachine->Update();
			// メインループの周期を制御する（例: 100msごと）
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			// TODO: フェーズ2以降でここに音声入力の処理やUIの更新を追加する
		}
	}


	void LupusApp::Shutdown()
	{
		// 多重呼び出しを防ぐ
		if (!m_isRunning) return;

		std::cout << "[LupusApp] シャットダウンを開始します" << std::endl;

		// アプリケーションの実行状態を false に設定する
		m_isRunning = false;

		// 監視スレッドと音声パイプラインを停止する
		if (m_audioPipeline) m_audioPipeline->Stop();
		if (m_monitorThread) m_monitorThread->Stop();

		std::cout << "[LupusApp] シャットダウンが完了しました" << std::endl;
	}


} // namespace app