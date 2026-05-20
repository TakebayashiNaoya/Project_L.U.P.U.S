/**
 * @file NetworkMonitor.h
 * @brief ネットワーク監視モジュール
 */
#pragma once
#include <string>
#include <vector>
#include "Source/Monitor/IMonitor.h"
#include "external/json.hpp"


namespace app
{


	/**
	 * @brief Wi-Fi SSID を監視し、ネットワーク接続状態と在宅・外出を判定する監視モジュール
	 * @details 判定結果は context.m_isConnected および context.m_isAtHome に書き込む。
	 *          - m_isConnected: 何らかの Wi-Fi に接続中であれば true
	 *          - m_isAtHome: 接続中の SSID が home_ssids リストと一致すれば true
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
		 * @brief Wi-Fi SSID を取得し、接続状態と在宅判定結果を context に書き込む
		 * @param context 更新対象の共有コンテキスト
		 */
		void Observe(SystemContext& context) override;

		/**
		 * @brief モジュール名を返す
		 * @return "NetworkMonitor"
		 */
		const char* GetName() const override;


	private:
		/**
		 * @brief 現在接続中の Wi-Fi SSID を取得する
		 * @return 接続中の SSID 文字列。取得失敗または未接続時は空文字列
		 */
		std::string GetCurrentSsid() const;


	private:
		/** プロファイルから読み込んだ自宅 SSID リスト */
		std::vector<std::string> m_homeSsids;
		/** 前回のネットワーク接続状態フラグ（差分ログ出力の抑制用） */
		bool m_isConnected = false;
		/** 前回の在宅状態フラグ（差分ログ出力の抑制用） */
		bool m_isAtHome = false;
	};


} // namespace app