/**
 * @file ProfileManager.h
 * @brief 設定ファイルの読み込みと統合を行う中央管理クラス
 */
#pragma once
#include <filesystem>
#include <string>
#include "external/json.hpp"


namespace app
{


	/**
	 * @brief config/ をベースとし、private/ の同名ファイルで上書きする中央管理クラス
	 * @details 読み込みルールは以下の通り。
	 *
	 *          JSON(短いパラメータのみ):
	 *            config/system.json   <- private/system.json   (あれば同名キーを上書き)
	 *            config/rules.json    <- private/rules.json    (あれば同名キーを上書き)
	 *            config/persona.json  <- private/persona.json  (あれば同名キーを上書き)
	 *            private/secrets.json (private/ のみ。なければ空として扱う)
	 *
	 *          Markdown(システムプロンプト):
	 *            config/profile.md    <- private/profile.md    (あれば全文上書き)
	 *
	 *          読み込み後、m_systemPrompt 内の ${キー名} プレースホルダーを
	 *          統合済み JSON の値で置換する。
	 */
	class ProfileManager
	{
	public:
		/**
		 * @brief 全設定ファイルを読み込み、統合プロファイルを構築する
		 * @return config/ の必須ファイル読み込みに成功した場合 true
		 */
		bool Load();

		/**
		 * @brief 統合済みのプロファイルデータを取得する
		 * @return 統合済み nlohmann::json オブジェクトへの定数参照
		 */
		const nlohmann::json& GetProfile() const;

		/**
		 * @brief プレースホルダー置換済みのシステムプロンプト文字列を取得する
		 * @return システムプロンプトのプレーンテキスト文字列
		 */
		const std::string& GetSystemPrompt() const;

		/**
		 * @brief アシスタント名を取得する
		 * @details persona.json の "assistant_name" キーの値を返す。
		 *          未設定の場合は "L.U.P.U.S." を返す。
		 * @return アシスタント名文字列
		 */
		std::string GetAssistantName() const;

		/**
		 * @brief Level 1 即時警告メッセージを取得する
		 * @details persona.json の "instant_warning_message" キーの値を返す。
		 *          未設定の場合はデフォルト文言を返す。
		 * @return 即時警告メッセージ文字列
		 */
		std::string GetInstantWarningMessage() const;

		/**
		 * @brief Standby 状態のメッセージを取得する
		 * @details persona.json の "standby_message" キーの値を返す。
		 *          未設定の場合はデフォルト文言を返す。
		 * @return Standby メッセージ文字列
		 */
		std::string GetStandbyMessage() const;

		/**
		 * @brief TaskCompleted 状態のメッセージを取得する
		 * @details persona.json の "completion_message" キーの値を返す。
		 *          未設定の場合はデフォルト文言を返す。
		 * @return TaskCompleted メッセージ文字列
		 */
		std::string GetCompletionMessage() const;

		/**
		 * @brief プロファイルが正常に読み込まれているか確認する
		 * @return 読み込み済みの場合 true
		 */
		bool IsLoaded() const;


	private:
		/**
		 * @brief JSON ファイルをベース+オーバーライドの順でマージして読み込む
		 * @param basePath     config/ 側の必須ファイルパス
		 * @param overridePath private/ 側の任意ファイルパス
		 * @param outJson      マージ結果の出力先
		 * @return basePath の読み込みに成功した場合 true
		 */
		bool LoadJsonWithOverride(
			const std::filesystem::path& basePath,
			const std::filesystem::path& overridePath,
			nlohmann::json& outJson
		) const;

		/**
		 * @brief JSON ファイルをオーバーライド専用として読み込む(ベースなし・任意)
		 * @param overridePath private/ 側の任意ファイルパス
		 * @param outJson      読み込み結果の出力先(不在時は空オブジェクト)
		 */
		void LoadJsonOptional(
			const std::filesystem::path& overridePath,
			nlohmann::json& outJson
		) const;

		/**
		 * @brief Markdown ファイルをベース+オーバーライドの順で読み込む
		 * @param basePath     config/ 側のファイルパス
		 * @param overridePath private/ 側のファイルパス
		 * @param outText      読み込み結果の出力先
		 */
		void LoadMarkdownWithOverride(
			const std::filesystem::path& basePath,
			const std::filesystem::path& overridePath,
			std::string& outText
		) const;

		/**
		 * @brief m_systemPrompt 内の ${キー名} プレースホルダーを統合済み JSON の値で置換する
		 * @details JSON の値が文字列型のキーのみを対象とする。
		 */
		void ReplacePlaceholders();

		/**
		 * @brief 指定パスから JSON ファイルを読み込む低レベル関数
		 * @param filePath 読み込むファイルパス
		 * @param outJson  読み込み結果の出力先
		 * @return 読み込みとパースに成功した場合 true
		 */
		bool ReadJsonFile(
			const std::filesystem::path& filePath,
			nlohmann::json& outJson
		) const;

		/**
		 * @brief 指定パスからテキストファイルを読み込む低レベル関数
		 * @param filePath 読み込むファイルパス
		 * @param outText  読み込み結果の出力先
		 * @return 読み込みに成功した場合 true
		 */
		bool ReadTextFile(
			const std::filesystem::path& filePath,
			std::string& outText
		) const;

		/**
		 * @brief src のすべてのキーを dst にマージする(同名キーは src が勝つ)
		 * @details 両方がオブジェクト型のキーは再帰的にディープマージする。
		 * @param dst マージ先の JSON オブジェクト
		 * @param src マージ元の JSON オブジェクト
		 */
		void MergeJson(nlohmann::json& dst, const nlohmann::json& src) const;


	private:
		/** config/ ディレクトリのパス */
		static constexpr const char* DIR_CONFIG = "config";
		/** private/ ディレクトリのパス */
		static constexpr const char* DIR_PRIVATE = "private";

		/** 統合済みプロファイルデータ */
		nlohmann::json m_profile;
		/** プレースホルダー置換済みのシステムプロンプト */
		std::string m_systemPrompt;
		/** プロファイルの読み込み状態 */
		bool m_isLoaded = false;
	};


} // namespace app