/**
 * @file StateStandby.h
 * @brief 待機状態クラス
 */
#pragma once
#include <string>
#include "Source/StateMachine/IState.h"


namespace app
{


	/**
	 * @brief 外出判定時のバックグラウンド待機状態
	 * @details コンストラクタで assistantName / standbyMessage を DI する。
	 *          OnUpdate() は初回のみ standbyMessage を出力し、以降はスキップする。
	 */
	class StateStandby : public IState
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param assistantName  アシスタント名(ログの冒頭ラベルに使用)
		 * @param standbyMessage Standby 状態のメッセージ(persona.json の standby_message)
		 */
		StateStandby(
			const std::string& assistantName,
			const std::string& standbyMessage
		);

		void OnEnter() override;
		void OnUpdate() override;
		void OnExit() override;
		const char* GetName() const override;


	private:
		/**
		 * @brief ユーザーへの通知を行うカプセル化メソッド
		 * @details 現時点では std::cout への出力のみ。
		 *          将来的に TTS 等の音声パイプラインへの連携はここに追加する。
		 * @param message 出力するメッセージ文字列
		 */
		void NotifyUser(const std::string& message) const;


	private:
		/** アシスタント名(ログの冒頭ラベルに使用) */
		const std::string m_assistantName;
		/** Standby 状態のメッセージ */
		const std::string m_standbyMessage;
		/** OnUpdate() で初回のみ出力するためのフラグ */
		bool m_isFirstUpdate = true;
	};


} // namespace app