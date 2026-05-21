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
#include "Source/LLM/ILLMClient.h"
#include "Source/LLM/GeminiClient.h"


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
		const std::string& systemPrompt = m_profileManager->GetSystemPrompt();
		const std::string assistantName = m_profileManager->GetAssistantName();
		const std::string instantWarningMessage = m_profileManager->GetInstantWarningMessage();
		const std::string standbyMessage = m_profileManager->GetStandbyMessage();
		const std::string completionMessage = m_profileManager->GetCompletionMessage();

		// APIキーの取得とLLMクライアントの生成
		const std::string geminiApiKey = m_profileManager->GetGeminiApiKey();
		std::unique_ptr<ILLMClient> llmClient = std::make_unique<GeminiClient>(geminiApiKey);

		// ステートマシンの初期化
		// 全 State が必要とする文字列を ProfileManager から受け取り、StateMachine 経由で DI する
		m_stateMachine = std::make_unique<StateMachine>();
		// StateMachine の初期化
		m_stateMachine->Init(
			m_context,
			systemPrompt,
			assistantName,
			instantWarningMessage,
			standbyMessage,
			completionMessage,
			std::move(llmClient)
		);

		// 監視スレッドの初期化
		const int intervalMs = profile.value("monitor_interval_ms", 1000);
		m_monitorThread = std::make_unique<MonitorThread>();
		m_monitorThread->Init(m_context, *m_stateMachine, intervalMs);

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
		m_isRunning = true;

		m_monitorThread->Start();
		m_audioPipeline->Start();

		std::cout << "[LupusApp] メインループを開始します。終了するには Enter キーを押してください" << std::endl;

		while (m_isRunning)
		{
			m_stateMachine->Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}


	void LupusApp::Shutdown()
	{
		if (!m_isRunning) return;

		std::cout << "[LupusApp] シャットダウンを開始します" << std::endl;

		m_isRunning = false;

		// MonitorThread を先に停止し、StateMachine::Evaluate() の呼び出しを断ち切る
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