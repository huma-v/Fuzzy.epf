#pragma once

#include <memory>
#include <tuple>

#define NOMINMAX

#include <AddInDefBase.h>
#include <ComponentBase.h>
#include <IMemoryManager.h>

#include "AbstractIndex.h"

namespace HV {

class ComponentImpl: public IComponentBase {
public:
    ComponentImpl();
    ~ComponentImpl();

    bool ADDIN_API Init(void* Interface) override;
    void ADDIN_API Done() override;
    bool ADDIN_API RegisterExtensionAs(WCHAR_T** wsExtName);
    bool ADDIN_API setMemManager(void* memManager);
    long ADDIN_API GetInfo() override;

    // Properties
    long ADDIN_API GetNProps() override;
    long ADDIN_API FindProp(const WCHAR_T* wsPropName) override;
    const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias) override;
    bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
    bool ADDIN_API SetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
    bool ADDIN_API IsPropReadable(const long lPropNum) override;
    bool ADDIN_API IsPropWritable(const long lPropNum) override;

    // Methods
    long ADDIN_API GetNMethods() override;
    long ADDIN_API FindMethod(const WCHAR_T* wsMethodName);
    const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias) override;
    long ADDIN_API GetNParams(const long lMethodNum);
    bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue) override;
    bool ADDIN_API HasRetVal(const long lMethodNum);
    bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray);
    bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);

    // Locale
    void ADDIN_API SetLocale(const WCHAR_T* wsLocale);

    // Custom methods
    bool Reset(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
    bool Query(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);

private:
    IAddInDefBaseEx* _interface = nullptr;
    IMemoryManager* _mem_manager = nullptr;

    std::unique_ptr<AbstractIndex> _index;

    // [(name, hasRetVal, nParams, pointer), ...]
    static constexpr std::tuple<const wchar_t*, bool, std::size_t, bool (ComponentImpl::*)(tVariant*, tVariant*, const long)> s_methods[] = {
        {L"Reset", false, 2, &ComponentImpl::Reset}, // Reset(strings, symmetric)
        {L"Query", true, 2, &ComponentImpl::Query}}; // Query(query_string, num_tiers)
};

} // namespace HV