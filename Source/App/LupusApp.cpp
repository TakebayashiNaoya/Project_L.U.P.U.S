/**
 * @file LupusApp.cpp
 * @brief アプリケーション統括クラス実装
 */
#include "stdafx.h"
#include "LupusApp.h"
#include "Source/Profile/ProfileManager.h"
#include "Source/StateMachine/StateMachine.h"
#include "Source/Monitor/MonitorThread.h"
#include "Source/Monitor/NetworkMonitor/NetworkMonitor.h"
#include "Source/Monitor/NotionMonitor/NotionMonitor.h"
#include "Source/Monitor/CodeReviewMonitor/CodeReviewMonitor.h"
#include "Source/Monitor/DocumentReviewMonitor/DocumentReviewMonitor.h"
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

		// 設定ファイルの読み込みと統合
		m_profileManager = std::make_unique<ProfileManager>();
		if (!m_profileManager->Load())
		{
			std::cerr << "[LupusApp] プロファイルの読み込みに失敗しました" << std::endl;
			return false;
		}

		const nlohmann::json& profile = m_profileManager->GetProfile();

		// ステートマシンの初期化
		m_stateMachine = std::make_unique<StateMachine>();
		m_stateMachine->Init(profile);

		// 監視スレッドの初期化
		const int intervalMs = profile.value("monitor_interval_ms", 1000);
		m_monitorThread = std::make_unique<MonitorThread>();
		m_monitorThread->Init(*m_stateMachine, intervalMs);

		// 監視モジュールの登録
		// NetworkMonitor: RequiresNetwork() == false のため常時動作する
		m_monitorThread->AddMonitor(std::make_unique<NetworkMonitor>(profile));
		// NotionMonitor: RequiresNetwork() == true のためネットワーク接続時のみ動作する
		m_monitorThread->AddMonitor(std::make_unique<NotionMonitor>(profile));
		// CodeReviewMonitor: 常時動作。差分スキャンにより在宅外でも低コストで動作する
		m_monitorThread->AddMonitor(std::make_unique<CodeReviewMonitor>(profile));
		// DocumentReviewMonitor: 常時動作。差分スキャンにより在宅外でも低コストで動作する
		m_monitorThread->AddMonitor(std::make_unique<DocumentReviewMonitor>(profile));

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
			// メインループの周期を制御する
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			// TODO: フェーズ2以降でここに音声入力の処理や UI の更新を追加する
		}
	}


	void LupusApp::Shutdown()
	{
		// 多重呼び出しを防ぐ
		if (!m_isRunning) return;

		std::cout << "[LupusApp] シャットダウンを開始します" << std::endl;

		// アプリケーションの実行状態を false に設定する
		m_isRunning = false;

		// MonitorThread を先に停止する。
		// AudioPipeline より先に止めることで、スレッド動作中に StateMachine::Evaluate() が
		// 呼ばれ続けるのを防ぐ。
		// reset() でスマートポインタを nullptr にすることで、デストラクタ経由の
		// 二重 Stop() 呼び出しを防止する。
		if (m_monitorThread)
		{
			m_monitorThread->Stop();
			m_monitorThread.reset();
		}

		if (m_audioPipeline)
		{
			m_audioPipeline->Stop();
			m_audioPipeline.reset();
		}

		std::cout << "[LupusApp] シャットダウンが完了しました" << std::endl;
	}


} // namespace app