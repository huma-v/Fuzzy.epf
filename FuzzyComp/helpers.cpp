#define NOMINMAX 

#include "helpers.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

#include <Windows.h> // DO NOT REORDER
#include <WinUser.h> // DO NOT REORDER

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>

namespace HV {

std::wstring convert_mbtowc(std::string_view in) {
    auto len = MultiByteToWideChar(CP_UTF8, 0, in.data(), in.size(), nullptr, 0);
    std::vector<wchar_t> buf(len);
    MultiByteToWideChar(CP_UTF8, 0, in.data(), in.size(), buf.data(), len);
    return {buf.begin(), buf.end()};
}

std::wstring convert_mbtowc(const char* in) {
    return in ? convert_mbtowc(std::string_view(in)) : std::wstring();
}

c1var convert_c1var(tVariant* rawvar) {
    auto type = TV_VT(rawvar) & VTYPE_TYPEMASK;

    switch (type) {
    // no distinction between NULL and EMPTY for now
    case VTYPE_EMPTY:
    case VTYPE_NULL:
        return {};
    // integer types all get converted to int
    case VTYPE_I2:
        return int(TV_I2(rawvar));
    case VTYPE_I4:
        return int(TV_I4(rawvar));
    case VTYPE_UI1:
        return int(TV_UI1(rawvar));
    case VTYPE_BOOL:
        return bool(TV_BOOL(rawvar));
    // floating point types all get converted to float
    case VTYPE_R4:
        return float(TV_R4(rawvar));
    case VTYPE_R8:
        return float(TV_R8(rawvar));
    // TODO: confirm it actually works
    // case VTYPE_DATE: {
    //  static chrono::local_seconds epoch = chrono::local_days(chrono::year(1899) / chrono::month(12) / chrono::day(30));
    //  return epoch + chrono::seconds(chrono::seconds::rep(TV_DATE(rawvar) * (60. * 60. * 24.))); // pretend leap seconds dont exist, like most software
    //}
    // case VTYPE_TM: {
    //  const auto& tm = rawvar->tmVal;
    //  return chrono::local_seconds(chrono::local_days(chrono::year(tm.tm_year) / chrono::month(tm.tm_mon) / chrono::day(tm.tm_mday))) + chrono::hours(tm.tm_hour) + chrono::minutes(tm.tm_min) + chrono::seconds(tm.tm_sec);
    //}
    case VTYPE_PSTR:
        return std::string_view(TV_STR(rawvar), rawvar->strLen);
    case VTYPE_PWSTR:
        return std::wstring_view(TV_WSTR(rawvar), rawvar->wstrLen);
    case VTYPE_BLOB:
        return boost::span<std::byte>(reinterpret_cast<std::byte*> TV_STR(rawvar), rawvar->strLen);
    default:
        throw std::logic_error("unreconized type"); // TODO: incorporate type id into the message (use boost::format?)
    }
}

WCHAR* c1PrepareWstr(IMemoryManager* memMgr, std::wstring_view ws) {
    WCHAR* str;
    memMgr->AllocMemory(reinterpret_cast<void**>(&str), (ws.size() + 1) * sizeof(WCHAR_T));
    std::wmemcpy(str, ws.data(), ws.size());
    str[ws.size()] = 0;
    return str;
}

void c1SetWstr(IMemoryManager* memMgr, tVariant* pvarRetValue, std::wstring_view ws) {
    pvarRetValue->vt = VTYPE_PWSTR;
    pvarRetValue->pwstrVal = c1PrepareWstr(memMgr, ws);
    pvarRetValue->wstrLen = ws.size();
}

std::wstring normalizeString(const std::wstring &string, std::size_t n, bool for_asymmetric_query) {
    std::wstring trimmed = boost::trim_fill_copy_if(string, L" ", [](wchar_t ch) { return !IsCharAlphaNumericW(ch); });
    std::wstring normalized(std::max(n, trimmed.size() + (for_asymmetric_query ? 1 : 2)), L' ');
    std::copy(trimmed.begin(), trimmed.end(), std::next(normalized.begin()));
    boost::to_upper(normalized);

    return normalized;
}

} // namespace HV
