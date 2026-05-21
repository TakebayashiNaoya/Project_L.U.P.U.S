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
		// 差分スキャン: 変更があったファイルのみ m_violationCache を更新する
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
				if (ext != ".cpp" && ext != ".h" && ext != ".hpp")
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

				const std::string key = PathToUtf8(filePath);
				const auto it = m_timestampCache.find(key);
				if (it != m_timestampCache.end() && it->second == lastWriteTime)
				{
					// 変更なし: このファイルの m_violationCache はそのまま保持する
					continue;
				}

				// 変更あり: タイムスタンプを更新してスキャンし、キャッシュを上書きする
				m_timestampCache[key] = lastWriteTime;
				m_violationCache[key] = ScanFile(filePath);
			}
		}

		// m_violationCache の全エントリを結合して最新の全件リストを組み立てる
		std::vector<std::string> allViolations;
		for (const auto& [key, violations] : m_violationCache)
		{
			allViolations.insert(allViolations.end(), violations.begin(), violations.end());
		}

		// 件数の変化があった場合のみログを出力する
		const bool hasViolations = !allViolations.empty();
		if (hasViolations != static_cast<bool>(context.m_hasCodeViolations))
		{
			if (hasViolations)
			{
				std::cout << "[CodeReviewMonitor] 命名規則違反を " << allViolations.size()
					<< " 件検出しました" << std::endl;
			}
			else
			{
				std::cout << "[CodeReviewMonitor] 命名規則違反がなくなりました" << std::endl;
			}
		}

		// 最新の全件リストを context に書き込む
		{
			std::lock_guard<std::mutex> lock(context.m_codeViolationsMutex);
			context.m_codeViolations = allViolations;
		}
		context.m_hasCodeViolations = hasViolations;
	}


	const char* CodeReviewMonitor::GetName() const
	{
		return "CodeReviewMonitor";
	}


	std::vector<std::string> CodeReviewMonitor::ScanFile(const std::filesystem::path& filePath) const
	{
		std::vector<std::string> violations;
		const std::string ext = filePath.extension().string();

		// 現在のファイル拡張子に適用すべきルールだけを activeRules に抽出する
		std::vector<const CodeReviewRule*> activeRules;
		for (const auto& rule : m_rules)
		{
			if (rule.m_extensions.empty())
			{
				activeRules.push_back(&rule);
			}
			else
			{
				for (const auto& targetExt : rule.m_extensions)
				{
					if (ext == targetExt)
					{
						activeRules.push_back(&rule);
						break;
					}
				}
			}
		}

		// 適用するルールが1つも無ければ、ファイルを開かずに終了（最適化）
		if (activeRules.empty()) return violations;

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

			if (ruleJson.contains("extensions") && ruleJson["extensions"].is_array())
			{
				for (const auto& extNode : ruleJson["extensions"])
				{
					if (extNode.is_string())
					{
						rule.m_extensions.push_back(extNode.get<std::string>());
					}
				}
			}

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