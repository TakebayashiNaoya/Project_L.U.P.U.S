/**
 * @file main.cpp
 * @brief エントリポイント
 */
#include "stdafx.h"
#include "Source/App/LupusApp.h"


int main()
{
	// コンソールの入出力エンコーディングを UTF-8 に設定する
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	app::LupusApp lupusApp;

	if (!lupusApp.Init())
	{
		std::cerr << "[main] 初期化に失敗しました。アプリケーションを終了します" << std::endl;
		return -1;
	}

	// Enter キー入力を別スレッドで待機し、Run() を終了させる
	std::thread inputThread([&lupusApp]()
		{
			std::cin.get();
			lupusApp.Shutdown();
		});

	lupusApp.Run();

	if (inputThread.joinable())
	{
		inputThread.join();
	}

	return 0;
}