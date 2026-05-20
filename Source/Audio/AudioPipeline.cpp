/**
 * @file AudioPipeline.cpp
 * @brief 音声パイプラインクラス実装（スケルトン）
 */
#include "stdafx.h"
#include "Source/Audio/AudioPipeline.h"


namespace app
{
	AudioPipeline::AudioPipeline()
	{}


	AudioPipeline::~AudioPipeline()
	{
		Stop();
	}


	void AudioPipeline::Start()
	{
		if (m_isRunning) return;

		m_isRunning = true;
		m_thread = std::thread(&AudioPipeline::ThreadLoop, this);
		std::cout << "[AudioPipeline] 音声パイプラインを開始しました" << std::endl;
	}


	void AudioPipeline::Stop()
	{
		m_isRunning = false;

		if (m_thread.joinable())
		{
			m_thread.join();
		}
		std::cout << "[AudioPipeline] 音声パイプラインを停止しました" << std::endl;
	}


	std::string AudioPipeline::GetLastTranscription() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_lastTranscription;
	}


	void AudioPipeline::ThreadLoop()
	{
		while (m_isRunning)
		{
			// TODO: フェーズ2で以下を実装する
			//       1. Silero VAD により音声区間を検出する
			//       2. 検出した音声区間を Whisper.cpp に渡してテキストに変換する
			//       3. 変換結果を m_lastTranscription に格納する
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}


} // namespace app
