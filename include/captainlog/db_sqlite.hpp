#pragma once

#include <sqlite3.h>

#include <captainlog/db.hpp>

namespace cl {

class DbSqlite : public Db {
public:
    DbSqlite(std::string&& path);

    ~DbSqlite() override;

    expected<void, std::string> open() override;

    expected<void, std::string> init_db() override;

    expected<void, std::string> insert(const Task& task) override;

    expected<void, std::string> insert(const json& json_task) override;

    expected<void, std::string> update(const json& json_task) override;

    expected<void, std::string> delete_from_id(const Task::TaskId&) override;

    expected<void, std::string> delete_all() override;

    expected<void, std::string> visit_all(std::function<bool(Task&&)> visitor) override;

    std::optional<Task> find_from_id(QueryArgs<Task, Task::TaskId>&&) override;

    std::optional<json> find_from_id(QueryArgs<json, Task::TaskId>&&) override;

    std::optional<Task> find_latest() override;

    expected<void, std::string> visit_n_latest(int count, std::function<bool(Task&&)> visitor) override;

    std::optional<Task> find_latest_for_day(const std::string& y_m_d_str) override;

    expected<void, std::string> visit_for_day(
        const std::string& y_m_d_str,
        std::function<bool(Task&&)> visitor) override;

    expected<void, std::string> visit_for_day(
        const std::string& y_m_d_str,
        std::function<bool(json&&)> visitor) override;

    std::optional<Task> find_at(const std::string& y_m_d_hh_mm_str) override;

    expected<void, std::string> visit_from_description(
        const std::string& partial_descr,
        std::function<bool(Task&&)> visitor) override;

private:
    enum QueryKey : unsigned int;

private:
    template<class R, typename... As>
    expected<void, std::string> exec(QueryKey, const QueryArgs<R, As...>&);

    template<class R, typename... As>
    std::optional<R> maybe_find(QueryKey, const QueryArgs<R, As...>&);

    template<class R, typename... As>
    expected<void, std::string> do_visit(QueryKey, const QueryArgs<R, As...>&, std::function<bool(R&&)>);

    expected<void, std::string> exec_query(std::string&& query_str) CL_MUST_USE_RESULT;
    expected<sqlite3_stmt*, std::string> prepare_query(QueryKey key, std::string&& query_str) CL_MUST_USE_RESULT;
    expected<sqlite3_stmt*, std::string> prepare_task_select_query(QueryKey key, std::string&& sub_query_str) CL_MUST_USE_RESULT;

    bool m_exists;
    std::string m_db_path;
    sqlite3* m_db;
    std::map<QueryKey, sqlite3_stmt*> m_statements;
};

}
