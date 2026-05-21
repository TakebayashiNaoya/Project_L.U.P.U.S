/**
 * @file ProfileManager.cpp
 * @brief 設定ファイルの読み込みと統合を行う中央管理クラス実装
 */
#include "stdafx.h"
#include "ProfileManager.h"

#include <fstream>
#include <sstream>


namespace app
{
	bool ProfileManager::Load()
	{
		m_profile = nlohmann::json::object();
		m_systemPrompt.clear();
		m_isLoaded = false;

		const std::filesystem::path configDir(DIR_CONFIG);
		const std::filesystem::path privateDir(DIR_PRIVATE);

		// ---------------------------------------------------------------
		// JSON ファイルの読み込み
		// config/ をベースとし、private/ の同名ファイルで同名キーを上書きする。
		// secrets は private/ のみから任意で読み込む。
		// ---------------------------------------------------------------

		nlohmann::json systemJson;
		if (!LoadJsonWithOverride(configDir / "system.json", privateDir / "system.json", systemJson))
		{
			return false;
		}

		nlohmann::json rulesJson;
		if (!LoadJsonWithOverride(configDir / "rules.json", privateDir / "rules.json", rulesJson))
		{
			return false;
		}

		nlohmann::json personaJson;
		if (!LoadJsonWithOverride(configDir / "persona.json", privateDir / "persona.json", personaJson))
		{
			return false;
		}

		nlohmann::json secretsJson;
		LoadJsonOptional(privateDir / "secrets.json", secretsJson);

		MergeJson(m_profile, systemJson);
		MergeJson(m_profile, rulesJson);
		MergeJson(m_profile, personaJson);
		MergeJson(m_profile, secretsJson);

		// ---------------------------------------------------------------
		// Markdown の読み込み
		// config/profile.md をベースとし、private/profile.md があれば全文上書きする。
		// ---------------------------------------------------------------

		LoadMarkdownWithOverride(configDir / "profile.md", privateDir / "profile.md", m_systemPrompt);

		// ---------------------------------------------------------------
		// プレースホルダーの置換
		// m_systemPrompt 内の ${キー名} を統合済み JSON の値で置換する。
		// ---------------------------------------------------------------

		ReplacePlaceholders();

		m_isLoaded = true;

		const std::string name = GetAssistantName();
		std::cout << "[ProfileManager] 統合完了: " << name << std::endl;

		return true;
	}


	const nlohmann::json& ProfileManager::GetProfile() const
	{
		return m_profile;
	}


	const std::string& ProfileManager::GetSystemPrompt() const
	{
		return m_systemPrompt;
	}


	std::string ProfileManager::GetAssistantName() const
	{
		return m_profile.value("assistant_name", "L.U.P.U.S.");
	}


	std::string ProfileManager::GetInstantWarningMessage() const
	{
		return m_profile.value(
			"instant_warning_message",
			"規約違反または未完了タスクが検出されています。作業に集中してください。"
		);
	}


	std::string ProfileManager::GetStandbyMessage() const
	{
		return m_profile.value("standby_message", "外出中のため待機モードです。");
	}


	std::string ProfileManager::GetCompletionMessage() const
	{
		return m_profile.value("completion_message", "全タスクが完了しました。お疲れ様でした。");
	}


	// 末尾付近に追加
	std::string ProfileManager::GetGeminiApiKey() const
	{
		if (m_profile.contains("gemini_api_key") && m_profile["gemini_api_key"].is_string())
		{
			return m_profile["gemini_api_key"].get<std::string>();
		}
		return "";
	}


	bool ProfileManager::IsLoaded() const
	{
		return m_isLoaded;
	}


	bool ProfileManager::LoadJsonWithOverride(
		const std::filesystem::path& basePath,
		const std::filesystem::path& overridePath,
		nlohmann::json& outJson
	) const
	{
		outJson = nlohmann::json::object();

		if (!ReadJsonFile(basePath, outJson))
		{
			return false;
		}

		if (std::filesystem::exists(overridePath))
		{
			nlohmann::json overrideJson;
			if (ReadJsonFile(overridePath, overrideJson))
			{
				MergeJson(outJson, overrideJson);
			}
		}

		return true;
	}


