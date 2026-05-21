/**
 * @file GeminiClient.cpp
 * @brief Gemini API クライアントクラス実装
 */
#include "stdafx.h"
#include <iostream>
#include <sstream>
#include "GeminiClient.h"
#include "external/json.hpp"

#pragma comment(lib, "winhttp.lib")


namespace app
{


	namespace
	{
		DWORD RESOLVE_TIMEOUT_MS = 10000;	// DNS解決タイムアウト
		DWORD CONNECT_TIMEOUT_MS = 10000;	// 接続タイムアウト
		DWORD SEND_TIMEOUT_MS = 10000;		// 送信タイムアウト
		DWORD RECEIVE_TIMEOUT_MS = 60000;	// 受信タイムアウト
	}


	GeminiClient::GeminiClient(const std::string& apiKey)
		: m_apiKey(apiKey)
	{}


	std::string GeminiClient::GenerateResponse(const std::string& prompt)
	{
		// Gemini API のエンドポイント
		std::wstring serverName = L"generativelanguage.googleapis.com";
		std::wstring path = L"/v1beta/models/gemini-3.5-flash:generateContent?key=" +
			std::wstring(m_apiKey.begin(), m_apiKey.end());

		// 送信する JSON ペイロードの構築
		nlohmann::json requestJson = {
			{"contents", {
				{{ "parts", {{ {"text", prompt} }} }}
			}}
		};
		std::string bodyString = requestJson.dump();

		std::string responseData;

		// 【学校LAN・プロキシ環境対応】
		// 第2引数を WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY にするか、
		// 制限を回避するためにプロキシ設定をIEやWindowsのシステム設定から自動追従させます。
		HINTERNET hSession = WinHttpOpen(
			L"LUPUS/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, // システムのプロキシ設定に自動追従
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0
		);
		if (!hSession)
		{
			return "[GeminiClient] エラー: WinHttpOpen に失敗しました。Code: " + std::to_string(GetLastError());
		}

		// タイムアウト設定
		WinHttpSetTimeouts(
			hSession,
			RESOLVE_TIMEOUT_MS,
			CONNECT_TIMEOUT_MS,
			SEND_TIMEOUT_MS,
			RECEIVE_TIMEOUT_MS
		);

		// サーバー接続
		HINTERNET hConnect = WinHttpConnect(hSession, serverName.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
		if (!hConnect)
		{
			DWORD err = GetLastError();
			WinHttpCloseHandle(hSession);
			return "[GeminiClient] エラー: WinHttpConnect に失敗しました。Code: " + std::to_string(err);
		}

		// リクエストハンドルのオープン (安全なHTTPS通信フラグを明示)
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
			DWORD err = GetLastError();
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return "[GeminiClient] エラー: WinHttpOpenRequest に失敗しました。Code: " + std::to_string(err);
		}

		// ヘッダー情報の追加
		std::wstring headers = L"Content-Type: application/json\r\n";

		// リクエストの送信
		BOOL isSuccess = WinHttpSendRequest(
			hRequest,
			headers.c_str(),
			static_cast<DWORD>(headers.length()),
			const_cast<char*>(bodyString.c_str()),
			static_cast<DWORD>(bodyString.length()),
			static_cast<DWORD>(bodyString.length()),
			0
		);

		if (!isSuccess)
		{
			std::cerr << "[GeminiClient] WinHttpSendRequest failed. Code: " << GetLastError() << std::endl;
		}
		else
		{
			// レスポンスの受信待ち
			isSuccess = WinHttpReceiveResponse(hRequest, nullptr);
			if (!isSuccess)
			{
				std::cerr << "[GeminiClient] WinHttpReceiveResponse failed. Code: " << GetLastError() << std::endl;
			}
		}

		// データの読み込み
		if (isSuccess)
		{
			DWORD dwSize = 0;
			do
			{
				if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
				{
					std::cerr << "[GeminiClient] WinHttpQueryDataAvailable failed. Code: " << GetLastError() << std::endl;
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
					std::cerr << "[GeminiClient] WinHttpReadData failed. Code: " << GetLastError() << std::endl;
					break;
				}

			} while (dwSize > 0);
		}

		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);

		// レスポンスが空だった場合の処理
		if (responseData.empty())
		{
			return "[GeminiClient] Error: 空のレスポンスを受け取りました。";
		}

		// レスポンス JSON の解析とテキストの抽出
		try
		{
			nlohmann::json parsed = nlohmann::json::parse(responseData);

			// API側からエラーメッセージが返ってきているかチェック (キーが間違っている、制限超過など)
			if (parsed.contains("error"))
			{
				std::string errorMsg = parsed["error"]["message"].get<std::string>();
				return "[GeminiClient] APIエラー: " + errorMsg + " (Code: " + std::to_string(parsed["error"]["code"].get<int>()) + ")";
			}

			// 正常なテキスト回答の抽出
			if (parsed.contains("candidates") && parsed["candidates"].is_array() && !parsed["candidates"].empty())
			{
				auto& firstCandidate = parsed["candidates"][0];
				if (firstCandidate.contains("content") && firstCandidate["content"].contains("parts"))
				{
					auto& parts = firstCandidate["content"]["parts"];
					if (parts.is_array() && !parts.empty() && parts[0].contains("text"))
					{
						return parts[0]["text"].get<std::string>();
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			return "[GeminiClient] JSON Parse Exception: " + std::string(e.what());
		}

		return "[GeminiClient] Error: 有効なテキストが見つかりませんでした。";
	}


}// namespace app