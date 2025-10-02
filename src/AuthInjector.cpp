#include <windows.h>
#include <fstream>
#include <detours.h>
#include "AuthInjector.h"

#pragma comment(lib, "detours.lib")

CURL_EASY_INIT_FUNC AuthInjector::originalCurlEasyInit = nullptr;
CURL_EASY_SETOPT_FUNC AuthInjector::originalCurlEasySetopt = nullptr;
CURL_EASY_PERFORM_FUNC AuthInjector::originalCurlEasyPerform = nullptr;

std::vector<std::pair<void *, std::string>> AuthInjector::curlHandles;

std::string AuthInjector::targetUrl = "https://auth.vatsim.net/api/fsd-jwt";
std::string AuthInjector::replacementUrl = INJECTOR_URL;

void *__cdecl AuthInjector::hookedCurlEasyInit() {
    debugLog("hookedCurlEasyInit called");

    if (!originalCurlEasyInit) {
        debugLog("ERROR: originalCurlEasyInit is null");
        return nullptr;
    }

    void *curl = originalCurlEasyInit();
    if (curl) {
        curlHandles.emplace_back(curl, "");
        debugLog("hookedCurlEasyInit: Created curl handle %p", curl);
    } else {
        debugLog("hookedCurlEasyInit: Failed to create curl handle");
    }

    return curl;
}

int __cdecl AuthInjector::hookedCurlEasySetopt(void *curl, int option, ...) {
    debugLog("hookedCurlEasySetopt called: curl=%p, option=%d", curl, option);

    if (!originalCurlEasySetopt) {
        debugLog("ERROR: originalCurlEasySetopt is null");
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

        if (url && strstr(url, targetUrl.c_str()) != nullptr) {
            debugLog("hookedCurlEasySetopt: Target URL detected, replacing with: %s", replacementUrl.c_str());
            actual_url = replacementUrl.c_str();

            // 更新记录
            for (auto &handle: curlHandles) {
                if (handle.first == curl) {
                    handle.second = replacementUrl;
                    debugLog("hookedCurlEasySetopt: Updated handle record");
                    break;
                }
            }
        } else if (url) {
            // 更新记录
            for (auto &handle: curlHandles) {
                if (handle.first == curl) {
                    handle.second = url;
                    break;
                }
            }
        }

        result = originalCurlEasySetopt(curl, option, actual_url);
    } else {
        // 对于其他选项，直接传递参数
        if (option >= 0 && option < 10000) {
            long value = va_arg(args, long);
            result = originalCurlEasySetopt(curl, option, value);
        } else if (option >= 10000 && option < 20000) {
            void *value = va_arg(args, void*);
            result = originalCurlEasySetopt(curl, option, value);
        } else if (option >= 20000 && option < 30000) {
            void *value = va_arg(args, void*);
            result = originalCurlEasySetopt(curl, option, value);
        } else {
            // 默认处理
            long value = va_arg(args, long);
            result = originalCurlEasySetopt(curl, option, value);
        }
    }

            va_end(args);
    debugLog("hookedCurlEasySetopt completed with result: %d", result);
    return result;
}

int __cdecl AuthInjector::hookedCurlEasyPerform(void *curl) {
    debugLog("hookedCurlEasyPerform called: curl=%p", curl);

    if (!originalCurlEasyPerform) {
        debugLog("ERROR: originalCurlEasyPerform is null");
        return 1;
    }

    // 记录请求的 URL
    std::string url;
    for (const auto &handle: curlHandles) {
        if (handle.first == curl) {
            url = handle.second;
            break;
        }
    }

    if (!url.empty()) {
        debugLog("hookedCurlEasyPerform: Performing request to: %s", url.c_str());
    }

    int result = originalCurlEasyPerform(curl);
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
    originalCurlEasyInit = (CURL_EASY_INIT_FUNC) GetProcAddress(hLibCurl, "curl_easy_init");
    originalCurlEasySetopt = (CURL_EASY_SETOPT_FUNC) GetProcAddress(hLibCurl, "curl_easy_setopt");
    originalCurlEasyPerform = (CURL_EASY_PERFORM_FUNC) GetProcAddress(hLibCurl, "curl_easy_perform");

    if (!originalCurlEasyInit) {
        debugLog("ERROR: Failed to get curl_easy_init address");
        return false;
    }
    if (!originalCurlEasySetopt) {
        debugLog("ERROR: Failed to get curl_easy_setopt address");
        return false;
    }
    if (!originalCurlEasyPerform) {
        debugLog("WARNING: Failed to get curl_easy_perform address, continuing without hooking it");
    }

    debugLog("Original function addresses:");
    debugLog("  curl_easy_init: 0x%p", originalCurlEasyInit);
    debugLog("  curl_easy_setopt: 0x%p", originalCurlEasySetopt);
    debugLog("  curl_easy_perform: 0x%p", originalCurlEasyPerform);

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
    if (originalCurlEasyInit) {
        result = DetourAttach(&(PVOID &) originalCurlEasyInit, hookedCurlEasyInit);
        if (result != NO_ERROR) {
            debugLog("ERROR: DetourAttach curl_easy_init failed: %d", result);
            DetourTransactionAbort();
            return false;
        }
        debugLog("Successfully attached curl_easy_init hook");
    }

    if (originalCurlEasySetopt) {
        result = DetourAttach(&(PVOID &) originalCurlEasySetopt, hookedCurlEasySetopt);
        if (result != NO_ERROR) {
            debugLog("ERROR: DetourAttach curl_easy_setopt failed: %d", result);
            DetourTransactionAbort();
            return false;
        }
        debugLog("Successfully attached curl_easy_setopt hook");
    }

    if (originalCurlEasyPerform) {
        result = DetourAttach(&(PVOID &) originalCurlEasyPerform, hookedCurlEasyPerform);
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
    if (originalCurlEasyInit) {
        DetourDetach(&(PVOID &) originalCurlEasyInit, hookedCurlEasyInit);
        debugLog("Detached curl_easy_init hook");
    }

    if (originalCurlEasySetopt) {
        DetourDetach(&(PVOID &) originalCurlEasySetopt, hookedCurlEasySetopt);
        debugLog("Detached curl_easy_setopt hook");
    }

    if (originalCurlEasyPerform) {
        DetourDetach(&(PVOID &) originalCurlEasyPerform, hookedCurlEasyPerform);
        debugLog("Detached curl_easy_perform hook");
    }

    // 提交事务
    DetourTransactionCommit();

    // 清理
    curlHandles.clear();

    debugLog("=== uninstallLibCurlHooks completed ===");
}

AuthInjector::AuthInjector() :
        CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION,
                PLUGIN_DEVELOPER, PLUGIN_COPYRIGHT) {
    debugLog("=== AuthInjector constructor started ===");
    debugLog("Target URL: %s", targetUrl.c_str());
    debugLog("Replacement URL: %s", replacementUrl.c_str());
    if (installLibCurlHooks()) {
        DisplayUserMessage(PLUGIN_DISPLAY_NAME, "Plugin", "libcurl.dll found and hooked",
                           true, false, false, false, false);
    }
    debugLog("=== AuthInjector constructor completed ===");
}

AuthInjector::~AuthInjector() {
    debugLog("=== AuthInjector destructor started ===");
    uninstallLibCurlHooks();
    debugLog("=== AuthInjector destructor completed ===");
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

    std::ofstream logfile(LOG_FILE_PATH, std::ios::app);
    if (logfile.is_open()) {
        logfile << buffer << std::endl;
        logfile.close();
    }
}
