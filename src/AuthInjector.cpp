// Copyright (c) 2025-2026 Half_nothing
// SPDX-License-Identifier: MIT

#include <windows.h>
#include <fstream>
#include <detours.h>
#include <sstream>

#include "AuthInjector.h"

CURL_EASY_INIT_FUNC AuthInjector::sOriginalCurlEasyInit = nullptr;
CURL_EASY_SETOPT_FUNC AuthInjector::sOriginalCurlEasySetopt = nullptr;
CURL_EASY_PERFORM_FUNC AuthInjector::sOriginalCurlEasyPerform = nullptr;

std::vector<std::pair<void *, std::string>> AuthInjector::sCurlHandles;

std::string AuthInjector::sTargetUrl = "https://auth.vatsim.net/api/fsd-jwt";
std::string AuthInjector::sReplacementUrl = INJECTOR_URL;
std::ofstream AuthInjector::sLogFile;

void *__cdecl AuthInjector::hookedCurlEasyInit() {
    debugLog("hookedCurlEasyInit called");

    if (!sOriginalCurlEasyInit) {
        debugLog("ERROR: sOriginalCurlEasyInit is null");
        return nullptr;
    }

    void *curl = sOriginalCurlEasyInit();
    if (curl) {
        sCurlHandles.emplace_back(curl, "");
        debugLog("hookedCurlEasyInit: Created curl handle %p", curl);
    } else {
        debugLog("hookedCurlEasyInit: Failed to create curl handle");
    }

    return curl;
}

int __cdecl AuthInjector::hookedCurlEasySetopt(void *curl, int option, ...) {
    debugLog("hookedCurlEasySetopt called: curl=%p, option=%d", curl, option);

    if (!sOriginalCurlEasySetopt) {
        debugLog("ERROR: sOriginalCurlEasySetopt is null");
        return 1;
    }

    va_list args;
            va_start(args, option);

    int result = 0;

    // 检查是否是设置 URL 的选项
    if (option == CURLOPT_URL) {
        const char *url = va_arg(args, const char*);
        debugLog("hookedCurlEasySetopt: URL option, original URL: %s", url ? url : "NULL");

        const char *actual_url = url;

        if (url && strstr(url, sTargetUrl.c_str()) != nullptr) {
            debugLog("hookedCurlEasySetopt: Target URL detected, replacing with: %s", sReplacementUrl.c_str());
            actual_url = sReplacementUrl.c_str();

            // 更新记录
            for (auto &handle: sCurlHandles) {
                if (handle.first == curl) {
                    handle.second = sReplacementUrl;
                    debugLog("hookedCurlEasySetopt: Updated handle record");
                    break;
                }
            }
        } else if (url) {
            // 更新记录
            for (auto &handle: sCurlHandles) {
                if (handle.first == curl) {
                    handle.second = url;
                    break;
                }
            }
        }

        result = sOriginalCurlEasySetopt(curl, option, actual_url);
    } else {
        // 对于其他选项，直接传递参数
        if (option >= 0 && option < 10000) {
            long value = va_arg(args, long);
            result = sOriginalCurlEasySetopt(curl, option, value);
        } else if (option >= 10000 && option < 20000) {
            void *value = va_arg(args, void*);
            result = sOriginalCurlEasySetopt(curl, option, value);
        } else if (option >= 20000 && option < 30000) {
            void *value = va_arg(args, void*);
            result = sOriginalCurlEasySetopt(curl, option, value);
        } else {
            // 默认处理
            long value = va_arg(args, long);
            result = sOriginalCurlEasySetopt(curl, option, value);
        }
    }

            va_end(args);
    debugLog("hookedCurlEasySetopt completed with result: %d", result);
    return result;
}

int __cdecl AuthInjector::hookedCurlEasyPerform(void *curl) {
    debugLog("hookedCurlEasyPerform called: curl=%p", curl);

    if (!sOriginalCurlEasyPerform) {
        debugLog("ERROR: sOriginalCurlEasyPerform is null");
        return 1;
    }

    // 记录请求的 URL
    std::string url;
    for (const auto &handle: sCurlHandles) {
        if (handle.first == curl) {
            url = handle.second;
            break;
        }
    }

    if (!url.empty()) {
        debugLog("hookedCurlEasyPerform: Performing request to: %s", url.c_str());
    }

    int result = sOriginalCurlEasyPerform(curl);
    debugLog("hookedCurlEasyPerform completed with result: %d", result);
    return result;
}

