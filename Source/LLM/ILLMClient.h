/**
 * @file ILLMClient.h
 * @brief LLM クライアントのインターフェース
 */
#pragma once
#include <string>


namespace app
{


	class ILLMClient
	{
	public:
		virtual ~ILLMClient() = default;

		/**
		 * @brief プロンプトを受け取り、AIからの返答文字列を同期的に返す
		 * @param prompt LLM に送信するプロンプト文字列
		 * @return LLM からの返答文字列
		 */
		virtual std::string GenerateResponse(const std::string& prompt) = 0;
	};


}// namespace app