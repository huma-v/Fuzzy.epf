#pragma once

#include <map>
#include <array>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "AbstractIndex.h"
#include "NGramVector.h"

namespace HV {

template <std::size_t N>
class AsymmetricNGramIndex: public AbstractIndex {
public:
    AsymmetricNGramIndex(const std::vector<std::vector<std::wstring>> &fieldsList);

    ResultType query(const std::wstring &queryString) const override;

private:
    std::unordered_multimap<std::array<wchar_t, N>, std::tuple<std::size_t, std::size_t, float>, boost::hash<std::array<wchar_t, N>>> _idx;
};

template <std::size_t N>
AsymmetricNGramIndex<N>::AsymmetricNGramIndex(const std::vector<std::vector<std::wstring>> &fieldsList) {
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
            for (const auto &[ngram, weight]: NGramVector<N>::fromString(str)) {
                _idx.insert({ngram, {fieldId, strId, weight}});
            }

            ++strId;
        }
        ++fieldId;
    }
}

template <std::size_t N>
AbstractIndex::ResultType AsymmetricNGramIndex<N>::query(const std::wstring &queryString) const {
    auto query_vec = NGramVector<N>::fromString(queryString, true);

    float query_vec_len = std::sqrt(
        std::accumulate(
            query_vec.begin(),
            query_vec.end(),
            0.f,
            [](auto acc, auto x) { return acc + std::pow(x.second, 2.f); }));

    // { (item_id, field_id): [(ngram, weight)..]... }
    std::unordered_multimap<std::pair<std::size_t, std::size_t>, std::pair<std::array<wchar_t, N>, float>, boost::hash<std::pair<std::size_t, std::size_t>>> unsorted_result;
    for (const auto &[ngram, weight]: query_vec) {
        for (auto [it, end] = _idx.equal_range(ngram); it != end; ++it) {
            const auto &[field_id, item_id, idx_weight] = it->second;
            unsorted_result.insert({{item_id, field_id}, {ngram, std::min(idx_weight, weight)}});
        }
    }

    // { weight: (item_id, field_id)... }
    std::multimap<float, std::pair<std::size_t, std::size_t>, std::greater<float>> sorted_result;
    if (auto it = unsorted_result.begin(); it != unsorted_result.end()) {
        auto it_copy = it;

        float result_len = 0.f;
        auto prev = it->first;
        for (;; ++it) {
            bool do_continue = (it != unsorted_result.end());
            if (!do_continue || it->first != prev) { // we stepped into a different item or end of the list
                result_len = std::sqrt(result_len);
                // looping over the previous item's ngrams again using it_copy
                float similarity_score = 0.f;
                for (; it_copy != it; ++it_copy) {
                    similarity_score += std::pow(std::min(it_copy->second.second, query_vec[it_copy->second.first]), 2.f);
                }
                similarity_score /= result_len * query_vec_len;
                sorted_result.emplace(similarity_score, prev);
                result_len = 0;
                if (!do_continue) {
                    break;
                }
            }
            result_len += std::pow(it->second.second, 2.f);
            prev = it->first;
        }
    }

    std::vector<std::tuple<std::size_t, std::size_t, float>> result;
    result.reserve(sorted_result.size());
    std::unordered_set<std::size_t> dedupSet;
    for (const auto &item: sorted_result) {
        const auto [_, inserted] = dedupSet.emplace(item.second.first);
        if (inserted) {
            result.emplace_back(item.second.first, item.second.second, item.first);
        }
    }

    result.shrink_to_fit();
    return result;
}

} // namespace HV
