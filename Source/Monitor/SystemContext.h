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
		/** 締め切り日（ISO 8601形式: "2026-05-21"）。未設定の場合は空文字列 */
		std::string m_dueDate;
		/** ステータス名（例: "未実装" / "実装中" / "保留"） */
		std::string m_status;
		/** ステータスのグループ名（"To-do" / "In progress" / "Complete"） */
		std::string m_group;
	};


	/**
	 * @brief 各 IMonitor が更新し、StateMachine::Evaluate() が参照する共有状態構造体
	 * @details atomic メンバはロックなしで読み書き可能。
	 *          m_pendingTasks は m_taskMutex で保護すること。
	 */
	struct SystemContext
	{
		/** 自宅 Wi-Fi に接続中かどうか（NetworkMonitor が更新） */
		std::atomic<bool> m_isAtHome{ false };
		/** 何らかのネットワークに接続中かどうか（NetworkMonitor が更新） */
		std::atomic<bool> m_isConnected{ false };
		/** 未完了タスクが1件以上あるかどうか（NotionMonitor が更新） */
		std::atomic<bool> m_hasPendingTasks{ false };
		/** 姿勢悪化フラグ（PostureMonitor が更新） */
		std::atomic<bool> m_isSlouching{ false };
		/** 未完了タスクの詳細リスト（NotionMonitor が更新） */
		std::vector<NotionTask> m_pendingTasks;
		/** m_pendingTasks の排他制御用ミューテックス */
		mutable std::mutex m_taskMutex;
	};


} // namespace app