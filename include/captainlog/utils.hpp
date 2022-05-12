#pragma once

#include <string>
#include <numeric>

namespace cl::utils {

const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string& s);

std::string rtrim(const std::string& s);
 
std::string trim(const std::string& s);

template<typename T, typename S>
std::string join(const T& items, S separator) {
    auto separator_fold = [&](const std::string& a, const std::string& b) {
        return a + separator + b;
    };

    std::string result;
    if (items.size() > 0) {
        const std::string& first = *items.cbegin();
        result = std::accumulate(std::next(items.cbegin()), items.cend(), first, separator_fold);
    }
    return result;
}

}

