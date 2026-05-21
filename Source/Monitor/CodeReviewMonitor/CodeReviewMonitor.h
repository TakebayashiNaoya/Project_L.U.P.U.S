/**
 * @file CodeReviewMonitor.h
 * @brief C++ ソースファイルの命名規則違反を検知する監視モジュール
 */
#pragma once
#include <filesystem>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include "Source/Monitor/IMonitor.h"
#include "external/json.hpp"


namespace app
{


	/**
	 * @brief JSON で定義された正規表現ルールを保持する構造体
	 * @details std::regex はコピー不可のためムーブのみ許可する。
	 */
	struct CodeReviewRule
	{
		/** ルール識別名(例: "bool_prefix") */
		std::string m_name;
		/** 違反を検知する正規表現パターン */
		std::regex m_pattern;
		/** ユーザーへ表示する違反の説明メッセージ */
		std::string m_message;
		/** 追加: このルールを適用する拡張子のリスト（空なら全対象） */
		std::vector<std::string> m_extensions;

		CodeReviewRule() = default;
		CodeReviewRule(const CodeReviewRule&) = delete;
		CodeReviewRule& operator=(const CodeReviewRule&) = delete;
		CodeReviewRule(CodeReviewRule&&) = default;
		CodeReviewRule& operator=(CodeReviewRule&&) = default;
	};


	/***************************************/


	/**
	 * @brief C++ ソースファイル(.cpp / .h)を差分スキャンし、
	 *        命名規則違反を Level 1 即時警告として SystemContext に書き込む監視モジュール
	 * @details std::filesystem::last_write_time を用いたキャッシュにより、
	 *          前回スキャン以降に更新されたファイルのみを正規表現でスキャンする。
	 *          全ファイルの最新違反リストは m_violationCache に保持し、
	 *          毎 Observe() で context.m_codeViolations に全結合して書き込む。
	 *          これにより差分スキャンのタイミングに依存せず常に最新の全件リストが参照可能になる。
	 */
	class CodeReviewMonitor : public IMonitor
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		explicit CodeReviewMonitor(const nlohmann::json& profile);

		/**
		 * @brief 監視対象ディレクトリの差分スキャンを実行し、違反を context に書き込む
		 * @param context 更新対象の共有コンテキスト
		 */
		void Observe(SystemContext& context) override;

		/**
		 * @brief モジュール名を返す
		 * @return "CodeReviewMonitor"
		 */
		const char* GetName() const override;


	private:
		/**
		 * @brief 単一ファイルをスキャンし、違反メッセージのリストを返す
		 * @param filePath スキャン対象のファイルパス
		 * @return 違反メッセージ文字列のリスト。違反なしの場合は空
		 */
		std::vector<std::string> ScanFile(const std::filesystem::path& filePath) const;

		/**
		 * @brief プロファイルの "code_review_rules" 配列から違反ルールを読み込む
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		void LoadRules(const nlohmann::json& profile);

		/**
		 * @brief プロファイルの "code_review_directories" 配列から監視対象ディレクトリを読み込む
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		void LoadDirectories(const nlohmann::json& profile);


	private:
		/** 監視対象のディレクトリパスリスト */
		std::vector<std::filesystem::path> m_targetDirectories;
		/** プロファイルから読み込んだ違反検知ルールのリスト */
		std::vector<CodeReviewRule> m_rules;
		/** ファイルパス → 前回スキャン時の last_write_time のキャッシュ */
		std::unordered_map<std::string, std::filesystem::file_time_type> m_timestampCache;
		/**
		 * @brief ファイルパス → そのファイルの最新違反リストのキャッシュ
		 * @details 差分スキャンで変更があったファイルのみ更新される。
		 *          毎 Observe() でこのキャッシュを全結合して context に書き込む。
		 */
		std::unordered_map<std::string, std::vector<std::string>> m_violationCache;
	};


} // namespace app