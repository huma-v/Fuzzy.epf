#include "ComponentImpl.h"

#include <algorithm>
#include <cwchar>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>

#include <rapidjson/reader.h>
#include <rapidjson/writer.h>

#include "AsymmetricNGramIndex.h"
#include "SymmetricNGramIndex.h"
#include "helpers.h"

namespace HV {

ComponentImpl::ComponentImpl() {
}

ComponentImpl::~ComponentImpl() {
}

bool ADDIN_API ComponentImpl::Init(void* Interface) {
    _interface = static_cast<IAddInDefBaseEx*>(Interface);
    return true;
}

void ADDIN_API ComponentImpl::Done() {
}

bool ADDIN_API ComponentImpl::RegisterExtensionAs(WCHAR_T** wsExtName) {
    *wsExtName = c1PrepareWstr(_mem_manager, PROJECT_NAME);
    return true;
}

bool ADDIN_API ComponentImpl::setMemManager(void* memManager) {
    _mem_manager = static_cast<IMemoryManager*>(memManager);
    return true;
}

long ADDIN_API ComponentImpl::GetInfo() {
    return 2000; // Not actually documented in the docs, but allows us to create multiple instances of same type
}

long ADDIN_API ComponentImpl::GetNProps() {
    return 0;
}

long ADDIN_API ComponentImpl::FindProp(const WCHAR_T* wsPropName) {
    return -1;
}

const WCHAR_T* ADDIN_API ComponentImpl::GetPropName(long lPropNum, long lPropAlias) {
    return nullptr;
}

bool ADDIN_API ComponentImpl::GetPropVal(const long lPropNum, tVariant* pvarPropVal) {
    tVarInit(pvarPropVal);
    return true;
}

bool ADDIN_API ComponentImpl::SetPropVal(const long lPropNum, tVariant* pvarPropVal) {
    return false;
}

bool ADDIN_API ComponentImpl::IsPropReadable(const long lPropNum) {
    return false;
}

bool ADDIN_API ComponentImpl::IsPropWritable(const long lPropNum) {
    return false;
}

long ADDIN_API ComponentImpl::GetNMethods() {
    return std::size(s_methods);
}

long ADDIN_API ComponentImpl::FindMethod(const WCHAR_T* wsMethodName) {
    auto b = std::begin(s_methods);
    auto e = std::end(s_methods);
    auto it = std::find_if(b, e, [&](auto el) { return std::wcscmp(std::get<0>(el), wsMethodName) == 0; });
    return (it != e) ? (it - b) : -1;
}

const WCHAR_T* ADDIN_API ComponentImpl::GetMethodName(const long lMethodNum, const long lMethodAlias) {
    if (lMethodNum >= std::size(s_methods))
        return nullptr;

    return c1PrepareWstr(_mem_manager, std::get<0>(s_methods[lMethodNum]));
}

long ADDIN_API ComponentImpl::GetNParams(const long lMethodNum) {
    return (lMethodNum < std::size(s_methods)) ? std::get<2>(s_methods[lMethodNum]) : 0;
}

bool ADDIN_API ComponentImpl::GetParamDefValue(const long lMethodNum, const long lParamNum, tVariant* pvarParamDefValue) {
    tVarInit(pvarParamDefValue);
    return (lMethodNum < std::size(s_methods));
}

bool ADDIN_API ComponentImpl::HasRetVal(const long lMethodNum) {
    return (lMethodNum < std::size(s_methods) && std::get<1>(s_methods[lMethodNum]));
}

bool ADDIN_API ComponentImpl::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray) {
    if (lMethodNum < std::size(s_methods)) {
        try {
            tVariant retBuf;
            tVarInit(&retBuf);
            bool ret = std::invoke(std::get<3>(s_methods[lMethodNum]), this, &retBuf, paParams, lSizeArray);
            switch (TV_VT(&retBuf)) {
            case VTYPE_PSTR:
            case VTYPE_BLOB: {
                void* ptr = reinterpret_cast<void*>(TV_STR(&retBuf));
                _mem_manager->FreeMemory(&ptr);
                break;
            }
            case VTYPE_PWSTR: {
                void* ptr = reinterpret_cast<void*>(TV_WSTR(&retBuf));
                _mem_manager->FreeMemory(&ptr);
                break;
            }
            }
            return true;
        } catch (const std::exception& e) {
            _interface->AddError(ADDIN_E_FAIL, PROJECT_NAME, convert_mbtowc(e.what()).c_str(), 0);
            return false;
        } catch (...) {
            return false;
        }
    } else {
        return false;
    }
}

