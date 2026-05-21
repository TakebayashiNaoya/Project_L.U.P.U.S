/**
 * @file IDocumentExtractor.h
 * @brief ドキュメントからテキストを抽出するStrategyインターフェースおよびMarkdown実装の宣言
 */
#pragma once
#include <filesystem>
#include <string>


namespace app
{


	/**
	 * @brief 各ドキュメント形式からテキストを抽出するStrategyインターフェース
	 * @details DocumentReviewMonitor は拡張子に応じて IDocumentExtractor の実装に処理を委譲する。
	 *          新しい形式(.docx / .xlsx 等)を追加する場合はこのインターフェースを実装すること。
	 */
	class IDocumentExtractor
	{
	public:
		/** @brief デストラクタ */
		virtual ~IDocumentExtractor() = default;

		/**
		 * @brief 指定されたファイルからテキストを抽出して返す
		 * @param filePath 抽出対象のファイルパス(Tempコピー済みのパスが渡される)
		 * @return 抽出されたプレーンテキスト文字列。失敗時は空文字列
		 */
		virtual std::string Extract(const std::filesystem::path& filePath) const = 0;

		/**
		 * @brief このExtractorが指定ファイルを処理できるかどうかを返す
		 * @param filePath 判定対象のファイルパス
		 * @return 処理可能な場合 true
		 */
		virtual bool CanHandle(const std::filesystem::path& filePath) const = 0;
	};


	/***************************************/


	/**
	 * @brief .md および .txt ファイルを対象とするテキスト抽出実装
	 * @details std::ifstream でファイルをそのまま読み込み、全文字列を返す。
	 *          画像参照等のバイナリ以外の表現はそのまま保持する。
	 */
	class MarkdownExtractor : public IDocumentExtractor
	{
	public:
		/**
		 * @brief .md / .txt ファイルを開いてテキストを全文抽出する
		 * @param filePath 抽出対象のファイルパス
		 * @return ファイルの全文字列。読み込み失敗時は空文字列
		 */
		std::string Extract(const std::filesystem::path& filePath) const override;

		/**
		 * @brief 拡張子が .md または .txt の場合に処理可能と判定する
		 * @param filePath 判定対象のファイルパス
		 * @return .md / .txt であれば true
		 */
		bool CanHandle(const std::filesystem::path& filePath) const override;
	};


} // namespace app