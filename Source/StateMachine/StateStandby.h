/**
 * @file StateStandby.h
 * @brief 待機状態クラス
 */
#pragma once
#include <string>
#include "Source/StateMachine/IState.h"


namespace app
{


	/** 前方宣言 */
	class IAudioPipeline;


	/**
	 * @brief 外出判定時のバックグラウンド待機状態
	 * @details コンストラクタで assistantName / standbyMessage / audioPipeline を DI する。
	 *          OnUpdate() は初回のみ standbyMessage を出力し、以降はスキップする。
	 */
	class StateStandby : public IState
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param assistantName  アシスタント名(ログの冒頭ラベルに使用)
		 * @param standbyMessage Standby 状態のメッセージ(persona.json の standby_message)
		 * @param audioPipeline  音声パイプラインへの非所有ポインタ。
		 *                       nullptr 許容(nullptr 時は音声出力をスキップ)。
		 *                       所有権は LupusApp が持ち、このクラスより長く生存する。
		 */
		StateStandby(
			const std::string& assistantName,
			const std::string& standbyMessage,
			IAudioPipeline* audioPipeline
		);

		void OnEnter() override;
		void OnUpdate() override;
		void OnExit() override;
		const char* GetName() const override;


	private:
		/**
		 * @brief ユーザーへの通知を行うカプセル化メソッド
		 * @details std::cout へのログ出力と、AudioPipeline 経由の音声再生を行う。
		 * @param message 出力するメッセージ文字列
		 */
		void NotifyUser(const std::string& message) const;


	private:
		/** アシスタント名(ログの冒頭ラベルに使用) */
		const std::string m_assistantName;
		/** Standby 状態のメッセージ */
		const std::string m_standbyMessage;
		/** 音声パイプラインへの非所有ポインタ */
		IAudioPipeline* m_audioPipeline = nullptr;
		/** OnUpdate() で初回のみ出力するためのフラグ */
		bool m_isFirstUpdate = true;
	};


} // namespace app