bool ADDIN_API ComponentImpl::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
    tVarInit(pvarRetValue);

    if (lMethodNum < std::size(s_methods)) {
        try {
            return std::invoke(std::get<3>(s_methods[lMethodNum]), this, pvarRetValue, paParams, lSizeArray);
        } catch (const std::exception& e) {
            _interface->AddError(ADDIN_E_FAIL, PROJECT_NAME, convert_mbtowc(e.what()).c_str(), 0);
            return false;
        } catch (...) {
            return false;
        }
    } else {
        return false;
    }
}

void ADDIN_API ComponentImpl::SetLocale(const WCHAR_T* wsLocale) {
    //  Noop
}

class FieldsListBuilder: public rapidjson::BaseReaderHandler<rapidjson::UTF16<>, FieldsListBuilder> {
public:
    bool Default() {
        return false;
    }

    bool String(const wchar_t* str, rapidjson::SizeType length, bool copy) {
        if (!_lastVec) {
            return false;
        }
        _lastVec->emplace_back(str, length);
        return true;
    }
    bool StartArray() {
        if (_level == 1) {
            _lastVec = &_result.emplace_back();
        } else if (_level > 1) {
            return false;
        }
        ++_level;
        return true;
    }
    bool EndArray(rapidjson::SizeType elementCount) {
        --_level;
        return true;
    }

    std::vector<std::vector<std::wstring>> getResult();

private:
    std::vector<std::vector<std::wstring>> _result;
    int _level = 0;
    std::vector<std::wstring>* _lastVec;
};

std::vector<std::vector<std::wstring>> FieldsListBuilder::getResult() {
    return std::move(_result);
}

bool ComponentImpl::Reset(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
    if (lSizeArray != 2 || paParams[0].vt != VTYPE_PWSTR || paParams[1].vt != VTYPE_BOOL)
        return false;

    tVarInit(pvarRetValue);

    std::wstring_view strlist_json_view(TV_WSTR(paParams), paParams->wstrLen);
    std::wstring strlist_json(strlist_json_view);
    bool symmetric = TV_BOOL(paParams + 1);

    rapidjson::GenericReader<rapidjson::UTF16<>, rapidjson::UTF16<>> reader;

    rapidjson::GenericStringStream<rapidjson::UTF16<>> ss(strlist_json.c_str());
    FieldsListBuilder handler;
    if (reader.Parse(ss, handler).IsError()) {
        throw std::runtime_error("JSON parse error!");
    }

    auto parseResult = handler.getResult();

    if (symmetric) {
        _index = std::make_unique<SymmetricNGramIndex<3>>(parseResult);
    } else {
        _index = std::make_unique<AsymmetricNGramIndex<3>>(parseResult);
    }

    return true;
}

bool ComponentImpl::Query(tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) {
    if (!pvarRetValue || lSizeArray != 2 || paParams[0].vt != VTYPE_PWSTR || paParams[1].vt != VTYPE_I4)
        return false;

    std::wstring_view qstr(TV_WSTR(paParams), paParams->wstrLen);
    int tiers = TV_I4(paParams + 1);

    rapidjson::GenericStringBuffer<rapidjson::UTF16<>> sb;
    rapidjson::Writer writer(sb);

    writer.StartArray();
    if (_index) {
        auto result = _index->query(std::wstring(qstr));
        float max_score = 0;
        if (result.size())
            max_score = std::get<2>(result[0]);

        int tier = tiers;
        for (const auto &[item_id, field_id, score]: result) {
            if (tiers != 0) { 
                if (score < max_score * 0.9999)
                    --tier;
                if (tier == 0)
                    break;
            }
            writer.StartArray();
            writer.Int64(item_id);
            writer.Int64(field_id);
            writer.Double(score);
            writer.EndArray();
        }
    }
    writer.EndArray();

    c1SetWstr(_mem_manager, pvarRetValue, sb.GetString());
    return true;
}

} // namespace HV
