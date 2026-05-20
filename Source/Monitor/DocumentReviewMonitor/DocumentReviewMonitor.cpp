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

		// Temp ディレクトリのパスをプロファイルから読み込む(なければデフォルト値を使用)
		const std::string tempDirStr = profile.value("document_temp_directory", "private/temp");
		m_tempDirectory = std::filesystem::path(tempDirStr);

		// Temp ディレクトリが存在しない場合は作成する
		if (!std::filesystem::exists(m_tempDirectory))
		{
			std::filesystem::create_directories(m_tempDirectory);
		}

		// Strategyパターンの Extractor を登録する(拡張子の追加はここに追記する)
		m_extractors.push_back(std::make_unique<MarkdownExtractor>());

		// TODO: フェーズ3以降で以下を追加する
		// m_extractors.push_back(std::make_unique<DocxExtractor>());
		// m_extractors.push_back(std::make_unique<XlsxExtractor>());

		std::cout << "[DocumentReviewMonitor] 監視ディレクトリ: " << m_targetDirectories.size()
			<< " 件を読み込みました" << std::endl;
	}


	void DocumentReviewMonitor::Observe(SystemContext& context)
	{
		// 今回のスキャンで変更を検知したファイルの抽出テキストを蓄積するバッファ
		std::ostringstream deepReviewBuffer;
		std::vector<std::string> newWarnings;
		bool isAnyFileChanged = false;

		for (const auto& dir : m_targetDirectories)
		{
			if (!std::filesystem::exists(dir))
			{
				continue;
			}

			for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
			{
				if (!entry.is_regular_file())
				{
					continue;
				}

				const std::filesystem::path& filePath = entry.path();

				// 処理可能な Extractor が存在するファイルのみ対象にする
				const IDocumentExtractor* extractor = FindExtractor(filePath);
				if (!extractor)
				{
					continue;
				}

				// last_write_time でキャッシュを確認し、差分のみ処理する
				const std::filesystem::file_time_type lastWriteTime = entry.last_write_time();
				const std::string key = filePath.string();

				const auto it = m_timestampCache.find(key);
				if (it != m_timestampCache.end() && it->second == lastWriteTime)
				{
					// タイムスタンプが変わっていないためスキップする
					continue;
				}

				m_timestampCache[key] = lastWriteTime;
				isAnyFileChanged = true;

				std::cout << "[DocumentReviewMonitor] 変更を検知しました: "
					<< filePath.filename().string() << std::endl;

				// Temp コピー機構: Word / Excel の排他ロックを回避してから読み込む
				const std::filesystem::path tempPath = CopyToTemp(filePath);
				if (tempPath.empty())
				{
					continue;
				}

				// テキストを抽出してバッファに蓄積する
				const std::string extractedText = extractor->Extract(tempPath);

				if (!extractedText.empty())
				{
					deepReviewBuffer << "=== " << filePath.filename().string() << " ===\n"
						<< extractedText << "\n\n";
				}

				// Level 1: 変更ファイルを検知したことを即時警告として通知する
				const std::string warningMsg =
					std::string("[DocumentReviewMonitor] ドキュメント変更を検知しました。レビューが必要です: ")
					+ filePath.filename().string();
				newWarnings.push_back(warningMsg);

				// Temp ファイルを削除する
				std::filesystem::remove(tempPath);
			}
		}

		if (!isAnyFileChanged)
		{
			return;
		}

		// Level 1 警告を SystemContext にスレッドセーフに書き込む
		{
			std::lock_guard<std::mutex> warningLock(context.m_warningsMutex);
			for (const auto& w : newWarnings)
			{
				context.m_instantWarnings.push_back(w);
			}
		}

		// Level 2 用の抽出テキストを SystemContext にスレッドセーフに書き込む
		{
			std::lock_guard<std::mutex> deepLock(context.m_deepReviewMutex);
			// 前回の結果と結合して蓄積する(将来の LLM 送信時に参照される)
			context.m_deepReviewResult += deepReviewBuffer.str();
		}
	}


	const char* DocumentReviewMonitor::GetName() const
	{
		return "DocumentReviewMonitor";
	}


	std::filesystem::path DocumentReviewMonitor::CopyToTemp(const std::filesystem::path& sourcePath) const
	{
		// コピー先のパスを "元のファイル名" のまま Temp ディレクトリ配下に作成する
		const std::filesystem::path destPath = m_tempDirectory / sourcePath.filename();

		try
		{
			// 既存の Temp ファイルを上書きする
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