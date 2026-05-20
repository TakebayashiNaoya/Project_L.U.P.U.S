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


	std::string NetworkMonitor::Observe()
	{
		const std::string currentSsid = GetCurrentSsid();

		// home_ssids が空、または SSID が一致しない場合は外出とみなす
		bool isAtHome = false;
		if (!currentSsid.empty() && !m_homeSsids.empty())
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
		if (isAtHome != m_isAtHome)
		{
			m_isAtHome = isAtHome;
			if (m_isAtHome)
			{
				std::cout << "[NetworkMonitor] 在宅を検出しました。SSID: " << currentSsid << std::endl;
			}
			else
			{
				std::cout << "[NetworkMonitor] 外出を検出しました。SSID: " << currentSsid << std::endl;
			}
		}

		return m_isAtHome ? "" : "Standby";
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