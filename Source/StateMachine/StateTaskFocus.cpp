/**
 * @file StateTaskFocus.cpp
 * @brief 集中状態クラス実装
 */
#include "stdafx.h"
#include "StateTaskFocus.h"


namespace app
{
	StateTaskFocus::StateTaskFocus(
		SystemContext& context,
		const std::string& assistantName,
		const std::string& instantWarningMessage
	)
		: m_context(context)
		, m_assistantName(assistantName)
		, m_instantWarningMessage(instantWarningMessage)
	{}


	void StateTaskFocus::OnEnter(const nlohmann::json& profile)
	{
		// 前回警告キャッシュをクリアして、OnUpdate() で必ず初回出力が走るようにする
		m_lastWarnings.clear();

		std::cout << "[StateTaskFocus] OnEnter" << std::endl;
		std::cout << "[" << m_assistantName << "] " << m_instantWarningMessage << std::endl;
	}


	void StateTaskFocus::OnUpdate()
	{
		// 全モニターの警告を統合取得する
		const std::vector<std::string> currentWarnings = m_context.GetAllWarnings();

		// 前回と内容が同じ場合は出力をスキップする(デバウンス)
		if (currentWarnings == m_lastWarnings)
		{
			return;
		}

		// 差分があった場合はキャッシュを更新して出力する
		m_lastWarnings = currentWarnings;

		if (currentWarnings.empty())
		{
			return;
		}

		// アシスタント名と即時警告メッセージを冒頭に添えてから違反リストを表示する
		std::cout << "[" << m_assistantName << "] " << m_instantWarningMessage << std::endl;
		std::cout << "[StateTaskFocus] 警告: " << currentWarnings.size() << " 件" << std::endl;

		for (const auto& warning : currentWarnings)
		{
			std::cout << "  >> " << warning << std::endl;
		}
	}


	void StateTaskFocus::OnExit()
	{
		m_lastWarnings.clear();
		std::cout << "[StateTaskFocus] OnExit" << std::endl;
	}


	const char* StateTaskFocus::GetName() const
	{
		return "TaskFocus";
	}


} // namespace app