// 安装钩子
bool AuthInjector::installLibCurlHooks() {
    debugLog("=== installLibCurlHooks started ===");

    // 获取 libcurl 模块
    HMODULE hLibCurl = GetModuleHandle("libcurl.dll");
    if (!hLibCurl) {
        debugLog("libcurl.dll not loaded, attempting to load...");
        hLibCurl = LoadLibrary("libcurl.dll");
        if (!hLibCurl) {
            debugLog("ERROR: Failed to load libcurl.dll");
            return false;
        }
    }

    debugLog("libcurl.dll loaded at: 0x%p", hLibCurl);

    // 获取函数地址
    sOriginalCurlEasyInit = (CURL_EASY_INIT_FUNC) GetProcAddress(hLibCurl, "curl_easy_init");
    sOriginalCurlEasySetopt = (CURL_EASY_SETOPT_FUNC) GetProcAddress(hLibCurl, "curl_easy_setopt");
    sOriginalCurlEasyPerform = (CURL_EASY_PERFORM_FUNC) GetProcAddress(hLibCurl, "curl_easy_perform");

    if (!sOriginalCurlEasyInit) {
        debugLog("ERROR: Failed to get curl_easy_init address");
        return false;
    }
    if (!sOriginalCurlEasySetopt) {
        debugLog("ERROR: Failed to get curl_easy_setopt address");
        return false;
    }
    if (!sOriginalCurlEasyPerform) {
        debugLog("WARNING: Failed to get curl_easy_perform address, continuing without hooking it");
    }

    debugLog("Original function addresses:");
    debugLog("  curl_easy_init: 0x%p", sOriginalCurlEasyInit);
    debugLog("  curl_easy_setopt: 0x%p", sOriginalCurlEasySetopt);
    debugLog("  curl_easy_perform: 0x%p", sOriginalCurlEasyPerform);

    // 开始 Detours 事务
    LONG result = DetourTransactionBegin();
    if (result != NO_ERROR) {
        debugLog("ERROR: DetourTransactionBegin failed: %d", result);
        return false;
    }

    result = DetourUpdateThread(GetCurrentThread());
    if (result != NO_ERROR) {
        debugLog("ERROR: DetourUpdateThread failed: %d", result);
        DetourTransactionAbort();
        return false;
    }

    // 安装钩子
    if (sOriginalCurlEasyInit) {
        result = DetourAttach(&(PVOID &) sOriginalCurlEasyInit, hookedCurlEasyInit);
        if (result != NO_ERROR) {
            debugLog("ERROR: DetourAttach curl_easy_init failed: %d", result);
            DetourTransactionAbort();
            return false;
        }
        debugLog("Successfully attached curl_easy_init hook");
    }

    if (sOriginalCurlEasySetopt) {
        result = DetourAttach(&(PVOID &) sOriginalCurlEasySetopt, hookedCurlEasySetopt);
        if (result != NO_ERROR) {
            debugLog("ERROR: DetourAttach curl_easy_setopt failed: %d", result);
            DetourTransactionAbort();
            return false;
        }
        debugLog("Successfully attached curl_easy_setopt hook");
    }

    if (sOriginalCurlEasyPerform) {
        result = DetourAttach(&(PVOID &) sOriginalCurlEasyPerform, hookedCurlEasyPerform);
        if (result != NO_ERROR) {
            debugLog("WARNING: DetourAttach curl_easy_perform failed: %d", result);
            // 不中止事务，继续执行
        } else {
            debugLog("Successfully attached curl_easy_perform hook");
        }
    }

    // 提交事务
    result = DetourTransactionCommit();
    if (result != NO_ERROR) {
        debugLog("ERROR: DetourTransactionCommit failed: %d", result);
        return false;
    }

    debugLog("=== installLibCurlHooks completed successfully ===");
    return true;
}

// 卸载钩子
void AuthInjector::uninstallLibCurlHooks() {
    debugLog("=== uninstallLibCurlHooks started ===");

    // 开始 Detours 事务
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // 卸载钩子
    if (sOriginalCurlEasyInit) {
        DetourDetach(&(PVOID &) sOriginalCurlEasyInit, hookedCurlEasyInit);
        debugLog("Detached curl_easy_init hook");
    }

    if (sOriginalCurlEasySetopt) {
        DetourDetach(&(PVOID &) sOriginalCurlEasySetopt, hookedCurlEasySetopt);
        debugLog("Detached curl_easy_setopt hook");
    }

    if (sOriginalCurlEasyPerform) {
        DetourDetach(&(PVOID &) sOriginalCurlEasyPerform, hookedCurlEasyPerform);
        debugLog("Detached curl_easy_perform hook");
    }

    // 提交事务
    DetourTransactionCommit();

    // 清理
    sCurlHandles.clear();

    debugLog("=== uninstallLibCurlHooks completed ===");
}

AuthInjector::AuthInjector(const std::string &logFilePath, const std::string &configFilePath) :
        CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION,
                PLUGIN_DEVELOPER, PLUGIN_COPYRIGHT) {
    sLogFile.open(logFilePath, std::ios::out | std::ios::app);
    std::ifstream injectorUrlFile(configFilePath, std::ios::in);
    if (injectorUrlFile.is_open()) {
        std::ostringstream oss;
        oss << injectorUrlFile.rdbuf();
        sReplacementUrl = oss.str();
        injectorUrlFile.close();
        std::string message("Replacement url change to ");
        message += sReplacementUrl;
        debugLog(message.c_str());
        DisplayUserMessage(PLUGIN_DISPLAY_NAME, "Plugin", message.c_str(), true, false, false, false, false);
    }
    debugLog("=== AuthInjector constructor started ===");
    debugLog("Target URL: %s", sTargetUrl.c_str());
    debugLog("Replacement URL: %s", sReplacementUrl.c_str());
    if (installLibCurlHooks()) {
        DisplayUserMessage(PLUGIN_DISPLAY_NAME, "Plugin", "Plugin ready and hook finish, no error", true, false, false, false, false);
    }
    debugLog("=== AuthInjector constructor completed ===");
}

AuthInjector::~AuthInjector() {
    debugLog("=== AuthInjector destructor started ===");
    uninstallLibCurlHooks();
    debugLog("=== AuthInjector destructor completed ===");
    sLogFile.close();
}

// 调试日志
void AuthInjector::debugLog(const char *format, ...) {
    char buffer[1024];
    va_list args;
            va_start(args, format);
    vsprintf_s(buffer, sizeof(buffer), format, args);
            va_end(args);

    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");

    if (sLogFile.good()) {
        sLogFile << buffer << std::endl;
    }
}
