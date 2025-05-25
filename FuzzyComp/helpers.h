#pragma once

#include <boost/core/span.hpp>
#include <string>
#include <string_view>
#include <variant>

#include <types.h>

#include <IMemoryManager.h>

namespace HV {

#define HV_WSTRINGIZE(STR) L#STR
#define HV_XWSTRINGIZE(STR) HV_WSTRINGIZE(STR)

constexpr wchar_t PROJECT_NAME[] = HV_XWSTRINGIZE(HV_PROJECT_NAME);

std::wstring convert_mbtowc(std::string_view in);
std::wstring convert_mbtowc(const char* in);

using c1var = std::variant<std::monostate, int, bool, float, std::string_view, std::wstring_view, boost::span<std::byte>>;

c1var convert_c1var(tVariant* rawvar);

WCHAR* c1PrepareWstr(IMemoryManager* memMgr, std::wstring_view ws);
void c1SetWstr(IMemoryManager* memMgr, tVariant* pvarRetValue, std::wstring_view ws);

std::wstring normalizeString(const std::wstring &string, std::size_t n, bool for_asymmetric_query = false);

} // namespace HV
