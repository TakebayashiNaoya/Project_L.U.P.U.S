/**
 * @file IAudioPipeline.h
 * @brief 音声パイプラインのインターフェース
 */
#pragma once
#include <string>


namespace app
{


	/**
	 * @brief 音声パイプラインが実装すべきインターフェース
	 */
	class IAudioPipeline
	{
	public:
		/** @brief デストラクタ */
		virtual ~IAudioPipeline() = default;

		/** @brief 音声パイプラインを開始する */
		virtual void Start() = 0;

		/** @brief 音声パイプラインを停止する */
		virtual void Stop() = 0;

		/**
		 * @brief 最後に認識されたテキストを返す
		 * @return 認識テキスト文字列
		 */
		virtual std::string GetLastTranscription() const = 0;

		/**
		 * @brief テキストを音声再生キューに追加する
		 * @param text 読み上げるテキスト文字列
		 * @details スレッドセーフであること。実装クラスが排他制御の責務を持つ。
		 */
		virtual void Speak(const std::string& text) = 0;
	};


} // namespace app