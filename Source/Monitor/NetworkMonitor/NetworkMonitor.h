/**
 * @file NetworkMonitor.h
 * @brief ネットワーク監視モジュール
 */
#pragma once
#include "stdafx.h"
#include "Source/Monitor/IMonitor.h"


namespace app
{


	/**
	 * @brief Wi-Fi SSID を監視し、在宅・外出を判定する監視モジュール
	 * @details 現在接続中の SSID がプロファイルの home_ssids リストに含まれる場合を在宅とみなす。
	 *          home_ssids が空、または SSID が一致しない場合は "Standby" への遷移を通知する。
	 */
	class NetworkMonitor : public IMonitor
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		explicit NetworkMonitor(const nlohmann::json& profile);

		/**
		 * @brief Wi-Fi SSID を取得し、在宅判定を行う
		 * @return 外出判定時は "Standby"、在宅判定時は空文字列
		 */
		std::string Observe() override;

		/**
		 * @brief モジュール名を返す
		 * @return "NetworkMonitor"
		 */
		const char* GetName() const override;


	private:
		/**
		 * @brief 現在接続中の Wi-Fi SSID を取得する
		 * @return 接続中の SSID 文字列。取得失敗時は空文字列
		 */
		std::string GetCurrentSsid() const;


	private:
		/** プロファイルから読み込んだ自宅 SSID リスト */
		std::vector<std::string> m_homeSsids;
		/** 前回の在宅状態フラグ（差分ログ出力の抑制用） */
		bool m_isAtHome = false;
	};


} // namespace app
