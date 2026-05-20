/**
 * @file NotionMonitor.cpp
 * @brief Notion タスク監視モジュール実装
 */
#include "stdafx.h"
#include "NotionMonitor.h"

#pragma comment(lib, "winhttp.lib")


namespace app
{
	static const INTERNET_PORT NOTION_API_PORT = INTERNET_DEFAULT_HTTPS_PORT;


	NotionMonitor::NotionMonitor(const nlohmann::json& profile)
	{
		m_notionToken = profile.value("notion_token", "");

		if (m_notionToken.empty())
		{
			std::cerr << "[NotionMonitor] notion_token がプロファイルに設定されていません" << std::endl;
		}
		else
		{
			std::cout << "[NotionMonitor] 初期化しました。DB は初回 Observe() 時に自動検出します" << std::endl;
		}
	}


	void NotionMonitor::Observe(SystemContext& context)
	{
		if (m_notionToken.empty())
		{
			return;
		}

		// 初回呼び出し時に Auto-Discovery を実行する
		if (!m_isDiscovered)
		{
			if (!DiscoverDatabases())
			{
				return;
			}
		}

		// 全 DB の未完了タスクを収集する
		std::vector<NotionTask> allPendingTasks;
		for (const auto& databaseId : m_databaseIds)
		{
			const auto tasks = FetchPendingTasks(databaseId);
			allPendingTasks.insert(allPendingTasks.end(), tasks.begin(), tasks.end());
		}

		const bool hasPendingTasks = !allPendingTasks.empty();

		// 状態が変化したときのみログを出力する
		if (hasPendingTasks != m_hasPendingTasks)
		{
			m_hasPendingTasks = hasPendingTasks;
			if (m_hasPendingTasks)
			{
				std::cout << "[NotionMonitor] 未完了タスクを検出しました。件数: " << allPendingTasks.size() << std::endl;
			}
			else
			{
				std::cout << "[NotionMonitor] 未完了タスクがなくなりました" << std::endl;
			}
		}

		// 共有コンテキストに結果を書き込む
		context.m_hasPendingTasks = m_hasPendingTasks;
		{
			std::lock_guard<std::mutex> lock(context.m_taskMutex);
			context.m_pendingTasks = std::move(allPendingTasks);
		}
	}


	const char* NotionMonitor::GetName() const
	{
		return "NotionMonitor";
	}


	bool NotionMonitor::DiscoverDatabases()
	{
		const std::string body = "{\"filter\":{\"value\":\"database\",\"property\":\"object\"}}";
		const std::string response = SendRequest("POST", "/v1/search", body);

		if (response.empty())
		{
			std::cerr << "[NotionMonitor] DB の自動検出に失敗しました" << std::endl;
			return false;
		}

		try
		{
			const nlohmann::json parsed = nlohmann::json::parse(response);
			if (!parsed.contains("results") || !parsed["results"].is_array())
			{
				std::cerr << "[NotionMonitor] /search のレスポンス形式が不正です" << std::endl;
				return false;
			}

			m_databaseIds.clear();
			for (const auto& db : parsed["results"])
			{
				if (!db.contains("id") || !db["id"].is_string()) continue;

				const std::string dbId = db["id"].get<std::string>();
				m_databaseIds.push_back(dbId);

				// 各 DB のステータスプロパティのスキーマを取得する
				FetchDatabaseSchema(dbId);
			}

			m_isDiscovered = true;
			std::cout << "[NotionMonitor] DB を " << m_databaseIds.size() << " 件検出しました" << std::endl;
			return !m_databaseIds.empty();
		}
		catch (const nlohmann::json::exception& e)
		{
			std::cerr << "[NotionMonitor] /search レスポンスのパースに失敗しました。" << e.what() << std::endl;
			return false;
		}
	}


