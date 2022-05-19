#pragma once

#include <tuple>
#include <set>
#include <iostream>

#include <captainlog/common.hpp>
#include <captainlog/expected.hpp>

using tl::expected;

namespace cl {

const std::string DATE_TIME_FORMAT = "%Y-%m-%d %T";
const std::string DATE_FORMAT = "%Y-%m-%d";
const std::string TIME_FORMAT = "%H:%M";

typedef std::tuple<
    std::string,
    std::string,
    std::string,
    std::string,
    std::string> BrokenDownYearMonthDayHourMinute;

class TaskSchedule {
public:
    static expected<TaskSchedule, std::string> create(
        const std::string& start_str,
        const std::string& stop_str) CL_MUST_USE_RESULT;

    static expected<TaskSchedule, std::string> create(
        const std::string& task_date,
        const std::string& start_time_str,
        const std::string& stop_time_str) CL_MUST_USE_RESULT;

public:
    const std::string& start_str() const { return m_start_str; }

    const std::string& stop_str() const { return m_stop_str; }

    BrokenDownYearMonthDayHourMinute broken_down_start_time() const;

    BrokenDownYearMonthDayHourMinute broken_down_end_time() const;

    unsigned long duration_sec() const;

    void swap(TaskSchedule& other);

    bool operator==(const TaskSchedule& other) const;

    friend std::ostream& operator<<(std::ostream& os, const TaskSchedule& task);

private:
    TaskSchedule(const std::string& start_str, const std::string& stop_str)
    : m_start_str(start_str), m_stop_str(stop_str) {}

    std::string m_start_str;
    std::string m_stop_str;

};

class Task {
public:
    typedef int TaskId;

    static const std::string PROPERTY_ID;
    static const std::string PROPERTY_START;
    static const std::string PROPERTY_STOP;
    static const std::string PROPERTY_PROJECT;
    static const std::string PROPERTY_DESCRIPTION;
    static const std::string PROPERTY_TAGS;
    static const std::string PROPERTY_COMMENT;

    static const std::set<std::string> REQUIRED_PROPERTIES;

    Task(const TaskSchedule& schedule,
        const std::string& project,
        const std::string& description,
        const std::string& tags,
        const std::string& comment);

    Task(Task&&);

    Task(TaskId id, Task&&);

    TaskId id() const { return m_id; }

    const TaskSchedule& schedule() const { return m_schedule; }

    const std::string& start_str() const { return m_schedule.start_str(); }

    const std::string& stop_str() const { return m_schedule.stop_str(); }

    const std::string& project() const { return m_project; }

    const std::string& description() const { return m_description; }

    const std::set<std::string> tags() const { return m_tags; }
    void tags_from_string(const std::string& str);

    std::string joined_tags() const;

    const std::string& comment() const { return m_comment; }
    void comment(const std::string& str) { m_comment = str; }

    void swap(Task& other);

    bool operator==(const Task& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Task& task);

private:
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = delete;

    int m_id;
    TaskSchedule m_schedule;
    std::string m_project;
    std::string m_description;
    std::string m_comment;
    std::set<std::string> m_tags;
};

void swap(Task& t1, Task& t2);

}

