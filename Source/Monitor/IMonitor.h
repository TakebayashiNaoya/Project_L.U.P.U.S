/**
 * @file IMonitor.h
 * @brief 監視モジュールのインターフェース
 */
#pragma once
#include "Source/Monitor/SystemContext.h"


namespace app
{


	/**
	 * @brief 各監視モジュール（カメラ・ネットワーク・Notion等）が実装すべきインターフェース
	 */
	class IMonitor
	{
	public:
		/** @brief デストラクタ */
		virtual ~IMonitor() = default;

		/**
		 * @brief 監視を1回実行し、結果を SystemContext に書き込む
		 * @param context 更新対象の共有コンテキスト
		 */
		virtual void Observe(SystemContext& context) = 0;

		/**
		 * @brief 監視モジュール名を返す
		 * @return モジュール名文字列
		 */
		virtual const char* GetName() const = 0;

		/**
		 * @brief ネットワーク接続時のみ動作するモジュールかどうかを返す
		 * @details true を返すモジュールは、context.m_isConnected が false の場合に
		 *          MonitorThread からの Observe() 呼び出しがスキップされる
		 * @return ネットワーク接続時のみ動作する場合 true。デフォルトは false
		 */
		virtual bool RequiresNetwork() const { return false; }
	};


} // namespace app