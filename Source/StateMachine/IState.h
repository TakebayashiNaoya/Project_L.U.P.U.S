/**
 * @file IState.h
 * @brief ステートマシンの状態インターフェース
 */
#pragma once
#include "stdafx.h"


namespace app
{


	/**
	 * @brief 各状態クラスが実装すべきインターフェース
	 */
	class IState
	{
	public:
		IState() = default;
		virtual ~IState() = default;

		/**
		 * @brief 状態に遷移した直後に1度だけ呼ばれる
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		virtual void OnEnter(const nlohmann::json& profile) = 0;

		/**
		 * @brief 状態が継続中に定期的に呼ばれる
		 */
		virtual void OnUpdate() = 0;

		/**
		 * @brief 状態から抜ける直前に1度だけ呼ばれる
		 */
		virtual void OnExit() = 0;

		/**
		 * @brief 状態名を返す
		 * @return 状態名文字列
		 */
		virtual const char* GetName() const = 0;
	};


} // namespace app
