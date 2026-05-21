/**
 * @file NetworkMonitor.cpp
 * @brief ネットワーク監視モジュール実装
 */
#include "stdafx.h"
#include "NetworkMonitor.h"

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")


namespace app
{
	NetworkMonitor::NetworkMonitor(const nlohmann::json& profile)
	{
		// プロファイルから自宅 SSID リストを読み込む
		if (profile.contains("home_ssids") && profile["home_ssids"].is_array())
		{
			for (const auto& ssid : profile["home_ssids"])
			{
				if (ssid.is_string())
				{
					m_homeSsids.push_back(ssid.get<std::string>());
				}
			}
		}

		std::cout << "[NetworkMonitor] 自宅 SSID を " << m_homeSsids.size() << " 件読み込みました" << std::endl;
	}


	void NetworkMonitor::Observe(SystemContext& context)
	{
		const std::string currentSsid = GetCurrentSsid();

		// SSID が取得できた場合はネットワークに接続中とみなす
		const bool isConnected = !currentSsid.empty();

		// 自宅 SSID リストと照合して在宅判定を行う
		bool isAtHome = false;
		if (isConnected && !m_homeSsids.empty())
		{
			for (const auto& homeSsid : m_homeSsids)
			{
				if (currentSsid == homeSsid)
				{
					isAtHome = true;
					break;
				}
			}
		}

		// 状態が変化したときのみログを出力する
		if (isAtHome != m_isAtHome || isConnected != m_isConnected)
		{
			m_isAtHome = isAtHome;
			m_isConnected = isConnected;

			if (!m_isConnected)
			{
				std::cout << "[NetworkMonitor] ネットワーク未接続を検出しました" << std::endl;
			}
			else if (m_isAtHome)
			{
				std::cout << "[NetworkMonitor] 在宅を検出しました。SSID: " << currentSsid << std::endl;
			}
			else
			{
				std::cout << "[NetworkMonitor] 外出先ネットワークを検出しました。SSID: " << currentSsid << std::endl;
			}
		}

		// 共有コンテキストに結果を書き込む
		context.m_isAtHome = m_isAtHome;
		context.m_isConnected = m_isConnected;

		// ---------------------------------------------------------------
		// [DEBUG] 在宅フラグ強制上書き: TaskFocus フルフロー確認用
		// 確認完了後はこのブロックを削除すること
		// ---------------------------------------------------------------
		context.m_isAtHome = true;
		context.m_isConnected = true;
		std::cout << "[NetworkMonitor][DEBUG] 在宅フラグを強制的に true に設定しました" << std::endl;
		// ---------------------------------------------------------------
	}


	const char* NetworkMonitor::GetName() const
	{
		return "NetworkMonitor";
	}


	std::string NetworkMonitor::GetCurrentSsid() const
	{
		std::string ssid;

		HANDLE hClient = nullptr;
		DWORD dwMaxClient = 2;
		DWORD dwCurVersion = 0;

		// WLAN API クライアントハンドルを開く
		DWORD dwResult = WlanOpenHandle(dwMaxClient, nullptr, &dwCurVersion, &hClient);
		if (dwResult != ERROR_SUCCESS)
		{
			std::cerr << "[NetworkMonitor] WlanOpenHandle に失敗しました。エラーコード: " << dwResult << std::endl;
			return ssid;
		}

		// インターフェースリストを取得する
		PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
		dwResult = WlanEnumInterfaces(hClient, nullptr, &pIfList);
		if (dwResult != ERROR_SUCCESS)
		{
			std::cerr << "[NetworkMonitor] WlanEnumInterfaces に失敗しました。エラーコード: " << dwResult << std::endl;
			WlanCloseHandle(hClient, nullptr);
			return ssid;
		}

		// 最初に見つかった接続済みインターフェースの SSID を取得する
		for (DWORD i = 0; i < pIfList->dwNumberOfItems; ++i)
		{
			const PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];
			if (pIfInfo->isState != wlan_interface_state_connected)
			{
				continue;
			}

			PWLAN_CONNECTION_ATTRIBUTES pConnAttr = nullptr;
			DWORD dwDataSize = 0;
			dwResult = WlanQueryInterface(
				hClient,
				&pIfInfo->InterfaceGuid,
				wlan_intf_opcode_current_connection,
				nullptr,
				&dwDataSize,
				reinterpret_cast<PVOID*>(&pConnAttr),
				nullptr);

			if (dwResult == ERROR_SUCCESS && pConnAttr != nullptr)
			{
				// SSID をバイト列から std::string に変換する
				const PDOT11_SSID pDot11Ssid = &pConnAttr->wlanAssociationAttributes.dot11Ssid;
				ssid = std::string(
					reinterpret_cast<const char*>(pDot11Ssid->ucSSID),
					pDot11Ssid->uSSIDLength);

				WlanFreeMemory(pConnAttr);
				break;
			}
		}

		WlanFreeMemory(pIfList);
		WlanCloseHandle(hClient, nullptr);

		return ssid;
	}


} // namespace app