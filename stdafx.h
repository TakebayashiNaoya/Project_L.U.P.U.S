/**
 * @file stdafx.h
 * @brief プリコンパイル済みヘッダー
 */
#pragma once

 // Windows API(NetworkMonitor / NotionMonitor 用)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <winhttp.h>

// 標準ライブラリ
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <vector>

// 外部ライブラリ
#include "external/json.hpp"
#include "external/CRC32.h"


/**
 * @brief std::filesystem::path を UTF-8 エンコードの std::string に変換するヘルパー
 * @details Windows の std::filesystem::path は内部でワイド文字(UTF-16)を保持しており、
 *          path::string() はシステムロケール(CP932)で変換するため日本語ファイル名が
 *          文字化けする。このヘルパーは WideCharToMultiByte で明示的に UTF-8 に変換する。
 * @param path 変換対象のパス
 * @return UTF-8 エンコードのパス文字列
 */
inline std::string PathToUtf8(const std::filesystem::path& path)
{
	const std::wstring wide = path.wstring();
	if (wide.empty())
	{
		return {};
	}

	const int size = WideCharToMultiByte(
		CP_UTF8, 0,
		wide.c_str(), static_cast<int>(wide.size()),
		nullptr, 0,
		nullptr, nullptr
	);

	std::string result(size, '\0');
	WideCharToMultiByte(
		CP_UTF8, 0,
		wide.c_str(), static_cast<int>(wide.size()),
		result.data(), size,
		nullptr, nullptr
	);

	return result;
}