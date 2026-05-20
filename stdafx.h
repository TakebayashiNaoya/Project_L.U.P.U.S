/**
 * @file stdafx.h
 * @brief プリコンパイル済みヘッダー
 */
#pragma once

 // Windows API（NetworkMonitor 用）
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <wlanapi.h>
#include <objbase.h>

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