/**
 * @file LupusApp.h
 * @brief アプリケーション統括クラス
 */
#pragma once
#include <memory>
#include <atomic>


namespace app
{


	/** 前方宣言 */
	class ProfileLoader;
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

		/** @brief メインループを実行する（ブロッキング） */
		void Run();

		/** @brief アプリケーションを終了する */
		void Shutdown();


	private:
		/** システムプロファイルの読み込みクラス */
		std::unique_ptr<ProfileLoader> m_profileLoader;
		/** ステートマシン管理クラス */
		std::unique_ptr<StateMachine>  m_stateMachine;
		/** 監視スレッド統括クラス */
		std::unique_ptr<MonitorThread> m_monitorThread;
		/** 音声パイプラインクラス */
		std::unique_ptr<AudioPipeline> m_audioPipeline;
		/** アプリケーションの実行状態 */
		std::atomic<bool> m_isRunning{ false };
	};


} // namespace app