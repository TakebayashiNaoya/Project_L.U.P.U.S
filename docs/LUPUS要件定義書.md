# プロジェクト要件定義書：L.U.P.U.S. (Local Utility & Productivity Unified System)

## 1. プロジェクト概要
ローカルPC環境に常駐し、ユーザーのコンテキスト（タスク進捗、コーディング規約、ドキュメント執筆状態）を監視・学習するC++ベースの開発支援AIアーキテクチャ。プログラム本体と外部データ（プロファイル・プロンプト等）を完全に分離する「データ駆動設計」を採用する。

## 2. システムアーキテクチャと命名規則
* **コアロジック:** C++ 
* **命名規則の厳格化:** クラス名は `PascalCase`。真偽値メンバ変数は `m_is` プレフィックス、数値等その他のメンバ変数は `m_` プレフィックス + `camelCase` を必ず守ること。

## 3. セキュリティおよびプロファイル動的読み込み仕様
1. 起動時に、Git管理対象外の `private/custom_profile.json` を最優先で走査・読み込む。
2. 存在しない場合は公開用の `config/default_profile.json` を読み込む。
これにより、ポートフォリオとしてのリポジトリの無菌化と、ローカル環境での個別ペルソナ（嫁モード等）の上書き運用を両立する。

## 4. SystemContext（データフローの核）とレビューアーキテクチャ
Monitorモジュールは純粋なセンサーとして機能し、収集データは `SystemContext` に集約される。
レビューと通知は、UX低下（警告疲れ）を防ぐため「Push/Pull分離モデル」を採用する。
* **Level 1（Push型 / 即時）:** 保存時のタイポ、マジックナンバー、命名規則違反（m_is漏れ等）の軽微なミスは即時検知し `m_instantWarnings` に格納。
* **Level 2（Pull型 / 要求時）:** LLMを用いた深い文脈レビューは自動実行せず、ユーザーからの明示的な要求時のみ実行し `m_deepReviewResult` に格納。

**【SystemContextの主要メンバ】**
* `std::atomic<bool> m_isAtHome`
* `std::atomic<bool> m_hasPendingTasks`
* `std::vector<std::string> m_instantWarnings` (Linter的即時ツッコミ用)
* `std::string m_deepReviewResult` (オンデマンドの深いレビュー結果用)

## 5. コンテキスト監視モジュール (IMonitor)
各Monitorは `SystemContext&` の参照を受け取り更新する。不要な監視をスキップするため `virtual bool RequiresAtHome() const` を実装する。

* **CodeReviewMonitor (ゼロコンフィグ差分スキャン):**
  プロジェクト全体の `.cpp`, `.h` を対象。`std::filesystem::last_write_time` を用いて直近保存されたファイルのみをスキャン。正規表現を用いて即時Push型（Level 1）の命名規則違反を検知する。
* **DocumentReviewMonitor (Strategyパターン & Tempコピー):**
  `.md`, `.txt`, `.docx`, `.xlsx` を対象とした差分スキャン。
  1. **Tempコピー機構:** Word/Excelの排他ロックを回避するため、保存検知時にファイルを `private/temp/` 等に一時コピーしてから読み込む。
  2. **Strategyパターン:** `IDocumentExtractor` インターフェースを定義し、拡張子に応じて `MarkdownExtractor` 等に処理を委譲。画像データ（mediaフォルダ等）は安全に無視し、テキスト抽出のみ行う。
* **NotionMonitor:** Notion APIの `/search` エンドポイント等を用い、ステータスプロパティの所属グループ(To-do, In progress)等を利用してゼロコンフィグで未完了判定を行う。

## 6. ステートマシン（状態遷移）
* **Standby:** 外出判定時(`m_isAtHome == false`)。
* **TaskFocus:** 自宅かつ、未完了タスクあり、または即時警告(`m_instantWarnings` が空でない)時に遷移。
* **TaskCompleted:** 自宅かつ全タスク完了、警告なし時に遷移。