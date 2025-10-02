#include "EuroScopePlugIn.h"
#include "AuthInjector.h"

AuthInjector *pPlugin;

void __declspec (dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn **ppPlugInInstance) {
    *ppPlugInInstance = pPlugin = new AuthInjector();
}

void __declspec (dllexport) EuroScopePlugInExit() {
    delete pPlugin;
}
