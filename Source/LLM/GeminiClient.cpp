/**
 * @file GeminiClient.cpp
 * @brief Gemini API クライアントクラス実装
 */
#include "stdafx.h"
#include "GeminiClient.h"
#include "external/json.hpp"

#pragma comment(lib, "winhttp.lib")


namespace app
{


	namespace
	{
		constexpr DWORD RESOLVE_TIMEOUT_MS = 10000;  // DNS 解決タイムアウト
		constexpr DWORD CONNECT_TIMEOUT_MS = 10000;  // 接続タイムアウト
		constexpr DWORD SEND_TIMEOUT_MS = 10000;  // 送信タイムアウト
		constexpr DWORD RECEIVE_TIMEOUT_MS = 60000;  // 受信タイムアウト
	}


	GeminiClient::GeminiClient(const std::string& apiKey)
		: m_apiKey(apiKey)
	{}


	std::string GeminiClient::GenerateResponse(const std::string& prompt)
	{
		// API キー未設定の場合は早期 return してネットワーク処理を行わない
		if (m_apiKey.empty())
		{
			return "[GeminiClient] エラー: API キーが設定されていません。secrets.json の gemini_api_key を確認してください。";
		}

		// Gemini API のエンドポイント
		const std::wstring serverName = L"generativelanguage.googleapis.com";
		const std::wstring path = L"/v1beta/models/gemini-2.5-flash:generateContent?key="
			+ std::wstring(m_apiKey.begin(), m_apiKey.end());

		// 送信する JSON ペイロードの構築
		const nlohmann::json requestJson = {
			{"contents", {
				{{ "parts", {{ {"text", prompt} }} }}
			}}
		};
		const std::string bodyString = requestJson.dump();

		std::string responseData;

		// WINHTTP_ACCESS_TYPE_DEFAULT_PROXY でシステムのプロキシ設定に自動追従する
		HINTERNET hSession = WinHttpOpen(
			L"LUPUS/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0
		);
		if (!hSession)
		{
			return "[GeminiClient] エラー: WinHttpOpen に失敗しました。Code: " + std::to_string(GetLastError());
		}

		// タイムアウト設定
		WinHttpSetTimeouts(hSession, RESOLVE_TIMEOUT_MS, CONNECT_TIMEOUT_MS, SEND_TIMEOUT_MS, RECEIVE_TIMEOUT_MS);

		// サーバー接続
		HINTERNET hConnect = WinHttpConnect(hSession, serverName.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
		if (!hConnect)
		{
			const DWORD err = GetLastError();
			WinHttpCloseHandle(hSession);
			return "[GeminiClient] エラー: WinHttpConnect に失敗しました。Code: " + std::to_string(err);
		}

		// リクエストハンドルのオープン(HTTPS 強制)
		HINTERNET hRequest = WinHttpOpenRequest(
			hConnect,
			L"POST",
			path.c_str(),
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE
		);
		if (!hRequest)
		{
			const DWORD err = GetLastError();
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return "[GeminiClient] エラー: WinHttpOpenRequest に失敗しました。Code: " + std::to_string(err);
		}

		// リクエストの送信
		const std::wstring headers = L"Content-Type: application/json\r\n";
		const BOOL isSent = WinHttpSendRequest(
			hRequest,
			headers.c_str(),
			static_cast<DWORD>(headers.length()),
			const_cast<char*>(bodyString.c_str()),
			static_cast<DWORD>(bodyString.length()),
			static_cast<DWORD>(bodyString.length()),
			0
		);
		if (!isSent)
		{
			const DWORD err = GetLastError();
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return "[GeminiClient] エラー: WinHttpSendRequest に失敗しました。Code: " + std::to_string(err);
		}

		// レスポンスの受信待ち
		if (!WinHttpReceiveResponse(hRequest, nullptr))
		{
			const DWORD err = GetLastError();
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return "[GeminiClient] エラー: WinHttpReceiveResponse に失敗しました。Code: " + std::to_string(err);
		}

		// レスポンスボディの読み込み
		DWORD dwSize = 0;
		do
		{
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
			{
				std::cerr << "[GeminiClient] WinHttpQueryDataAvailable に失敗しました。Code: "
					<< GetLastError() << std::endl;
				break;
			}

			if (dwSize == 0)
			{
				break;
			}

			std::vector<char> buffer(dwSize + 1, 0);
			DWORD dwDownloaded = 0;

			if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
			{
				responseData.append(buffer.data(), dwDownloaded);
			}
			else
			{
				std::cerr << "[GeminiClient] WinHttpReadData に失敗しました。Code: "
					<< GetLastError() << std::endl;
				break;
			}

		} while (dwSize > 0);

		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);

		// レスポンスが空だった場合の処理
		if (responseData.empty())
		{
			return "[GeminiClient] エラー: 空のレスポンスを受け取りました。";
		}

		// レスポンス JSON の解析とテキストの抽出
		try
		{
			const nlohmann::json parsed = nlohmann::json::parse(responseData);

			// API 側からエラーメッセージが返ってきているかチェック
			if (parsed.contains("error"))
			{
				const std::string errorMsg = parsed["error"]["message"].get<std::string>();
				const int         errorCode = parsed["error"]["code"].get<int>();
				return "[GeminiClient] API エラー: " + errorMsg
					+ " (Code: " + std::to_string(errorCode) + ")";
			}

			// 正常なテキスト回答の抽出
			if (parsed.contains("candidates") &&
				parsed["candidates"].is_array() &&
				!parsed["candidates"].empty())
			{
				const auto& firstCandidate = parsed["candidates"][0];
				if (firstCandidate.contains("content") &&
					firstCandidate["content"].contains("parts"))
				{
					const auto& parts = firstCandidate["content"]["parts"];
					if (parts.is_array() && !parts.empty() && parts[0].contains("text"))
					{
						return parts[0]["text"].get<std::string>();
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			return "[GeminiClient] JSON パースエラー: " + std::string(e.what());
		}

		return "[GeminiClient] エラー: 有効なテキストが見つかりませんでした。";
	}


} // namespace app