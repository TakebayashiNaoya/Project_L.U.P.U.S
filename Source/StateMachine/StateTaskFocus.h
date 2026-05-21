/**
 * @file StateTaskFocus.h
 * @brief 集中状態クラス
 */
#pragma once
#include <future>
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
	 *          instantWarningMessage / llmClient を DI する。
	 *          OnUpdate() で BuildPrompt() の差分を比較し、変化があった場合のみ
	 *          LLM リクエストを起動するデバウンス機構を持つ。
	 *          LLM 呼び出しは std::async で非同期化し、std::future で寿命を管理する。
	 *          OnExit() で未完了の非同期処理を wait() してから抜けるため、
	 *          ILLMClient の use-after-free は発生しない。
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
		 * @param llmClient             LLM クライアントへの非所有ポインタ。
		 *                              nullptr 許容(nullptr 時は LLM 呼び出しをスキップ)。
		 *                              所有権は StateMachine が持ち、このクラスより長く生存する。
		 *                              スレッド安全性は ILLMClient 実装側の責務とする。
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
		 * @brief systemPrompt・環境情報・警告リスト・タスクリストを結合して LLM プロンプトを組み立てる
		 * @param warnings GetAllWarnings() で取得した警告リスト
		 * @return 組み立て済みのプロンプト文字列
		 */
		std::string BuildPrompt(const std::vector<std::string>& warnings) const;

		/**
		 * @brief 保留中の LLM レスポンスを取り出してログ出力する
		 * @details m_pendingResponse が有効かつ ready な場合のみ出力する。
		 *          ブロッキングしない(wait_for(0) で即時チェック)。
		 */
		void FlushPendingResponse();

		/**
		 * @brief 非同期で LLM リクエストを起動し、m_pendingResponse に格納する
		 * @details 前のリクエストが処理中の場合は新規起動をスキップする。
		 * @param prompt 送信するプロンプト文字列
		 */
		void LaunchLLMRequest(const std::string& prompt);


	private:
		/** 各 Monitor が更新する共有コンテキストへの参照(非所有) */
		SystemContext& m_context;
		/** LLM 連携用のシステムプロンプト文字列 */
		const std::string m_systemPrompt;
		/** アシスタント名(ログの冒頭ラベルに使用) */
		const std::string m_assistantName;
		/** Level 1 即時警告メッセージ */
		const std::string m_instantWarningMessage;
		/** LLM クライアントへの非所有ポインタ(所有権は StateMachine が持つ) */
		ILLMClient* m_llmClient = nullptr;
		/**
		 * @brief 前回 BuildPrompt() で生成したプロンプト文字列のキャッシュ
		 * @details この文字列と比較して差分がなければ LLM 呼び出しをスキップする(デバウンス)
		 */
		std::string m_lastPrompt;
		/**
		 * @brief 非同期 LLM リクエストの Future
		 * @details std::async で起動し、OnUpdate() で ready チェック、OnExit() で wait() する。
		 *          有効な Future が存在する間は新規リクエストをスキップする。
		 */
		std::future<std::string> m_pendingResponse;
	};


} // namespace app