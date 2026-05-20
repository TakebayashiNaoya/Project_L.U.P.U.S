/**
 * @file IAudioPipeline.h
 * @brief 音声パイプラインのインターフェース
 */
#pragma once
#include "stdafx.h"


namespace app
{


	/**
	 * @brief VAD + Whisper を用いた音声認識パイプラインのインターフェース
	 * @details フェーズ2以降で実装する。フェーズ1ではスケルトンのみ。
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
	};


} // namespace app
