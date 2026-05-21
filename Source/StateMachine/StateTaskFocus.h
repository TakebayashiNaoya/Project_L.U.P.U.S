/**
 * @file StateTaskFocus.h
 * @brief 集中状態クラス
 */
#pragma once
#include <string>
#include <vector>
#include "Source/StateMachine/IState.h"
#include "Source/Monitor/SystemContext.h"


namespace app
{


	/** 前方宣言 */
	class ILLMClient;


	/**
	 * @brief 未完了タスクあり・規約違反・姿勢悪化を検知した際の集中促進状態
	 * @details コンストラクタで SystemContext / systemPrompt / assistantName /
	 *          instantWarningMessage を DI する。
	 *          OnUpdate() で GetAllWarnings() の差分を比較し、変化があった場合のみ
	 *          LLM プロンプトを組み立てて出力するデバウンス機構を持つ。
	 */
	class StateTaskFocus : public IState
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param context               各 Monitor が更新する共有コンテキスト(非所有参照)
		 * @param systemPrompt          LLM 連携用のシステムプロンプト文字列
		 * @param assistantName         アシスタント名(ログの冒頭ラベルに使用)
		 * @param instantWarningMessage Level 1 即時警告メッセージ
		 */
		StateTaskFocus(
			SystemContext& context,
			const std::string& systemPrompt,
			const std::string& assistantName,
			const std::string& instantWarningMessage,
			ILLMClient* llmClient
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

		/**
		 * @brief systemPrompt・警告リスト・タスクリストを結合して LLM プロンプトを組み立てる
		 * @param warnings GetAllWarnings() で取得した警告リスト
		 * @return 組み立て済みのプロンプト文字列
		 */
		std::string BuildPrompt(const std::vector<std::string>& warnings) const;


	private:
		/** 各 Monitor が更新する共有コンテキストへの参照(非所有) */
		SystemContext& m_context;
		/** LLM 連携用のシステムプロンプト文字列 */
		const std::string m_systemPrompt;
		/** アシスタント名(ログの冒頭ラベルに使用) */
		const std::string m_assistantName;
		/** Level 1 即時警告メッセージ */
		const std::string m_instantWarningMessage;
		/** LLM クライアントへの非所有ポインタ*/
		ILLMClient* m_llmClient = nullptr;
		/**
		 * @brief 前回 OnUpdate() で出力した警告リストのキャッシュ
		 * @details GetAllWarnings() の結果と比較してデバウンスを実現する
		 */
		std::string m_lastPrompt;
	};


} // namespace app