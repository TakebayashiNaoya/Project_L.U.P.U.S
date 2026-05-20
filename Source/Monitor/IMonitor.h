/**
 * @file IMonitor.h
 * @brief 監視モジュールのインターフェース
 */
#pragma once
#include "stdafx.h"


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
		 * @brief 監視を1回実行し、遷移すべき状態名を返す
		 * @return 遷移先の状態名。遷移不要の場合は空文字列を返す
		 */
		virtual std::string Observe() = 0;

		/**
		 * @brief 監視モジュール名を返す
		 * @return モジュール名文字列
		 */
		virtual const char* GetName() const = 0;
	};


} // namespace app
