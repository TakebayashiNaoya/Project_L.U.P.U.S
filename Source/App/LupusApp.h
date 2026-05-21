/**
 * @file LupusApp.h
 * @brief アプリケーション統括クラス
 */
#pragma once
#include <atomic>
#include <memory>
#include "Source/Monitor/SystemContext.h"


namespace app
{


	/** 前方宣言 */
	class ProfileManager;
	class StateMachine;
	class MonitorThread;
	class AudioPipeline;


	/**
	 * @brief L.U.P.U.S. の全モジュールを生成・起動・終了する統括クラス
	 */
	class LupusApp
	{
	public:
		LupusApp();
		~LupusApp();

		/**
		 * @brief アプリケーションを初期化する
		 * @return 初期化に成功した場合 true
		 */
		bool Init();

		/** @brief メインループを実行する(ブロッキング) */
		void Run();

		/** @brief アプリケーションを終了する */
		void Shutdown();


	private:
		/** 各 Monitor が更新し StateMachine と State が参照する共有コンテキスト */
		SystemContext m_context;
		/** 設定ファイルの中央管理クラス */
		std::unique_ptr<ProfileManager> m_profileManager;
		/** ステートマシン管理クラス */
		std::unique_ptr<StateMachine>   m_stateMachine;
		/** 監視スレッド統括クラス */
		std::unique_ptr<MonitorThread>  m_monitorThread;
		/** 音声パイプラインクラス */
		std::unique_ptr<AudioPipeline>  m_audioPipeline;
		/** アプリケーションの実行状態 */
		std::atomic<bool> m_isRunning{ false };
	};


} // namespace app