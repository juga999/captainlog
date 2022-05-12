#pragma once

#include <istream>
#include <map>

#include <sqlite3.h>

#include <captainlog/expected.hpp>
#include <captainlog/common.hpp>
#include <captainlog/task.hpp>

using tl::expected;

namespace cl {

class Db {
public:
    Db(std::string&& path);

    ~Db();

    expected<void, std::string> open() CL_MUST_USE_RESULT;

    expected<void, std::string> init_db() CL_MUST_USE_RESULT;

    expected<void, std::string> insert(const Task& task) CL_MUST_USE_RESULT;

    expected<void, std::string> delete_from_id(int) CL_MUST_USE_RESULT;

    expected<void, std::string> delete_all() CL_MUST_USE_RESULT;

    expected<void, std::string> visit_all(std::function<bool(Task&&)> visitor) CL_MUST_USE_RESULT;

    std::optional<Task> find_from_id(int) CL_MUST_USE_RESULT;

    std::optional<Task> find_latest() CL_MUST_USE_RESULT;

    expected<void, std::string> visit_n_latest(int count, std::function<bool(Task&&)> visitor) CL_MUST_USE_RESULT;

    std::optional<Task> find_latest_for_day(const std::string&) CL_MUST_USE_RESULT;

    std::optional<Task> find_at(const std::string&) CL_MUST_USE_RESULT;

    expected<void, std::string> visit_from_description(
        const std::string& partial_descr,
        std::function<bool(Task&&)> visitor) CL_MUST_USE_RESULT;

    expected<unsigned int, std::string> import_legacy_csv(std::istream& is) CL_MUST_USE_RESULT;

    expected<unsigned int, std::string> import_legacy_csv(const std::string& filename) CL_MUST_USE_RESULT;

    expected<unsigned int, std::string> export_legacy_csv(const std::string& filename) CL_MUST_USE_RESULT;

private:
    enum QueryKey : unsigned int;

private:
    Db(const Db&) = delete;
    Db(Db&&) = delete;
    Db& operator=(const Db&) = delete;
    Db& operator=(Db&&) = delete;

    expected<void, std::string> exec_query(std::string&& query_str) CL_MUST_USE_RESULT;
    expected<sqlite3_stmt*, std::string> prepare_query(QueryKey key, std::string&& query_str) CL_MUST_USE_RESULT;
    expected<sqlite3_stmt*, std::string> prepare_task_select_query(QueryKey key, std::string&& sub_query_str) CL_MUST_USE_RESULT;

    Task from_row(sqlite3_stmt* stmt) CL_MUST_USE_RESULT;

    bool m_exists;
    std::string m_db_path;
    sqlite3* m_db;
    std::map<QueryKey, sqlite3_stmt*> m_statements;
};

}

