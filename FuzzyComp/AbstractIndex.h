#pragma once

#include <tuple>
#include <vector>
#include <string>

class AbstractIndex {
public:
    // [(item_id, field_id, score), ...]
    using ResultType = std::vector<std::tuple<std::size_t, std::size_t, float>>;

    virtual ~AbstractIndex() {}

    virtual ResultType query(const std::wstring &queryString) const = 0;
};
