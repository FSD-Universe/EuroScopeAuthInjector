// Copyright (c) 2025-2026 Half_nothing
// SPDX-License-Identifier: MIT

#pragma once

#ifndef AUTH_INJECTOR_H_
#define AUTH_INJECTOR_H_

#include <string>
#include <vector>
#include "EuroScopePlugIn.h"
#include "definition.h"

class AuthInjector : public EuroScopePlugIn::CPlugIn {
public:
    AuthInjector(const std::string &logFilePath, const std::string &configFilePath);

    ~AuthInjector() override;

    AuthInjector(const AuthInjector &instance) = delete;

    AuthInjector(const AuthInjector &&instance) = delete;

private:
    // 原始函数指针
    static CURL_EASY_INIT_FUNC sOriginalCurlEasyInit;
    static CURL_EASY_SETOPT_FUNC sOriginalCurlEasySetopt;
    static CURL_EASY_PERFORM_FUNC sOriginalCurlEasyPerform;

    static std::string sTargetUrl;
    static std::string sReplacementUrl;

    // 存储 CURL 句柄和 URL 的映射
    static std::vector<std::pair<void *, std::string>> sCurlHandles;

    static std::ofstream sLogFile;

    void static debugLog(const char *format, ...);

    void uninstallLibCurlHooks();

    bool static installLibCurlHooks();

    // 钩子函数：curl_easy_perform
    int static __cdecl hookedCurlEasyPerform(void *curl);

    // 钩子函数：curl_easy_setopt
    int static __cdecl hookedCurlEasySetopt(void *curl, int option, ...);

    // 钩子函数：curl_easy_init
    void static *__cdecl hookedCurlEasyInit();

};

#endif
