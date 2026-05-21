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


	/**
	 * @brief 未完了タスクあり・規約違反・姿勢悪化を検知した際の集中促進状態
	 * @details コンストラクタで SystemContext / assistantName / instantWarningMessage を DI する。
	 *          OnUpdate() で GetAllWarnings() の差分を比較し、変化があった場合のみ
	 *          アシスタント名とメッセージとともに警告を出力するデバウンス機構を持つ。
	 */
	class StateTaskFocus : public IState
	{
	public:
		/**
		 * @brief コンストラクタ
		 * @param context               各 Monitor が更新する共有コンテキスト(非所有参照)
		 * @param assistantName         アシスタント名(persona.json の assistant_name)
		 * @param instantWarningMessage Level 1 即時警告メッセージ
		 */
		StateTaskFocus(
			SystemContext& context,
			const std::string& assistantName,
			const std::string& instantWarningMessage
		);

		void OnEnter(const nlohmann::json& profile) override;
		void OnUpdate() override;
		void OnExit() override;
		const char* GetName() const override;


	private:
		/** 各 Monitor が更新する共有コンテキストへの参照(非所有) */
		SystemContext& m_context;
		/** アシスタント名(ログの冒頭ラベルに使用) */
		const std::string m_assistantName;
		/** Level 1 即時警告メッセージ */
		const std::string m_instantWarningMessage;
		/**
		 * @brief 前回 OnUpdate() で出力した警告リストのキャッシュ
		 * @details GetAllWarnings() の結果と比較してデバウンスを実現する
		 */
		std::vector<std::string> m_lastWarnings;
	};


} // namespace app