	void NotionMonitor::FetchDatabaseSchema(const std::string& databaseId)
	{
		const std::string response = SendRequest("GET", "/v1/databases/" + databaseId, "");
		if (response.empty()) return;

		try
		{
			const nlohmann::json parsed = nlohmann::json::parse(response);
			if (!parsed.contains("properties")) return;

			// 全プロパティから status 型を探す
			for (const auto& [key, prop] : parsed["properties"].items())
			{
				if (!prop.contains("type") || prop["type"] != "status") continue;
				if (!prop.contains("status")) continue;

				const auto& statusDef = prop["status"];
				if (!statusDef.contains("groups") || !statusDef["groups"].is_array()) continue;

				// "Complete" グループの option_ids を収集する
				std::unordered_set<std::string> completeIds;
				for (const auto& group : statusDef["groups"])
				{
					if (!group.contains("name") || group["name"] != "Complete") continue;
					if (!group.contains("option_ids") || !group["option_ids"].is_array()) continue;

					for (const auto& optId : group["option_ids"])
					{
						if (optId.is_string())
						{
							completeIds.insert(optId.get<std::string>());
						}
					}
				}

				m_completeOptionIds[databaseId] = std::move(completeIds);
				break;
			}
		}
		catch (const nlohmann::json::exception& e)
		{
			std::cerr << "[NotionMonitor] DB スキーマのパースに失敗しました。" << e.what() << std::endl;
		}
	}


	std::vector<NotionTask> NotionMonitor::FetchPendingTasks(const std::string& databaseId) const
	{
		std::vector<NotionTask> tasks;

		const std::string path = "/v1/databases/" + databaseId + "/query";
		const std::string response = SendRequest("POST", path, "{}");
		if (response.empty()) return tasks;

		try
		{
			const nlohmann::json parsed = nlohmann::json::parse(response);
			if (!parsed.contains("results") || !parsed["results"].is_array()) return tasks;

			for (const auto& page : parsed["results"])
			{
				if (IsPageIncomplete(page, databaseId))
				{
					tasks.push_back(ParseTask(page, databaseId));
				}
			}
		}
		catch (const nlohmann::json::exception& e)
		{
			std::cerr << "[NotionMonitor] DB クエリのパースに失敗しました。" << e.what() << std::endl;
		}

		return tasks;
	}


	NotionTask NotionMonitor::ParseTask(const nlohmann::json& page, const std::string& databaseId) const
	{
		NotionTask task;

		if (!page.contains("properties") || !page["properties"].is_object())
		{
			return task;
		}

		const auto& properties = page["properties"];

		for (const auto& [key, prop] : properties.items())
		{
			if (!prop.contains("type")) continue;

			const std::string type = prop["type"].get<std::string>();

			// タスク名: title 型
			if (type == "title" && prop.contains("title") && prop["title"].is_array())
			{
				for (const auto& text : prop["title"])
				{
					if (text.contains("plain_text") && text["plain_text"].is_string())
					{
						task.m_title += text["plain_text"].get<std::string>();
					}
				}
			}

			// 担当者: multi_select 型（担当者プロパティの型に合わせる）
			else if (type == "multi_select" && prop.contains("multi_select") && prop["multi_select"].is_array())
			{
				for (const auto& item : prop["multi_select"])
				{
					if (item.contains("name") && item["name"].is_string())
					{
						task.m_assignees.push_back(item["name"].get<std::string>());
					}
				}
			}

			// 担当者: people 型
			else if (type == "people" && prop.contains("people") && prop["people"].is_array())
			{
				for (const auto& person : prop["people"])
				{
					if (person.contains("name") && person["name"].is_string())
					{
						task.m_assignees.push_back(person["name"].get<std::string>());
					}
				}
			}

			// 締め切り日: date 型
			else if (type == "date" && prop.contains("date") && !prop["date"].is_null())
			{
				const auto& date = prop["date"];
				// end があれば締め切り日、なければ start を使う
				if (date.contains("end") && !date["end"].is_null())
				{
					task.m_dueDate = date["end"].get<std::string>();
				}
				else if (date.contains("start") && !date["start"].is_null())
				{
					task.m_dueDate = date["start"].get<std::string>();
				}
			}

			// ステータス: status 型
			else if (type == "status" && prop.contains("status") && !prop["status"].is_null())
			{
				const auto& statusObj = prop["status"];
				if (statusObj.contains("name") && statusObj["name"].is_string())
				{
					task.m_status = statusObj["name"].get<std::string>();
				}

				// option_id から group 名を逆引きする
				if (statusObj.contains("id") && statusObj["id"].is_string())
				{
					const std::string optionId = statusObj["id"].get<std::string>();
					const auto it = m_completeOptionIds.find(databaseId);
					if (it != m_completeOptionIds.end() && it->second.count(optionId))
					{
						task.m_group = "Complete";
					}
					else
					{
						// Complete でなければ To-do か In progress
						// 詳細なグループ分けは FetchDatabaseSchema で拡張可能
						task.m_group = "Incomplete";
					}
				}
			}
		}

		return task;
	}


