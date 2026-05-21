/**
 * @file DocumentReviewMonitor.h
 * @brief ドキュメントファイルを差分スキャンしテキストを抽出する監視モジュール
 */
#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Source/Monitor/IMonitor.h"
#include "Source/Monitor/DocumentReviewMonitor/IDocumentExtractor.h"
#include "external/json.hpp"


namespace app
{


	/**
	 * @brief .md / .txt / .docx / .xlsx を対象とした差分スキャン監視モジュール
	 * @details std::filesystem::last_write_time によるキャッシュで変更ファイルのみを処理する。
	 *          Word / Excel の排他ロックを回避するため Temp コピー機構を使用する。
	 *          抽出テキストは m_deepReviewResult に格納され、Level 2（LLM 要求時）に使用される。
	 *          Level 1 には変更ファイルの検出通知のみを m_instantWarnings に書き込む。
	 */
	class DocumentReviewMonitor : public IMonitor
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		explicit DocumentReviewMonitor(const nlohmann::json& profile);

		/**
		 * @brief 監視対象ディレクトリを差分スキャンし、変更ドキュメントのテキストを抽出する
		 * @param context 更新対象の共有コンテキスト
		 */
		void Observe(SystemContext& context) override;

		/**
		 * @brief モジュール名を返す
		 * @return "DocumentReviewMonitor"
		 */
		const char* GetName() const override;


	private:
		/**
		 * @brief 排他ロックを回避するため、対象ファイルを Temp ディレクトリにコピーする
		 * @param sourcePath コピー元のファイルパス
		 * @return コピー先の一時ファイルパス。失敗時は空のパス
		 */
		std::filesystem::path CopyToTemp(const std::filesystem::path& sourcePath) const;

		/**
		 * @brief 対象ファイルを処理可能な IDocumentExtractor を検索して返す
		 * @param filePath 対象のファイルパス
		 * @return 処理可能な Extractor のポインタ。見つからない場合は nullptr
		 */
		const IDocumentExtractor* FindExtractor(const std::filesystem::path& filePath) const;

		/**
		 * @brief プロファイルの "document_review_directories" 配列から監視対象ディレクトリを読み込む
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		void LoadDirectories(const nlohmann::json& profile);


	private:
		/** 監視対象のディレクトリパスリスト */
		std::vector<std::filesystem::path> m_targetDirectories;
		/** Temp コピー先ディレクトリのパス */
		std::filesystem::path m_tempDirectory;
		/** Strategyパターンの Extractor リスト */
		std::vector<std::unique_ptr<IDocumentExtractor>> m_extractors;
		/** ファイルパス → 前回スキャン時の last_write_time のキャッシュ */
		std::unordered_map<std::string, std::filesystem::file_time_type> m_timestampCache;
	};


} // namespace app
