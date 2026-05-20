/**
 * @file ProfileLoader.cpp
 * @brief システムプロファイルの読み込みクラス実装
 */
#include "stdafx.h"
#include "ProfileLoader.h"
#include <fstream>


namespace app
{


	bool ProfileLoader::Load()
	{
		// カスタムプロファイルを優先して読み込む
		const std::filesystem::path customPath(CUSTOM_PROFILE_PATH);
		if (std::filesystem::exists(customPath))
		{
			std::cout << "[ProfileLoader] カスタムプロファイルを読み込みます: "
				<< customPath << std::endl;
			return LoadFromFile(customPath);
		}

		// カスタムプロファイルが存在しない場合はデフォルトにフォールバック
		const std::filesystem::path defaultPath(DEFAULT_PROFILE_PATH);
		std::cout << "[ProfileLoader] デフォルトプロファイルを読み込みます: "
			<< defaultPath << std::endl;
		return LoadFromFile(defaultPath);
	}


	bool ProfileLoader::LoadFromFile(const std::filesystem::path& filePath)
	{
		if (!std::filesystem::exists(filePath))
		{
			std::cerr << "[ProfileLoader] ファイルが見つかりません: " << filePath << std::endl;
			return false;
		}

		std::ifstream file(filePath);
		if (!file.is_open())
		{
			std::cerr << "[ProfileLoader] ファイルを開けません: " << filePath << std::endl;
			return false;
		}

		try
		{
			file >> m_profile;
			m_isLoaded = true;
			std::cout << "[ProfileLoader] 読み込み完了: "
				<< m_profile.value("assistant_name", "unknown") << std::endl;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::cerr << "[ProfileLoader] JSONパースエラー: " << e.what() << std::endl;
			return false;
		}

		return true;
	}


	const nlohmann::json& ProfileLoader::GetProfile() const
	{
		return m_profile;
	}


	bool ProfileLoader::IsLoaded() const
	{
		return m_isLoaded;
	}


} // namespace app