	bool NotionMonitor::IsPageIncomplete(const nlohmann::json& page, const std::string& databaseId) const
	{
		if (!page.contains("properties") || !page["properties"].is_object())
		{
			return false;
		}

		const auto& properties = page["properties"];

		// 優先度1: status プロパティを探す
		for (const auto& [key, prop] : properties.items())
		{
			if (!prop.contains("type") || prop["type"] != "status") continue;

			// status の値が未設定なら未完了とみなす
			if (!prop.contains("status") || prop["status"].is_null()) return true;

			const auto& statusObj = prop["status"];
			if (!statusObj.contains("id") || !statusObj["id"].is_string()) return true;

			const std::string optionId = statusObj["id"].get<std::string>();

			// Complete グループの option_ids に含まれていなければ未完了
			const auto it = m_completeOptionIds.find(databaseId);
			if (it == m_completeOptionIds.end()) return true;

			return it->second.count(optionId) == 0;
		}

		// 優先度2: checkbox プロパティを探す
		for (const auto& [key, prop] : properties.items())
		{
			if (!prop.contains("type") || prop["type"] != "checkbox") continue;

			if (prop.contains("checkbox") && prop["checkbox"].is_boolean())
			{
				return !prop["checkbox"].get<bool>();
			}
		}

		// status も checkbox もなければ未完了とみなす
		return true;
	}


	std::string NotionMonitor::SendRequest(const std::string& method, const std::string& path, const std::string& body) const
	{
		std::string response;

		const std::wstring methodW(method.begin(), method.end());
		const std::wstring pathW(path.begin(), path.end());
		const std::wstring tokenW(m_notionToken.begin(), m_notionToken.end());
		const std::wstring authHeader = std::wstring(L"Authorization: Bearer ") + tokenW;

		HINTERNET hSession = WinHttpOpen(
			L"LUPUS/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0);
		if (!hSession)
		{
			std::cerr << "[NotionMonitor] WinHttpOpen に失敗しました。エラーコード: " << GetLastError() << std::endl;
			return response;
		}

		HINTERNET hConnect = WinHttpConnect(hSession, L"api.notion.com", NOTION_API_PORT, 0);
		if (!hConnect)
		{
			std::cerr << "[NotionMonitor] WinHttpConnect に失敗しました。エラーコード: " << GetLastError() << std::endl;
			WinHttpCloseHandle(hSession);
			return response;
		}

		HINTERNET hRequest = WinHttpOpenRequest(
			hConnect,
			methodW.c_str(),
			pathW.c_str(),
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE);
		if (!hRequest)
		{
			std::cerr << "[NotionMonitor] WinHttpOpenRequest に失敗しました。エラーコード: " << GetLastError() << std::endl;
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return response;
		}

		WinHttpAddRequestHeaders(
			hRequest,
			authHeader.c_str(),
			static_cast<DWORD>(authHeader.size()),
			WINHTTP_ADDREQ_FLAG_ADD);

		WinHttpAddRequestHeaders(
			hRequest,
			L"Notion-Version: 2022-06-28",
			static_cast<DWORD>(wcslen(L"Notion-Version: 2022-06-28")),
			WINHTTP_ADDREQ_FLAG_ADD);

		WinHttpAddRequestHeaders(
			hRequest,
			L"Content-Type: application/json",
			static_cast<DWORD>(wcslen(L"Content-Type: application/json")),
			WINHTTP_ADDREQ_FLAG_ADD);

		// GET の場合はボディなしで送信する
		const BOOL isSent = WinHttpSendRequest(
			hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			body.empty() ? nullptr : const_cast<char*>(body.c_str()),
			static_cast<DWORD>(body.size()),
			static_cast<DWORD>(body.size()),
			0);

		if (!isSent || !WinHttpReceiveResponse(hRequest, nullptr))
		{
			std::cerr << "[NotionMonitor] リクエストの送受信に失敗しました。エラーコード: " << GetLastError() << std::endl;
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return response;
		}

		DWORD dwSize = 0;
		do
		{
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
			if (dwSize == 0) break;

			std::vector<char> buffer(dwSize + 1, '\0');
			DWORD dwDownloaded = 0;
			if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
			{
				response.append(buffer.data(), dwDownloaded);
			}
		} while (dwSize > 0);

		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);

		return response;
	}


} // namespace app