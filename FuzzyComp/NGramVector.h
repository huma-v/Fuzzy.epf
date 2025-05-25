#pragma once

#include <string>
#include <ostream>
#include <iterator>

#include <boost/container_hash/hash.hpp>

#include "helpers.h"

namespace HV {

template <std::size_t N>
class NGramVector {
public:
    auto begin() const;

    auto end() const;

    float operator[](const std::array<wchar_t, N> &key) const;

    void normalize();

    static NGramVector fromString(const std::wstring &source, bool for_asymmetric_query = false);

    friend std::wostream &operator<<(std::wostream &os, const NGramVector<N> &vec) {
        os << L"[";
        std::size_t count = 0;
        for (auto &kv: vec._map) {
            if (count != 0) {
                os << L", ";
            }
            os << L"\"";
            std::copy(kv.first.begin(), kv.first.end(),
                      std::ostream_iterator<wchar_t, wchar_t>(os));
            os << L"\":" << kv.second;
            ++count;
        }
        os << L"]";
        return os;
    }

private:
    std::unordered_map<std::array<wchar_t, N>, float, boost::hash<std::array<wchar_t, N>>> _map;
};

template <std::size_t N>
auto NGramVector<N>::begin() const { return _map.cbegin(); }

template <std::size_t N>
auto NGramVector<N>::end() const { return _map.cend(); }

template <std::size_t N>
float NGramVector<N>::operator[](const std::array<wchar_t, N> &key) const {
    return _map.at(key);
}

template <std::size_t N>
void NGramVector<N>::normalize() {
    float mag = 0.f;
    for (const auto &[k, v]: _map) {
        mag += std::pow(v, 2.f);
    }
    mag = std::pow(mag, 0.5f);
    float invMag = 1.f / mag;
    for (auto &[k, v]: _map) {
        v *= invMag;
    }
}

template <std::size_t N>
NGramVector<N> NGramVector<N>::fromString(const std::wstring &source, bool for_asymmetric_query) {
    auto normalized = normalizeString(source, N, for_asymmetric_query);
    NGramVector result;
    for (std::size_t i = 0, len = normalized.size(); i < len - N + 1; ++i) {
        std::array<wchar_t, N> key;
        std::copy(normalized.begin() + i, normalized.begin() + i + N, key.begin());

        auto [it, _] = result._map.try_emplace(key, 0.f);
        it->second += 1.f;
    }

    return result;
}

} // namespace HV
