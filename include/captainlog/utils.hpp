#pragma once

#include <iostream>
#include <string>
#include <numeric>

#include <captainlog/expected.hpp>

using tl::expected;

namespace cl::utils {

const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string& s);

std::string rtrim(const std::string& s);
 
std::string trim(const std::string& s);

template<class T, typename S>
std::string join(const T& items, const S& separator) {
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

template<class T, typename S>
void split(const std::string& str, const S& separator, T& dest)
{
    std::istringstream input(str);
    std::string item;
    while(std::getline(input, item, separator)) {
        dest.insert(cl::utils::trim(item));
    }
}

expected<std::string, std::string> normalize_hh_mm_time(const std::string& input);

expected<std::string, std::string> normalize_yyyy_mm_dd_date(const std::string& input);

expected<std::string, std::string> normalize_yyyy_mm_dd_hh_mm_date_time(const std::string& input);

}
