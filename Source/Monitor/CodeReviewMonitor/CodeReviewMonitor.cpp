/**
 * @file CodeReviewMonitor.cpp
 * @brief C++ ソースファイルの命名規則違反を検知する監視モジュール実装
 */
#include "stdafx.h"
#include "CodeReviewMonitor.h"

#include <fstream>
#include <sstream>


namespace app
{
	CodeReviewMonitor::CodeReviewMonitor(const nlohmann::json& profile)
	{
		LoadDirectories(profile);
		LoadRules(profile);

		std::cout << "[CodeReviewMonitor] 監視ディレクトリ: " << m_targetDirectories.size()
			<< " 件、ルール: " << m_rules.size() << " 件を読み込みました" << std::endl;
	}


	void CodeReviewMonitor::Observe(SystemContext& context)
	{
		std::vector<std::string> newWarnings;

		for (const auto& dir : m_targetDirectories)
		{
			if (!std::filesystem::exists(dir))
			{
				continue;
			}

			// ディレクトリを再帰走査し、.cpp / .h ファイルを対象にする
			for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
			{
				if (!entry.is_regular_file())
				{
					continue;
				}

				const std::filesystem::path& filePath = entry.path();
				const std::string ext = filePath.extension().string();
				if (ext != ".cpp" && ext != ".h")
				{
					continue;
				}

				// last_write_time を取得し、キャッシュと比較して差分のみスキャンする
				const std::filesystem::file_time_type lastWriteTime = entry.last_write_time();
				const std::string key = filePath.string();

				const auto it = m_timestampCache.find(key);
				if (it != m_timestampCache.end() && it->second == lastWriteTime)
				{
					// タイムスタンプが変わっていないためスキップする
					continue;
				}

				// タイムスタンプを更新してからスキャンを実行する
				m_timestampCache[key] = lastWriteTime;
				const std::vector<std::string> violations = ScanFile(filePath);

				for (const auto& violation : violations)
				{
					newWarnings.push_back(violation);
				}
			}
		}

		// 結果を SystemContext にスレッドセーフに書き込む
		{
			std::lock_guard<std::mutex> lock(context.m_warningsMutex);

			// 現フェーズでは CodeReviewMonitor の結果で警告リストを全クリアして書き直す
			context.m_instantWarnings.clear();

			for (const auto& w : newWarnings)
			{
				context.m_instantWarnings.push_back(w);
			}
		}

		const bool isHasViolations = !newWarnings.empty();
		context.m_hasCodeViolations = isHasViolations;

		if (isHasViolations)
		{
			std::cout << "[CodeReviewMonitor] 命名規則違反を " << newWarnings.size()
				<< " 件検出しました" << std::endl;
		}
	}


	const char* CodeReviewMonitor::GetName() const
	{
		return "CodeReviewMonitor";
	}


	std::vector<std::string> CodeReviewMonitor::ScanFile(const std::filesystem::path& filePath) const
	{
		std::vector<std::string> violations;

		std::ifstream ifs(filePath);
		if (!ifs.is_open())
		{
			std::cerr << "[CodeReviewMonitor] ファイルを開けませんでした: "
				<< filePath.string() << std::endl;
			return violations;
		}

		int lineNumber = 0;
		std::string line;

		while (std::getline(ifs, line))
		{
			++lineNumber;

			for (const auto& rule : m_rules)
			{
				if (std::regex_search(line, rule.m_pattern))
				{
					// 違反メッセージを "ファイル名(行番号): ルール名 - 説明" の形式で生成する
					std::ostringstream oss;
					oss << filePath.filename().string()
						<< "(" << lineNumber << ")"
						<< " [" << rule.m_name << "] "
						<< rule.m_message
						<< " => " << line;
					violations.push_back(oss.str());
				}
			}
		}

		return violations;
	}


	void CodeReviewMonitor::LoadRules(const nlohmann::json& profile)
	{
		if (!profile.contains("code_review_rules") || !profile["code_review_rules"].is_array())
		{
			std::cout << std::string("[CodeReviewMonitor] プロファイルに code_review_rules が見つかりません。")
				+ "デフォルトルールを使用します" << std::endl;
			return;
		}

		for (const auto& ruleJson : profile["code_review_rules"])
		{
			if (!ruleJson.contains("name") || !ruleJson.contains("pattern") || !ruleJson.contains("message"))
			{
				continue;
			}

			CodeReviewRule rule;
			rule.m_name = ruleJson["name"].get<std::string>();
			rule.m_message = ruleJson["message"].get<std::string>();

			try
			{
				rule.m_pattern = std::regex(
					ruleJson["pattern"].get<std::string>(),
					std::regex::ECMAScript
				);
				m_rules.push_back(std::move(rule));
			}
			catch (const std::regex_error& e)
			{
				std::cerr << "[CodeReviewMonitor] ルール \"" << rule.m_name
					<< "\" の正規表現が不正です: " << e.what() << std::endl;
			}
		}
	}


	void CodeReviewMonitor::LoadDirectories(const nlohmann::json& profile)
	{
		if (!profile.contains("code_review_directories") || !profile["code_review_directories"].is_array())
		{
			std::cout << std::string("[CodeReviewMonitor] プロファイルに code_review_directories が見つかりません。")
				+ "スキャンを省略します" << std::endl;
			return;
		}

		for (const auto& dir : profile["code_review_directories"])
		{
			if (dir.is_string())
			{
				m_targetDirectories.emplace_back(dir.get<std::string>());
			}
		}
	}


} // namespace app