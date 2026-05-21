/**
 * @file SystemContext.h
 * @brief 各監視モジュールが更新する共有コンテキスト構造体
 */
#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <vector>


namespace app
{


	/**
	 * @brief Notion から取得した1タスク分の情報を保持する構造体
	 */
	struct NotionTask
	{
		/** タスク名 */
		std::string m_title;
		/** 担当者名リスト */
		std::vector<std::string> m_assignees;
		/** 締め切り日(ISO 8601形式: "2026-05-21")。未設定の場合は空文字列 */
		std::string m_dueDate;
		/** ステータス名(例: "未実装" / "実装中" / "保留") */
		std::string m_status;
		/** ステータスのグループ名("To-do" / "In progress" / "Complete") */
		std::string m_group;
	};


	/**
	 * @brief 各 IMonitor が更新し、StateMachine::Evaluate() が参照する共有状態構造体
	 * @details atomic メンバはロックなしで読み書き可能。
	 *          各コンテナはそれぞれ専用の Mutex で保護すること。
	 *          全警告を横断的に取得する場合は GetAllWarnings() を使用すること。
	 */
	struct SystemContext
	{
		// ---------------------------------------------------------------
		// ネットワーク・在宅フラグ(NetworkMonitor が更新)
		// ---------------------------------------------------------------

		/** 自宅 Wi-Fi に接続中かどうか */
		std::atomic<bool> m_isAtHome{ false };
		/** 何らかのネットワークに接続中かどうか */
		std::atomic<bool> m_isConnected{ false };


		// ---------------------------------------------------------------
		// タスク情報(NotionMonitor が更新)
		// ---------------------------------------------------------------

		/** 未完了タスクが1件以上あるかどうか */
		std::atomic<bool> m_hasPendingTasks{ false };
		/** 未完了タスクの詳細リスト */
		std::vector<NotionTask> m_pendingTasks;
		/** m_pendingTasks の排他制御用 Mutex */
		mutable std::mutex m_taskMutex;


		// ---------------------------------------------------------------
		// 姿勢フラグ(PostureMonitor が更新)
		// ---------------------------------------------------------------

		/** 姿勢悪化フラグ */
		std::atomic<bool> m_isSlouching{ false };


		// ---------------------------------------------------------------
		// コーディング規約違反(CodeReviewMonitor が更新)
		// ---------------------------------------------------------------

		/** コーディング規約違反が1件以上あるかどうか */
		std::atomic<bool> m_hasCodeViolations{ false };
		/**
		 * @brief コーディング規約違反の詳細メッセージリスト
		 * @details CodeReviewMonitor の Observe() 冒頭で clear() してから書き直す。
		 *          読み書き時は必ず m_codeViolationsMutex でロックすること。
		 */
		std::vector<std::string> m_codeViolations;
		/** m_codeViolations の排他制御用 Mutex */
		mutable std::mutex m_codeViolationsMutex;


		// ---------------------------------------------------------------
		// ドキュメント変更通知(DocumentReviewMonitor が更新)
		// ---------------------------------------------------------------

		/**
		 * @brief ドキュメント変更検知の通知メッセージリスト
		 * @details DocumentReviewMonitor の Observe() 冒頭で clear() してから書き直す。
		 *          読み書き時は必ず m_documentWarningsMutex でロックすること。
		 */
		std::vector<std::string> m_documentWarnings;
		/** m_documentWarnings の排他制御用 Mutex */
		mutable std::mutex m_documentWarningsMutex;


		// ---------------------------------------------------------------
		// Level 2 オンデマンドレビュー(将来の LLM 連携用)
		// ---------------------------------------------------------------

		/**
		 * @brief Level 2 Pull 型のオンデマンドレビュー結果文字列
		 * @details ユーザーからの明示的な要求時のみ LLM が生成して格納する。
		 *          読み書き時は必ず m_deepReviewMutex でロックすること。
		 */
		std::string m_deepReviewResult;
		/** m_deepReviewResult の排他制御用 Mutex */
		mutable std::mutex m_deepReviewMutex;


		// ---------------------------------------------------------------
		// 統合読み取りビュー
		// ---------------------------------------------------------------

		/**
		 * @brief 全モニターの警告を結合して返す読み取り専用ビュー
		 * @details m_codeViolations と m_documentWarnings を両方ロックして結合する。
		 *          LLM プロンプト組み立てや StateTaskFocus での差分比較に使用する。
		 * @return 全警告メッセージのコピーリスト
		 */
		std::vector<std::string> GetAllWarnings() const
		{
			std::vector<std::string> result;

			{
				std::lock_guard<std::mutex> lock(m_codeViolationsMutex);
				result.insert(result.end(), m_codeViolations.begin(), m_codeViolations.end());
			}
			{
				std::lock_guard<std::mutex> lock(m_documentWarningsMutex);
				result.insert(result.end(), m_documentWarnings.begin(), m_documentWarnings.end());
			}

			return result;
		}
	};


} // namespace app