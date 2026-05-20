/**
 * @file NotionMonitor.h
 * @brief Notion タスク監視モジュール
 */
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Source/Monitor/IMonitor.h"
#include "external/json.hpp"


namespace app
{


	/**
	 * @brief Notion API を叩いて未完了タスクの一覧を監視する監視モジュール
	 * @details ネットワーク接続時のみ動作する（RequiresNetwork() が true を返す）。
	 *          初回 Observe() 時に以下の Auto-Discovery を実行する:
	 *            1. /search で連携済み全 DB の ID を取得
	 *            2. /databases/{id} で各 DB のステータスプロパティのスキーマを取得し
	 *               "Complete" グループに属する option_id セットを保存
	 *          タスク判定はゼロコンフィグ方式:
	 *            - status プロパティ: option_id が Complete グループ以外 → 未完了
	 *            - status がない場合: checkbox が false → 未完了
	 */
	class NotionMonitor : public IMonitor
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param profile 読み込み済みのシステムプロファイル
		 */
		explicit NotionMonitor(const nlohmann::json& profile);

		/**
		 * @brief Notion API を呼び出し、未完了タスク一覧を context に書き込む
		 * @details 初回呼び出し時に Auto-Discovery を実行する
		 * @param context 更新対象の共有コンテキスト
		 */
		void Observe(SystemContext& context) override;

		/**
		 * @brief モジュール名を返す
		 * @return "NotionMonitor"
		 */
		const char* GetName() const override;

		/**
		 * @brief ネットワーク接続時のみ動作するモジュールであることを示す
		 * @return true
		 */
		bool RequiresNetwork() const override { return true; }


	private:
		/**
		 * @brief /search で連携済み DB を検出し、各 DB のスキーマを取得する
		 * @return 1件以上検出できた場合 true
		 */
		bool DiscoverDatabases();

		/**
		 * @brief /databases/{id} から status プロパティの Complete グループの option_ids を取得する
		 * @param databaseId 対象データベース ID
		 */
		void FetchDatabaseSchema(const std::string& databaseId);

		/**
		 * @brief 指定 DB の全未完了タスクを取得して返す
		 * @param databaseId 対象データベース ID
		 * @return 未完了タスクのリスト
		 */
		std::vector<NotionTask> FetchPendingTasks(const std::string& databaseId) const;

		/**
		 * @brief ページオブジェクトから NotionTask を生成する
		 * @param page Notion API レスポンスの results 配列の1要素
		 * @param databaseId タスクが属するデータベース ID
		 * @return 生成した NotionTask
		 */
		NotionTask ParseTask(const nlohmann::json& page, const std::string& databaseId) const;

		/**
		 * @brief ページオブジェクトが未完了かどうかを判定する
		 * @details status の option_id が Complete グループ以外なら未完了。
		 *          status がなければ checkbox が false なら未完了。
		 * @param page Notion API レスポンスの results 配列の1要素
		 * @param databaseId タスクが属するデータベース ID
		 * @return 未完了であれば true
		 */
		bool IsPageIncomplete(const nlohmann::json& page, const std::string& databaseId) const;

		/**
		 * @brief WinHTTP で Notion API に HTTPS リクエストを送信する
		 * @param method HTTP メソッド（"POST" / "GET"）
		 * @param path エンドポイントのパス
		 * @param body リクエストボディの JSON 文字列（GET の場合は空文字列）
		 * @return レスポンスボディ文字列。失敗時は空文字列
		 */
		std::string SendRequest(const std::string& method, const std::string& path, const std::string& body) const;


	private:
		/** Notion API トークン */
		std::string m_notionToken;
		/** Auto-Discovery で取得したデータベース ID リスト */
		std::vector<std::string> m_databaseIds;
		/** DB ID → Complete グループに属する option_id のセット */
		std::unordered_map<std::string, std::unordered_set<std::string>> m_completeOptionIds;
		/** DB の自動検出が完了しているかどうか */
		bool m_isDiscovered = false;
		/** 前回の未完了タスク有無フラグ（差分ログ出力の抑制用） */
		bool m_hasPendingTasks = false;
	};


} // namespace app