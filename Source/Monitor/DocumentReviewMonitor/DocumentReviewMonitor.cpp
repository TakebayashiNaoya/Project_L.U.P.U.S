/**
 * @file DocumentReviewMonitor.cpp
 * @brief ドキュメントファイルを差分スキャンしテキストを抽出する監視モジュール実装
 */
#include "stdafx.h"
#include "DocumentReviewMonitor.h"

#include <fstream>
#include <sstream>


namespace app
{
	// m_deepReviewResult の最大保持文字数。超過した場合は古いものを切り捨てる
	static constexpr std::size_t DEEP_REVIEW_MAX_CHARS = 100000;


	// ---------------------------------------------------------------
	// MarkdownExtractor の実装
	// ---------------------------------------------------------------

	std::string MarkdownExtractor::Extract(const std::filesystem::path& filePath) const
	{
		std::ifstream ifs(filePath);
		if (!ifs.is_open())
		{
			return {};
		}

		std::ostringstream oss;
		oss << ifs.rdbuf();
		return oss.str();
	}


	bool MarkdownExtractor::CanHandle(const std::filesystem::path& filePath) const
	{
		const std::string ext = filePath.extension().string();
		return (ext == ".md" || ext == ".txt");
	}


	// ---------------------------------------------------------------
	// DocumentReviewMonitor の実装
	// ---------------------------------------------------------------

	DocumentReviewMonitor::DocumentReviewMonitor(const nlohmann::json& profile)
	{
		LoadDirectories(profile);

		const std::string tempDirStr = profile.value("document_temp_directory", "private/temp");
		m_tempDirectory = std::filesystem::path(tempDirStr);

		if (!std::filesystem::exists(m_tempDirectory))
		{
			std::filesystem::create_directories(m_tempDirectory);
		}

		m_extractors.push_back(std::make_unique<MarkdownExtractor>());

		// TODO: フェーズ3以降で以下を追加する
		// m_extractors.push_back(std::make_unique<DocxExtractor>());
		// m_extractors.push_back(std::make_unique<XlsxExtractor>());

		std::cout << "[DocumentReviewMonitor] 監視ディレクトリ: " << m_targetDirectories.size()
			<< " 件を読み込みました" << std::endl;
	}


	void DocumentReviewMonitor::Observe(SystemContext& context)
	{
		// 自モジュール専用のコンテナを冒頭でクリアして最新結果で書き直す
		{
			std::lock_guard<std::mutex> lock(context.m_documentWarningsMutex);
			context.m_documentWarnings.clear();
		}

		std::ostringstream deepReviewBuffer;
		std::vector<std::string> newWarnings;
		bool isAnyFileChanged = false;

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
				std::cerr << "[DocumentReviewMonitor] ディレクトリの走査を開始できませんでした: "
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

				const IDocumentExtractor* extractor = FindExtractor(filePath);
				if (!extractor)
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
					continue;
				}

				m_timestampCache[key] = lastWriteTime;
				isAnyFileChanged = true;

				std::cout << "[DocumentReviewMonitor] 変更を検知しました: "
					<< PathToUtf8(filePath.filename()) << std::endl;

				const std::filesystem::path tempPath = CopyToTemp(filePath);
				if (tempPath.empty())
				{
					continue;
				}

				const std::string extractedText = extractor->Extract(tempPath);
				if (!extractedText.empty())
				{
					deepReviewBuffer << "=== " << PathToUtf8(filePath.filename()) << " ===\n"
						<< extractedText << "\n\n";
				}

				newWarnings.push_back(
					std::string("[DocumentReviewMonitor] ドキュメント変更を検知しました。レビューが必要です: ")
					+ PathToUtf8(filePath.filename())
				);

				// error_code 版 remove でスレッド停止を防ぐ
				std::error_code removeEc;
				std::filesystem::remove(tempPath, removeEc);
				if (removeEc)
				{
					std::cerr << "[DocumentReviewMonitor] Temp ファイルの削除に失敗しました: "
						<< PathToUtf8(tempPath) << " (" << removeEc.message() << ")" << std::endl;
				}
			}
		}

		if (!isAnyFileChanged)
		{
			return;
		}

		// Level 1 警告を専用コンテナにスレッドセーフに書き込む
		{
			std::lock_guard<std::mutex> warningLock(context.m_documentWarningsMutex);
			context.m_documentWarnings = newWarnings;
		}

		// Level 2 用テキストを上限付きで蓄積する
		{
			std::lock_guard<std::mutex> deepLock(context.m_deepReviewMutex);
			context.m_deepReviewResult += deepReviewBuffer.str();

			// 上限を超えた場合は先頭から切り捨てて最新のみ保持する
			if (context.m_deepReviewResult.size() > DEEP_REVIEW_MAX_CHARS)
			{
				context.m_deepReviewResult =
					context.m_deepReviewResult.substr(
						context.m_deepReviewResult.size() - DEEP_REVIEW_MAX_CHARS
					);
			}
		}
	}


	const char* DocumentReviewMonitor::GetName() const
	{
		return "DocumentReviewMonitor";
	}


	std::filesystem::path DocumentReviewMonitor::CopyToTemp(const std::filesystem::path& sourcePath) const
	{
		const std::filesystem::path destPath = m_tempDirectory / sourcePath.filename();

		try
		{
			std::filesystem::copy_file(
				sourcePath,
				destPath,
				std::filesystem::copy_options::overwrite_existing
			);
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			std::cerr << "[DocumentReviewMonitor] Temp コピーに失敗しました: "
				<< e.what() << std::endl;
			return {};
		}

		return destPath;
	}


	const IDocumentExtractor* DocumentReviewMonitor::FindExtractor(const std::filesystem::path& filePath) const
	{
		for (const auto& extractor : m_extractors)
		{
			if (extractor->CanHandle(filePath))
			{
				return extractor.get();
			}
		}
		return nullptr;
	}


	void DocumentReviewMonitor::LoadDirectories(const nlohmann::json& profile)
	{
		if (!profile.contains("document_review_directories") ||
			!profile["document_review_directories"].is_array())
		{
			std::cout << "[DocumentReviewMonitor] プロファイルに document_review_directories が見つかりません。スキャンを省略します" << std::endl;
			return;
		}

		for (const auto& dir : profile["document_review_directories"])
		{
			if (dir.is_string())
			{
				m_targetDirectories.emplace_back(dir.get<std::string>());
			}
		}
	}


} // namespace app