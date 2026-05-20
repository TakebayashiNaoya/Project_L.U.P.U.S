/**
 * @file AudioPipeline.h
 * @brief 音声パイプラインクラス（スケルトン）
 */
#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include "Source/Audio/IAudioPipeline.h"


namespace app
{


	/**
	 * @brief Silero VAD + Whisper.cpp を用いた音声認識パイプライン
	 * @details フェーズ2以降で実装する。フェーズ1ではスケルトンのみ。
	 */
	class AudioPipeline : public IAudioPipeline
	{
	public:
		AudioPipeline();
		~AudioPipeline();

		/** @brief 音声パイプラインを開始する */
		void Start() override;
		/** @brief 音声パイプラインを停止する */
		void Stop() override;
		/**
		 * @brief 最後に認識されたテキストを返す
		 * @return 認識テキスト文字列
		 */
		std::string GetLastTranscription() const override;


	private:
		/** @brief 音声監視スレッドのメインループ */
		void ThreadLoop();


	private:
		/** 音声監視スレッド */
		std::thread m_thread;
		/** 音声監視スレッドの実行状態 */
		std::atomic<bool> m_isRunning{ false };
		/** 最後に認識されたテキストを保護するミューテックス */
		mutable std::mutex m_mutex;
		/** 最後に認識されたテキスト */
		std::string m_lastTranscription;
	};


} // namespace app