/**
 * @file StateTaskCompleted.h
 * @brief タスク完了状態クラス
 */
#pragma once
#include <string>
#include "Source/StateMachine/IState.h"


namespace app
{


	/** 前方宣言 */
	class IAudioPipeline;


	/**
	 * @brief 自宅かつ全タスク完了時のリラックス状態
	 * @details コンストラクタで assistantName / completionMessage / audioPipeline を DI する。
	 *          OnUpdate() は初回のみ completionMessage を出力し、以降はスキップする。
	 */
	class StateTaskCompleted : public IState
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param assistantName     アシスタント名(ログの冒頭ラベルに使用)
		 * @param completionMessage TaskCompleted 状態のメッセージ(persona.json の completion_message)
		 * @param audioPipeline     音声パイプラインへの非所有ポインタ。
		 *                          nullptr 許容(nullptr 時は音声出力をスキップ)。
		 *                          所有権は LupusApp が持ち、このクラスより長く生存する。
		 */
		StateTaskCompleted(
			const std::string& assistantName,
			const std::string& completionMessage,
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
		/** TaskCompleted 状態のメッセージ */
		const std::string m_completionMessage;
		/** 音声パイプラインへの非所有ポインタ */
		IAudioPipeline* m_audioPipeline = nullptr;
		/** OnUpdate() で初回のみ出力するためのフラグ */
		bool m_isFirstUpdate = true;
	};


} // namespace app