	void ProfileManager::LoadJsonOptional(
		const std::filesystem::path& overridePath,
		nlohmann::json& outJson
	) const
	{
		outJson = nlohmann::json::object();

		if (!std::filesystem::exists(overridePath))
		{
			std::cout << "[ProfileManager] オプションファイルをスキップします: "
				<< overridePath.string() << std::endl;
			return;
		}

		ReadJsonFile(overridePath, outJson);
	}


	void ProfileManager::LoadMarkdownWithOverride(
		const std::filesystem::path& basePath,
		const std::filesystem::path& overridePath,
		std::string& outText
	) const
	{
		outText.clear();

		const bool isBaseLoaded = ReadTextFile(basePath, outText);

		if (std::filesystem::exists(overridePath))
		{
			std::string overrideText;
			if (ReadTextFile(overridePath, overrideText))
			{
				outText = std::move(overrideText);
				return;
			}
		}

		if (!isBaseLoaded)
		{
			std::cerr << "[ProfileManager] 警告: profile.md がいずれも見つかりません。空のプロンプトで起動します" << std::endl;
		}
	}


	void ProfileManager::ReplacePlaceholders()
	{
		if (m_systemPrompt.empty())
		{
			return;
		}

		for (const auto& [key, value] : m_profile.items())
		{
			if (!value.is_string())
			{
				continue;
			}

			const std::string placeholder = std::string("${") + key + "}";
			const std::string replacement = value.get<std::string>();

			std::string::size_type pos = 0;
			while ((pos = m_systemPrompt.find(placeholder, pos)) != std::string::npos)
			{
				m_systemPrompt.replace(pos, placeholder.size(), replacement);
				pos += replacement.size();
			}
		}
	}


	bool ProfileManager::ReadJsonFile(
		const std::filesystem::path& filePath,
		nlohmann::json& outJson
	) const
	{
		if (!std::filesystem::exists(filePath))
		{
			std::cerr << "[ProfileManager] 必須ファイルが見つかりません: "
				<< filePath.string() << std::endl;
			return false;
		}

		std::ifstream ifs(filePath);
		if (!ifs.is_open())
		{
			std::cerr << "[ProfileManager] ファイルを開けません: "
				<< filePath.string() << std::endl;
			return false;
		}

		try
		{
			ifs >> outJson;
			std::cout << "[ProfileManager] 読み込み完了: " << filePath.string() << std::endl;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::cerr << "[ProfileManager] JSON パースエラー (" << filePath.string()
				<< "): " << e.what() << std::endl;
			outJson = nlohmann::json::object();
			return false;
		}

		return true;
	}


	bool ProfileManager::ReadTextFile(
		const std::filesystem::path& filePath,
		std::string& outText
	) const
	{
		outText.clear();

		if (!std::filesystem::exists(filePath))
		{
			return false;
		}

		std::ifstream ifs(filePath);
		if (!ifs.is_open())
		{
			std::cerr << "[ProfileManager] テキストファイルを開けません: "
				<< filePath.string() << std::endl;
			return false;
		}

		std::ostringstream oss;
		oss << ifs.rdbuf();
		outText = oss.str();

		std::cout << "[ProfileManager] Markdown 読み込み完了: "
			<< filePath.string() << std::endl;

		return true;
	}


	void ProfileManager::MergeJson(nlohmann::json& dst, const nlohmann::json& src) const
	{
		if (!src.is_object())
		{
			return;
		}

		for (const auto& [key, value] : src.items())
		{
			if (dst.contains(key) && dst[key].is_object() && value.is_object())
			{
				MergeJson(dst[key], value);
			}
			else
			{
				dst[key] = value;
			}
		}
	}


} // namespace app