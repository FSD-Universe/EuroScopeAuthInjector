// Copyright (c) 2025-2026 Half_nothing
// SPDX-License-Identifier: MIT

#pragma once

#ifndef DEFINITION_H
#define DEFINITION_H

// 原始函数类型定义
typedef void *(__cdecl *CURL_EASY_INIT_FUNC)();

typedef int (__cdecl *CURL_EASY_SETOPT_FUNC)(void *curl, int option, ...);

typedef int (__cdecl *CURL_EASY_PERFORM_FUNC)(void *curl);

// libcurl 选项常量
#define CURLOPT_URL 10002

#define PLUGIN_NAME "EuroScope Auth Injector"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_DEVELOPER "Half_nothing"
#define PLUGIN_COPYRIGHT "MIT License (C) 2025-2026"

#define LOG_FILE_PATH "EuroScopeInjector.log"
#define INJECTOR_URL_FILENAME "InjectorUrl.txt"
#define PLUGIN_DISPLAY_NAME "AuthInjector"
#define INJECTOR_URL "http://127.0.0.1:6810/api/users/sessions/fsd"

#endif
