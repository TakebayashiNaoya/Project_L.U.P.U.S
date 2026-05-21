/**
 * @file IState.h
 * @brief ステートマシンの状態インターフェース
 */
#pragma once


namespace app
{


	/**
	 * @brief 各状態クラスが実装すべきインターフェース
	 * @details 各 State が必要とするデータはすべてコンストラクタ経由で DI する。
	 *          OnEnter() は遷移通知のみを担い、外部データへの依存を持たない。
	 */
	class IState
	{
	public:
		IState() = default;
		virtual ~IState() = default;

		/**
		 * @brief 状態に遷移した直後に1度だけ呼ばれる
		 */
		virtual void OnEnter() = 0;

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