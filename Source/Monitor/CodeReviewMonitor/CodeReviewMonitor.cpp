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
		// 自モジュール専用のコンテナを冒頭でクリアして最新結果で書き直す
		{
			std::lock_guard<std::mutex> lock(context.m_codeViolationsMutex);
			context.m_codeViolations.clear();
		}

		std::vector<std::string> newViolations;

		for (const auto& dir : m_targetDirectories)
		{
			if (!std::filesystem::exists(dir))
			{
				continue;
			}

			// skip_permission_denied で権限不足のディレクトリを読み飛ばす
			std::error_code iterEc;
			std::filesystem::recursive_directory_iterator dirIter(
				dir,
				std::filesystem::directory_options::skip_permission_denied,
				iterEc
			);

			if (iterEc)
			{
				std::cerr << "[CodeReviewMonitor] ディレクトリの走査を開始できませんでした: "
					<< PathToUtf8(dir) << " (" << iterEc.message() << ")" << std::endl;
				continue;
			}

			for (const auto& entry : dirIter)
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

				// error_code 版で last_write_time を取得し、失敗したファイルはスキップする
				std::error_code timeEc;
				const std::filesystem::file_time_type lastWriteTime = entry.last_write_time(timeEc);
				if (timeEc)
				{
					continue;
				}

				// キャッシュと比較して差分のみスキャンする
				const std::string key = PathToUtf8(filePath);
				const auto it = m_timestampCache.find(key);
				if (it != m_timestampCache.end() && it->second == lastWriteTime)
				{
					continue;
				}

				m_timestampCache[key] = lastWriteTime;
				const std::vector<std::string> violations = ScanFile(filePath);

				for (const auto& violation : violations)
				{
					newViolations.push_back(violation);
				}
			}
		}

		// 結果を専用コンテナにスレッドセーフに書き込む
		{
			std::lock_guard<std::mutex> lock(context.m_codeViolationsMutex);
			context.m_codeViolations = newViolations;
		}

		const bool hasViolations = !newViolations.empty();
		context.m_hasCodeViolations = hasViolations;

		if (hasViolations)
		{
			std::cout << "[CodeReviewMonitor] 命名規則違反を " << newViolations.size()
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
				<< PathToUtf8(filePath) << std::endl;
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
					std::ostringstream oss;
					oss << PathToUtf8(filePath.filename())
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
			std::cout << "[CodeReviewMonitor] プロファイルに code_review_rules が見つかりません。デフォルトルールを使用します" << std::endl;
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
			std::cout << "[CodeReviewMonitor] プロファイルに code_review_directories が見つかりません。スキャンを省略します" << std::endl;
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