/**
 * @file GeminiClient.h
 * @brief Gemini API を呼び出す LLM クライアントの実装
 */
#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>
#include "ILLMClient.h"


namespace app
{


	/**
	 * @brief WinHTTP を使用して Gemini API を呼び出す ILLMClient 実装
	 * @details GenerateResponse() は同期的にブロッキングする。
	 *          非同期化は呼び出し元(StateTaskFocus)が std::async で管理する。
	 */
	class GeminiClient : public ILLMClient
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param apiKey Gemini API キー文字列。空の場合は GenerateResponse() でエラーを返す。
		 */
		explicit GeminiClient(const std::string& apiKey);

		/** @brief デストラクタ */
		~GeminiClient() override = default;

		/**
		 * @brief Gemini API を呼び出してプロンプトに対する返答を取得する
		 * @param prompt LLM に送信するプロンプト文字列
		 * @return Gemini API からの返答文字列。エラー時はエラー内容の文字列を返す。
		 */
		std::string GenerateResponse(const std::string& prompt) override;


	private:
		/** Gemini API キー文字列 */
		std::string m_apiKey;
	};


} // namespace app