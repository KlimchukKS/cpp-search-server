#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <set>

std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(static_cast<const std::basic_string<char>>(str));
        }
    }
    return non_empty_strings;
}
