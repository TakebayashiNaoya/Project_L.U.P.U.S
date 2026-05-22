/**
 * @file AudioPipeline.cpp
 * @brief 音声パイプラインクラス実装
 */
#include "stdafx.h"
#include "Source/Audio/AudioPipeline.h"
#include <cctype>
#include <windows.h>
#include <mmsystem.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "winmm.lib")


namespace app
{
	namespace
	{
		/** Style-Bert-VITS2 サーバーのホスト名 */
		constexpr const wchar_t* TTS_SERVER_HOST = L"127.0.0.1";
		/** Style-Bert-VITS2 サーバーのポート番号 */
		constexpr INTERNET_PORT TTS_SERVER_PORT = 5000;
		/** Style-Bert-VITS2 の音声合成エンドポイント */
		constexpr const wchar_t* TTS_ENDPOINT = L"/voice";
		/** 使用するモデルの ID(デフォルトモデル) */
		constexpr int TTS_MODEL_ID = 0;
		/** 使用する話者の ID(デフォルト話者) */
		constexpr int TTS_SPEAKER_ID = 0;
		/** WinHTTP の接続タイムアウト(ミリ秒) */
		constexpr DWORD TTS_CONNECT_TIMEOUT_MS = 5000;
		/** WinHTTP のレスポンス受信タイムアウト(ミリ秒) */
		constexpr DWORD TTS_RECEIVE_TIMEOUT_MS = 30000;
	}


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
		// 既に停止済みの場合は何もしない
		if (!m_isRunning) return;

		m_isRunning = false;

		// ThreadLoop() が条件変数で待機している場合に備えて通知する
		m_condition.notify_all();

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


