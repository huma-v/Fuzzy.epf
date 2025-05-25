#pragma once

#include <map>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "AbstractIndex.h"
#include "NGramVector.h"

namespace HV {

template <std::size_t N>
class SymmetricNGramIndex: public AbstractIndex {
public:
    SymmetricNGramIndex(const std::vector<std::vector<std::wstring>> &fieldsList);

    ResultType query(const std::wstring &queryString) const override;

private:
    std::unordered_multimap<std::array<wchar_t, N>, std::tuple<std::size_t, std::size_t, float>, boost::hash<std::array<wchar_t, N>>> _idx;
};

template <std::size_t N>
SymmetricNGramIndex<N>::SymmetricNGramIndex(const std::vector<std::vector<std::wstring>> &fieldsList) {
    auto [minSize, maxSize] = std::accumulate(
        fieldsList.begin(),
        fieldsList.end(),
        std::pair<std::size_t, std::size_t>{std::numeric_limits<std::size_t>::max(), 0},
        [](auto acc, auto x) {
            std::size_t size = x.size();
            return std::pair{std::min(acc.first, size), std::max(acc.second, size)};
        });

    if (minSize != maxSize) {
        throw std::runtime_error("Fields vectors must be of same length!");
    }

    std::size_t fieldId = 0;
    for (auto &fieldsArray: fieldsList) {
        std::size_t strId = 0;
        for (const auto &str: fieldsArray) {
            auto nGramVec = NGramVector<N>::fromString(str);
            nGramVec.normalize();
            for (const auto &[ngram, weight]: nGramVec) {
                _idx.insert({ngram, {fieldId, strId, weight}});
            }

            ++strId;
        }
        ++fieldId;
    }
}

template <std::size_t N>
AbstractIndex::ResultType SymmetricNGramIndex<N>::query(const std::wstring &queryString) const {
    auto query_vec = NGramVector<N>::fromString(queryString);
    query_vec.normalize();

    // { (item_id, field_id): weight }
    std::unordered_map<std::pair<std::size_t, std::size_t>, float, boost::hash<std::pair<std::size_t, std::size_t>>> unsorted_result;
    for (const auto &[ngram, weight]: query_vec) {
        for (auto [it, end] = _idx.equal_range(ngram); it != end; ++it) {
            const auto &[field_id, item_id, idx_weight] = it->second;
            auto [ur_it, _] = unsorted_result.try_emplace({item_id, field_id}, 0.f);
            ur_it->second += weight * idx_weight;
        }
    }

    std::vector<std::tuple<std::size_t, std::size_t, float>> sorted_result;
    for (const auto &item: unsorted_result) {
        sorted_result.emplace_back(item.first.first, item.first.second, item.second);
    }
    std::sort(sorted_result.begin(), sorted_result.end(), [](const auto &lhs, const auto &rhs) { return std::get<2>(lhs) > std::get<2>(rhs); });

    std::vector<std::tuple<std::size_t, std::size_t, float>> result;
    result.reserve(unsorted_result.size());
    std::unordered_set<std::size_t> dedupSet;
    for (const auto &item: sorted_result) {
        const auto [_, inserted] = dedupSet.emplace(std::get<0>(item));
        if (inserted) {
            result.push_back(item);
        }
    }

    result.shrink_to_fit();
    return result;
}

} // namespace HV
