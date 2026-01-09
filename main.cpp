#include <filesystem>
#include "EuroScopePlugIn.h"
#include "AuthInjector.h"

AuthInjector *pPlugin;
HMODULE pluginModule = nullptr;

inline std::string GetRealFileName(const std::string &path);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            pluginModule = hModule;
            break;
    }
    return TRUE;
}

void __declspec (dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn **ppPlugInInstance) {
    *ppPlugInInstance = pPlugin = new AuthInjector(GetRealFileName(LOG_FILE_PATH), GetRealFileName(INJECTOR_URL_FILENAME));
}

void __declspec (dllexport) EuroScopePlugInExit() {
    delete pPlugin;
}

inline std::string GetRealFileName(const std::string &path) {
    namespace fs = std::filesystem;
    if (path.empty()) {
        return path;
    }
    fs::path pFilename = path;
    if (!fs::is_regular_file(pFilename) && pluginModule != nullptr) {
        TCHAR pBuffer[MAX_PATH] = {0};
        GetModuleFileName(pluginModule, pBuffer, sizeof(pBuffer) / sizeof(TCHAR) - 1);
        fs::path dllPath = pBuffer;
        pFilename = dllPath.parent_path() / path;
    }
    return pFilename.string();
}