	void AudioPipeline::Speak(const std::string& text)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_speakQueue.push(text);
		}

		// ThreadLoop() の wait() を解除して即時処理を開始させる
		m_condition.notify_one();

		std::cout << "[AudioPipeline] Speak キューに追加: " << text << std::endl;
	}


	void AudioPipeline::ThreadLoop()
	{
		while (m_isRunning)
		{
			std::string text;

			// キューが空の間は条件変数で待機する
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_condition.wait(lock, [this] {
					return !m_speakQueue.empty() || !m_isRunning;
					});

				// Stop() による終了通知を受けた場合は即座に抜ける
				if (!m_isRunning && m_speakQueue.empty())
				{
					break;
				}

				text = m_speakQueue.front();
				m_speakQueue.pop();
			}

			if (text.empty())
			{
				continue;
			}

			// TTS リクエストを送信して WAV を取得し、再生する
			std::cout << "[AudioPipeline] TTS リクエスト送信: " << text << std::endl;
			const std::vector<char> wavData = FetchWav(text);

			if (wavData.empty())
			{
				std::cerr << "[AudioPipeline] WAV の取得に失敗しました。テキスト: " << text << std::endl;
				continue;
			}

			// PlayWav() はチャンク再生が終わるまでブロッキングする
			PlayWav(wavData);
		}
	}


	std::vector<char> AudioPipeline::FetchWav(const std::string& text) const
	{
		std::vector<char> wavData;

		// クエリパラメータを組み立てる
		// text は UTF-8 でパーセントエンコードして渡す
		const std::string encodedText = UrlEncode(text);
		const std::string queryStr =
			"?text=" + encodedText +
			"&model_id=" + std::to_string(TTS_MODEL_ID) +
			"&speaker_id=" + std::to_string(TTS_SPEAKER_ID);

		// wstring に変換してリクエストパスを組み立てる
		const std::wstring path =
			std::wstring(TTS_ENDPOINT) +
			std::wstring(queryStr.begin(), queryStr.end());

		// HTTP セッションを開く(HTTP のため WINHTTP_FLAG_SECURE は不要)
		HINTERNET hSession = WinHttpOpen(
			L"LUPUS/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0
		);
		if (!hSession)
		{
			std::cerr << "[AudioPipeline] WinHttpOpen に失敗しました。Code: "
				<< GetLastError() << std::endl;
			return wavData;
		}

		WinHttpSetTimeouts(
			hSession,
			TTS_CONNECT_TIMEOUT_MS,
			TTS_CONNECT_TIMEOUT_MS,
			TTS_CONNECT_TIMEOUT_MS,
			TTS_RECEIVE_TIMEOUT_MS
		);

		HINTERNET hConnect = WinHttpConnect(
			hSession,
			TTS_SERVER_HOST,
			TTS_SERVER_PORT,
			0
		);
		if (!hConnect)
		{
			std::cerr << "[AudioPipeline] WinHttpConnect に失敗しました。Code: "
				<< GetLastError() << std::endl;
			WinHttpCloseHandle(hSession);
			return wavData;
		}

		// HTTP(非 HTTPS)でリクエストを開く
		HINTERNET hRequest = WinHttpOpenRequest(
			hConnect,
			L"GET",
			path.c_str(),
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			0  // WINHTTP_FLAG_SECURE を渡さない = HTTP
		);
		if (!hRequest)
		{
			std::cerr << "[AudioPipeline] WinHttpOpenRequest に失敗しました。Code: "
				<< GetLastError() << std::endl;
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return wavData;
		}

		const BOOL isSent = WinHttpSendRequest(
			hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			WINHTTP_NO_REQUEST_DATA,
			0,
			0,
			0
		);
		if (!isSent)
		{
			std::cerr << "[AudioPipeline] WinHttpSendRequest に失敗しました。Code: "
				<< GetLastError() << std::endl;
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return wavData;
		}

		if (!WinHttpReceiveResponse(hRequest, nullptr))
		{
			std::cerr << "[AudioPipeline] WinHttpReceiveResponse に失敗しました。Code: "
				<< GetLastError() << std::endl;
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return wavData;
		}

		// HTTP ステータスコードを確認する。200 以外はエラーレスポンスのため WAV として扱わない
		DWORD dwStatusCode = 0;
		DWORD dwStatusCodeSize = sizeof(dwStatusCode);
		WinHttpQueryHeaders(
			hRequest,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&dwStatusCode,
			&dwStatusCodeSize,
			WINHTTP_NO_HEADER_INDEX
		);

		if (dwStatusCode != 200)
		{
			std::cerr << "[AudioPipeline] TTS サーバーがエラーを返しました。HTTP Status: "
				<< dwStatusCode << std::endl;
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return wavData;
		}

		// レスポンスボディ(WAV バイナリ)を読み込む
		DWORD dwSize = 0;
		do
		{
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
			if (dwSize == 0) break;

			const std::size_t offset = wavData.size();
			wavData.resize(offset + dwSize);
			DWORD dwDownloaded = 0;

			if (!WinHttpReadData(hRequest, wavData.data() + offset, dwSize, &dwDownloaded))
			{
				std::cerr << "[AudioPipeline] WinHttpReadData に失敗しました。Code: "
					<< GetLastError() << std::endl;
				wavData.clear();
				break;
			}

			// 実際に読み込めたバイト数にリサイズする
			wavData.resize(offset + dwDownloaded);

		} while (dwSize > 0);

		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);

		std::cout << "[AudioPipeline] WAV 取得完了。サイズ: " << wavData.size() << " bytes" << std::endl;
		return wavData;
	}


	void AudioPipeline::PlayWav(const std::vector<char>& wavData) const
	{
		if (wavData.empty())
		{
			return;
		}

		// SND_MEMORY: メモリ上の WAV データを直接再生する
		// SND_SYNC:   再生が終わるまでブロッキングする(チャンク順次再生のために必須)
		// SND_NODEFAULT: 失敗時にデフォルトのビープ音を鳴らさない
		// Unicode ビルドでは PlaySound が PlaySoundW に解決されるため、
		// メモリ上の WAV バイナリ(LPCSTR)を渡す際は PlaySoundA を明示する
		const BOOL result = PlaySoundA(
			wavData.data(),
			nullptr,
			SND_MEMORY | SND_SYNC | SND_NODEFAULT
		);

		if (!result)
		{
			std::cerr << "[AudioPipeline] PlaySound に失敗しました。Code: "
				<< GetLastError() << std::endl;
		}
	}


	std::string AudioPipeline::UrlEncode(const std::string& text) const
	{
		// RFC 3986 に基づきパーセントエンコードを行う。
		// 英数字と一部の記号(- _ . ~)以外をすべてエンコードする。
		// UTF-8 の日本語はそのままバイト単位でエンコードされる。
		std::string encoded;
		encoded.reserve(text.size() * 3);

		for (const unsigned char c : text)
		{
			if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			{
				encoded += static_cast<char>(c);
			}
			else
			{
				constexpr char hex[] = "0123456789ABCDEF";
				encoded += '%';
				encoded += hex[(c >> 4) & 0x0F];
				encoded += hex[c & 0x0F];
			}
		}

		return encoded;
	}


} // namespace app