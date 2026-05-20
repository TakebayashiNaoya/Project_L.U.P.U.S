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
	 *          m_pendingTasks は m_taskMutex で、
	 *          m_instantWarnings は m_warningsMutex で、
	 *          m_deepReviewResult は m_deepReviewMutex で保護すること。
	 */
	struct SystemContext
	{
		// ---------------------------------------------------------------
		// ネットワーク・在宅フラグ(NetworkMonitor が更新)
		// ---------------------------------------------------------------

		/** 自宅 Wi-Fi に接続中かどうか(NetworkMonitor が更新) */
		std::atomic<bool> m_isAtHome{ false };
		/** 何らかのネットワークに接続中かどうか(NetworkMonitor が更新) */
		std::atomic<bool> m_isConnected{ false };


		// ---------------------------------------------------------------
		// タスク情報(NotionMonitor が更新)
		// ---------------------------------------------------------------

		/** 未完了タスクが1件以上あるかどうか(NotionMonitor が更新) */
		std::atomic<bool> m_hasPendingTasks{ false };
		/** 未完了タスクの詳細リスト(NotionMonitor が更新) */
		std::vector<NotionTask> m_pendingTasks;
		/** m_pendingTasks の排他制御用ミューテックス */
		mutable std::mutex m_taskMutex;


		// ---------------------------------------------------------------
		// 姿勢フラグ(PostureMonitor が更新)
		// ---------------------------------------------------------------

		/** 姿勢悪化フラグ(PostureMonitor が更新) */
		std::atomic<bool> m_isSlouching{ false };


		// ---------------------------------------------------------------
		// Level 1 即時警告(CodeReviewMonitor / DocumentReviewMonitor が更新)
		// ---------------------------------------------------------------

		/** コーディング規約違反が1件以上あるかどうか(CodeReviewMonitor が更新) */
		std::atomic<bool> m_hasCodeViolations{ false };
		/**
		 * @brief Level 1 Push型の即時警告文字列リスト
		 * @details CodeReviewMonitor や DocumentReviewMonitor が違反を検知した際に格納する。
		 *          LLM へのプロンプト(カンペ)として結合して使用することを想定している。
		 *          読み書き時は必ず m_warningsMutex でロックすること。
		 */
		std::vector<std::string> m_instantWarnings;
		/** m_instantWarnings の排他制御用ミューテックス */
		mutable std::mutex m_warningsMutex;


		// ---------------------------------------------------------------
		// Level 2 オンデマンドレビュー(将来の LLM 連携用)
		// ---------------------------------------------------------------

		/**
		 * @brief Level 2 Pull型のオンデマンドレビュー結果文字列
		 * @details ユーザーからの明示的な要求時のみ LLM が生成して格納する。
		 *          読み書き時は必ず m_deepReviewMutex でロックすること。
		 */
		std::string m_deepReviewResult;
		/** m_deepReviewResult の排他制御用ミューテックス */
		mutable std::mutex m_deepReviewMutex;
	};


} // namespace app