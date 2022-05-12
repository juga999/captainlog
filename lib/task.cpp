#define __STDC_WANT_LIB_EXT1__ 1
#define _XOPEN_SOURCE

#include <sstream>
#include <iomanip>
#include <ctime>
#include <regex>

#include <captainlog/utils.hpp>
#include <captainlog/task.hpp>

using tl::make_unexpected;

namespace cl {

const std::regex date_time_regexp(
    "(\\d+)-(\\d+)-(\\d+) (\\d+):(\\d+):(\\d+)",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

static expected<time_t, std::string> parse_date_time_string(const std::string& value, const std::string& fmt)
{
    std::tm t = {};
    std::istringstream iss(value);
    iss >> std::get_time(&t, fmt.c_str());
    if (iss.fail()) {
        return make_unexpected("Invalid date time value: " + value);
    }
    time_t result = ::mktime(&t);
    if (result == static_cast<time_t>(-1)) {
        return make_unexpected(value + " cannot be represented as a calendar time");
    }
    return result;
}

static BrokenDownYearMonthDayHourMinute broken_down_time(const std::string& value)
{
    std::smatch m;
    std::regex_match(value, m, date_time_regexp);

    return std::make_tuple(m[1].str(), m[2].str(), m[3].str(), m[4].str(), m[5].str());
}

expected<TaskSchedule, std::string> TaskSchedule::create(
    const std::string& start_str, 
    const std::string& stop_str)
{
    auto expected_start = parse_date_time_string(start_str, cl::DATE_TIME_FORMAT);
    if (!expected_start) {
        return make_unexpected(expected_start.error());
    }
    auto expected_stop = parse_date_time_string(stop_str, cl::DATE_TIME_FORMAT);
    if (!expected_stop) {
        return make_unexpected(expected_stop.error());
    }
    if (expected_start.value() >= expected_stop.value()) {
        return make_unexpected("Invalid chronology: " + start_str + " -> " + stop_str);
    }

    expected<TaskSchedule, std::string> result({start_str, stop_str});
    return result;
}

expected<TaskSchedule, std::string> TaskSchedule::create(
        const std::string& task_date,
        const std::string& start_time_str,
        const std::string& stop_time_str)
{
    std::string start_str = task_date + " " + start_time_str + ":00";
    std::string stop_str = task_date + " " + stop_time_str + ":00";

    return TaskSchedule::create(start_str, stop_str);
}

unsigned long TaskSchedule::duration_sec() const
{
    auto expected_start = parse_date_time_string(m_start_str, cl::DATE_TIME_FORMAT);
    auto expected_stop = parse_date_time_string(m_stop_str, cl::DATE_TIME_FORMAT);
    unsigned long result = expected_stop.value() - expected_start.value();
    return result;
}

BrokenDownYearMonthDayHourMinute TaskSchedule::broken_down_start_time() const
{
    return broken_down_time(m_start_str);
}

BrokenDownYearMonthDayHourMinute TaskSchedule::broken_down_end_time() const
{
    return broken_down_time(m_stop_str);
}

void TaskSchedule::swap(TaskSchedule& other)
{
    m_start_str.swap(other.m_start_str);
    m_stop_str.swap(other.m_stop_str);
}

bool TaskSchedule::operator==(const TaskSchedule& other) const
{
    return (
        (m_start_str.compare(other.m_start_str) == 0) &&
        (m_stop_str.compare(other.m_stop_str) == 0));
}

std::ostream& operator<<(std::ostream& os, const TaskSchedule& schedule)
{
    os << schedule.m_start_str;
    os << " -> ";
    os << schedule.m_stop_str;
    return os;
}

Task::Task(
    const TaskSchedule& schedule,
    const std::string& project,
    const std::string& description,
    const std::string& tags,
    const std::string& comment)
    : m_id(0)
    , m_schedule(schedule)
    , m_project(project)
    , m_description(description)
    , m_comment(comment)
{
    tags_from_string(tags);
}

Task::Task(Task&& other)
    : m_id(other.m_id)
    , m_schedule(std::move(other.m_schedule))
    , m_project(std::move(other.m_project))
    , m_description(std::move(other.m_description))
    , m_comment(std::move(other.m_comment))
    , m_tags(std::move(other.m_tags))
{
}

Task::Task(int id, Task&& other)
    : Task(std::move(other))
{
    m_id = id;
}

void Task::tags_from_string(const std::string& str)
{
    std::istringstream input(str);
    std::string item;
    while(std::getline(input, item, ',')) {
        m_tags.insert(cl::utils::trim(item));
    }
}

std::string Task::joined_tags() const
{
    return cl::utils::join(m_tags, ',');
}

void Task::swap(Task& other)
{
    std::swap(m_id, other.m_id);
    m_schedule.swap(other.m_schedule);
    m_project.swap(other.m_project);
    m_description.swap(other.m_description);
    m_comment.swap(other.m_comment);
    m_tags.swap(other.m_tags);
}

bool Task::operator==(const Task& other) const
{
    return (
        (m_schedule == other.m_schedule) &&
        (m_project.compare(other.m_project) == 0) &&
        (m_description.compare(other.m_description) == 0) &&
        (m_comment.compare(other.m_comment) == 0) &&
        (m_tags == other.m_tags));
}

void swap(Task& t1, Task& t2)
{
    t1.swap(t2);
}

std::ostream& operator<<(std::ostream& os, const Task& task)
{
    os << "@" << task.id() << " ";
    os << "[" << task.project() << "] ";
    os << task.schedule();
    os << " : " << task.description();
    if (task.tags().size() > 0) {
        os << " (" << task.joined_tags() << ")";
    }
    return os;
}

}
