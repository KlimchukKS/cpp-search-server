#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));

    while (str.size() != str.npos && !str.empty()) {
        result.push_back(str.substr(0u, str.find(' ')));
        str.remove_prefix(std::min(str.find(" "), str.size()));
        str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    }

    return result;
}
