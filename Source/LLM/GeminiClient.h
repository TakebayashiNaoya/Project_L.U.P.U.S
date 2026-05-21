/**
 * @file GeminiClient.h
 * @brief Gemini API を呼び出す LLM クライアントの実装
 */
#pragma once
#include "ILLMClient.h"
#include <string>
#include <windows.h>
#include <winhttp.h>


namespace app
{


	class GeminiClient : public ILLMClient
	{
	public:
		explicit GeminiClient(const std::string& apiKey);
		~GeminiClient() override = default;

		/**
		 * @brief Gemini API を呼び出してプロンプトに対する返答を取得する
		 * @param prompt LLM に送信するプロンプト文字列
		 * @return Gemini API からの返答文字列
		 */
		std::string GenerateResponse(const std::string& prompt) override;


	private:
		/** APIキー */
		std::string m_apiKey;
	};


}// namespace app