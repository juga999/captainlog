#pragma once

#include <istream>
#include <map>
#include <tuple>
#include <type_traits>

#include <nlohmann/json.hpp>

#include <captainlog/expected.hpp>
#include <captainlog/common.hpp>
#include <captainlog/task.hpp>

using tl::expected;
using json = nlohmann::json;

namespace cl {

template<class R, typename... As>
class QueryArgs {
public:
    typedef std::tuple<As...> Tuple;

    QueryArgs(As... args)
        : m_args(std::tuple<As...>(args...)) {}

    const Tuple& args() const { return m_args; }

private:
    const Tuple m_args;

private:
    QueryArgs(const QueryArgs&) = delete;
    QueryArgs& operator=(const QueryArgs&) = delete;
};

class Db {
public:
    static expected<Task, std::string> task_from_json(const json& json_task) CL_MUST_USE_RESULT;

public:
    virtual ~Db() {}

    virtual expected<void, std::string> open() CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> init_db() CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> insert(const Task& task) CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> insert(const json& json_task) CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> update(const json& json_task) CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> delete_from_id(const Task::TaskId&) CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> delete_all() CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> visit_all(std::function<bool(Task&&)> visitor) CL_MUST_USE_RESULT = 0;

    virtual std::optional<Task> find_from_id(QueryArgs<Task, Task::TaskId>&&) CL_MUST_USE_RESULT = 0;

    virtual std::optional<json> find_from_id(QueryArgs<json, Task::TaskId>&&) CL_MUST_USE_RESULT = 0;

    virtual std::optional<Task> find_latest() CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> visit_n_latest(int count, std::function<bool(Task&&)> visitor) CL_MUST_USE_RESULT = 0;

    virtual std::optional<Task> find_latest_for_day(const std::string& y_m_d_str) CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> visit_for_day(
        const std::string& y_m_d_str,
        std::function<bool(Task&&)> visitor) CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> visit_for_day(
        const std::string& y_m_d_str,
        std::function<bool(json&&)> visitor) CL_MUST_USE_RESULT = 0;

    virtual std::optional<Task> find_at(const std::string& y_m_d_hh_mm_str) CL_MUST_USE_RESULT = 0;

    virtual expected<void, std::string> visit_from_description(
        const std::string& partial_descr,
        std::function<bool(Task&&)> visitor) CL_MUST_USE_RESULT = 0;

protected:
    Db() {}

private:
    Db(const Db&) = delete;
    Db(Db&&) = delete;
    Db& operator=(const Db&) = delete;
    Db& operator=(Db&&) = delete;
};

}

