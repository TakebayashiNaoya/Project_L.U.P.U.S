/**
 * @file ProfileLoader.h
 * @brief システムプロファイルの読み込みクラス
 */
#pragma once
#include "stdafx.h"


namespace app
{


	/**
	 * @brief システムプロファイルをJSONファイルから読み込むクラス
	 * @details private/custom_profile.json を優先し、
	 *          存在しない場合は config/default_profile.json にフォールバックする
	 */
	class ProfileLoader
	{
	public:
		/**
		 * @brief プロファイルを読み込む
		 * @return 読み込みに成功した場合 true
		 */
		bool Load();

		/**
		 * @brief 読み込んだプロファイルデータを取得する
		 * @return nlohmann::json オブジェクトへの参照
		 */
		const nlohmann::json& GetProfile() const;

		/**
		 * @brief プロファイルが正常に読み込まれているか確認する
		 * @return 読み込み済みの場合 true
		 */
		bool IsLoaded() const;


	private:
		/**
		 * @brief 指定されたパスからJSONファイルを読み込む
		 * @param filePath 読み込むJSONファイルのパス
		 * @return 読み込みに成功した場合 true
		 */
		bool LoadFromFile(const std::filesystem::path& filePath);


	private:
		/** カスタムプロファイルのパス */
		static constexpr const char* CUSTOM_PROFILE_PATH = "private/custom_profile.json";
		/** デフォルトプロファイルのパス */
		static constexpr const char* DEFAULT_PROFILE_PATH = "config/default_profile.json";
		/** 読み込んだプロファイルデータ */
		nlohmann::json m_profile;
		/** プロファイルの読み込み状態 */
		bool m_isLoaded = false;
	};


} // namespace app
