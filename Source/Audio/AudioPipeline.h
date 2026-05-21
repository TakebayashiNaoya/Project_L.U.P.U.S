/**
 * @file AudioPipeline.h
 * @brief 音声パイプラインクラス
 */
#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include <mmsystem.h>
#include <winhttp.h>
#include "Source/Audio/IAudioPipeline.h"


namespace app
{


	/**
	 * @brief Style-Bert-VITS2 を用いた音声合成パイプライン
	 * @details Speak() でテキストをキューに積み、ThreadLoop() が順次
	 *          Style-Bert-VITS2 ローカル HTTP サーバーへリクエストを送り
	 *          WAV を受け取って PlaySound() で再生する。
	 *          PlaySound() はチャンクごとにブロッキング再生するため、
	 *          前のチャンクが言い終わってから次のチャンクを再生する。
	 *          Speak() はスレッドセーフ。
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
		/**
		 * @brief テキストを音声再生キューに追加する
		 * @param text 読み上げるテキスト文字列
		 * @details スレッドセーフ。条件変数で ThreadLoop() に即時通知する。
		 */
		void Speak(const std::string& text) override;


	private:
		/**
		 * @brief 音声処理スレッドのメインループ
		 * @details キューからテキストを取り出し、TTS リクエスト → WAV 再生を繰り返す。
		 */
		void ThreadLoop();

		/**
		 * @brief Style-Bert-VITS2 にテキストを送り WAV バイナリを取得する
		 * @param text 読み上げるテキスト文字列
		 * @return WAV バイナリ。失敗時は空の vector を返す。
		 */
		std::vector<char> FetchWav(const std::string& text) const;

		/**
		 * @brief WAV バイナリをメモリ上で再生する(ブロッキング)
		 * @param wavData WAV フォーマットのバイナリデータ
		 */
		void PlayWav(const std::vector<char>& wavData) const;

		/**
		 * @brief URL エンコードを行う
		 * @param text エンコード対象の文字列(UTF-8)
		 * @return パーセントエンコード済み文字列
		 */
		std::string UrlEncode(const std::string& text) const;


	private:
		/** 音声処理スレッド */
		std::thread m_thread;
		/** 音声処理スレッドの実行状態 */
		std::atomic<bool> m_isRunning{ false };
		/** キューと認識テキストを保護するミューテックス */
		mutable std::mutex m_mutex;
		/** キューにデータが積まれたことを通知する条件変数 */
		std::condition_variable m_condition;
		/** 音声再生キュー */
		std::queue<std::string> m_speakQueue;
		/** 最後に認識されたテキスト */
		std::string m_lastTranscription;
	};


} // namespace app