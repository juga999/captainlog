#include <sstream>
#include <iomanip>
#include <regex>

#include <captainlog/utils.hpp>

using tl::make_unexpected;

namespace cl::utils {

std::string ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s)
{
    return rtrim(ltrim(s));
}

const std::regex hh_mm_time_regexp(
    "(\\d+)[:\\.](\\d+)",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

expected<std::string, std::string> normalize_hh_mm_time(const std::string& input)
{
    std::smatch m;
    std::regex_match(input, m, hh_mm_time_regexp);

    if (m.empty()) {
        return make_unexpected("Unknown time format: " + input);
    }

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << m[1].str();
    oss << ':';
    oss << std::setw(2) << std::setfill('0') << m[2].str();

    return oss.str();
}

const std::regex yyyy_mm_dd_date_regexp(
    "(\\d+)[-\\.](\\d+)[-\\.](\\d+)",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

expected<std::string, std::string> normalize_yyyy_mm_dd_date(const std::string& input)
{
    std::smatch m;
    std::regex_match(input, m, yyyy_mm_dd_date_regexp);

    if (m.empty()) {
        return make_unexpected("Unknown date format: " + input);
    }

    std::ostringstream oss;
    oss << m[1].str();
    oss << '-';
    oss << std::setw(2) << std::setfill('0') << m[2].str();
    oss << '-';
    oss << std::setw(2) << std::setfill('0') << m[3].str();

    return oss.str();
}

const std::regex yyyy_mm_dd_hh_mm_date_time_regexp(
    "(\\d+)[-\\.](\\d+)[-\\.](\\d+)\\s+(\\d+)[:\\.](\\d+)",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

expected<std::string, std::string> normalize_yyyy_mm_dd_hh_mm_date_time(const std::string& input)
{
    std::smatch m;
    std::regex_match(input, m, yyyy_mm_dd_hh_mm_date_time_regexp);

    if (m.empty()) {
        return make_unexpected("Unknown date format: " + input);
    }

    std::ostringstream oss;
    oss << m[1].str();
    oss << '-';
    oss << std::setw(2) << std::setfill('0') << m[2].str();
    oss << '-';
    oss << std::setw(2) << std::setfill('0') << m[3].str();
    oss << ' ';
    oss << std::setw(2) << std::setfill('0') << m[4].str();
    oss << ':';
    oss << std::setw(2) << std::setfill('0') << m[5].str();

    return oss.str();
}

}
