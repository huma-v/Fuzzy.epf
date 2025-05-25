#include <type_traits>

#include <ComponentBase.h>

#include "helpers.h"
#include "ComponentImpl.h"

#include "version.h"


static_assert(std::is_same_v<WCHAR_T, wchar_t>);


const WCHAR_T* GetClassNames() {
    return HV::PROJECT_NAME;
}


long GetClassObject(const WCHAR_T* wsName, IComponentBase** pIntf) {
    if (!*pIntf) {
        if (std::wcscmp(wsName, HV::PROJECT_NAME) == 0) {
            try {
                *pIntf = new HV::ComponentImpl;
                return 1;
            } catch (...) {
                return 0;
            }
        }
    }
    return 0;
}


long DestroyObject(const IComponentBase** pIntf) {
    if (!*pIntf) return 1;

    delete *pIntf; *pIntf = nullptr;
    return 0;
}


AppCapabilities SetPlatformCapabilities(const AppCapabilities caps) {
    return eAppCapabilitiesLast;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
