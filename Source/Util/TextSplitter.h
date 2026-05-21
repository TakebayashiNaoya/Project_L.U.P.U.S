/**
 * @file TextSplitter.h
 * @brief テキストチャンク分割ユーティリティ
 */
#pragma once
#include <string>
#include <vector>


namespace app
{


	/**
	 * @brief テキストを音声合成に適したチャンクに分割するユーティリティクラス
	 * @details 日本語の句読点・記号・改行をデリミタとして分割する。
	 *          デリミタ自体はチャンク末尾に残すため、音声合成時のイントネーションが保たれる。
	 */
	class TextSplitter
	{
	public:
		/**
		 * @brief テキストを句読点・改行で分割してチャンクのリストを返す
		 * @param text 分割対象のテキスト文字列
		 * @return 分割後のチャンクのリスト。空文字・空白のみのチャンクは除外する。
		 *         デリミタが一切含まれない場合は元のテキストを1件として返す。
		 * @details デリミタ: 句点(。)・感嘆符(！)・疑問符(？)・改行(\n)
		 *          デリミタ自体は分割後のチャンク末尾に残す。
		 */
		static std::vector<std::string> SplitIntoChunks(const std::string& text);


	private:
		TextSplitter() = delete;
	};


} // namespace app
