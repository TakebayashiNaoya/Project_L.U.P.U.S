/**
 * @file TextSplitter.cpp
 * @brief テキストチャンク分割ユーティリティ実装
 */
#include "stdafx.h"
#include "TextSplitter.h"
#include <string_view>


namespace app
{
	std::vector<std::string> TextSplitter::SplitIntoChunks(const std::string& text)
	{
		std::vector<std::string> chunks;
		std::string current;

		// 日本語のマルチバイト文字（UTF-8）を考慮しながら1バイトずつ走査する。
		// 句点(。= E3 80 82)・感嘆符(！= EF BC 81)・疑問符(？= EF BC 9F) は
		// いずれも3バイトの UTF-8 シーケンスのため、先頭バイトで判定し3バイト単位で処理する。
		// 改行(\n) は1バイトのため個別に処理する。
		const std::size_t length = text.size();
		std::size_t i = 0;

		while (i < length)
		{
			const unsigned char byte = static_cast<unsigned char>(text[i]);

			// 改行(\n)はデリミタとして扱い、チャンクを確定して次へ進む
			if (byte == '\n')
			{
				current += text[i];
				++i;

				if (!current.empty())
				{
					chunks.push_back(current);
					current.clear();
				}
				continue;
			}

			// UTF-8 の3バイトシーケンス先頭(0xE0〜0xEF)の場合、3バイト取り出す
			if (byte >= 0xE0 && byte <= 0xEF && i + 2 < length)
			{
				const std::string seq = text.substr(i, 3);

				// 句点: E3 80 82
				// 感嘆符: EF BC 81
				// 疑問符: EF BC 9F
				constexpr std::string_view kKuten = "\xE3\x80\x82";
				constexpr std::string_view kExclaim = "\xEF\xBC\x81";
				constexpr std::string_view kQuestion = "\xEF\xBC\x9F";

				current += seq;
				i += 3;

				if (seq == kKuten || seq == kExclaim || seq == kQuestion)
				{
					chunks.push_back(current);
					current.clear();
				}
				continue;
			}

			// それ以外のバイト(ASCII・他のマルチバイト先頭・継続バイト)はそのまま追加する
			current += text[i];
			++i;
		}

		// ループ終了後に残ったテキストを最終チャンクとして追加する
		if (!current.empty())
		{
			chunks.push_back(current);
		}

		// 空文字・空白のみのチャンクを除外する
		std::vector<std::string> result;
		result.reserve(chunks.size());
		for (const auto& chunk : chunks)
		{
			bool isBlank = true;
			for (const char c : chunk)
			{
				if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
				{
					isBlank = false;
					break;
				}
			}
			if (!isBlank)
			{
				result.push_back(chunk);
			}
		}

		// デリミタが一切なく結果が空の場合は元のテキストを1件として返す。
		// ただし元のテキスト自体が空白のみの場合は空配列を返す。
		if (result.empty() && !text.empty())
		{
			bool isTextBlank = true;
			for (const char c : text)
			{
				if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
				{
					isTextBlank = false;
					break;
				}
			}

			if (!isTextBlank)
			{
				result.push_back(text);
			}
		}

		return result;
	}


} // namespace app