#include <filesystem>
#include <iostream>
#include <fstream>

#include <chrono>
#include <thread>

#include <captainlog/db_sqlite.hpp>
#include <captainlog/utils.hpp>

using namespace std::chrono;
namespace fs = std::filesystem;

using tl::expected;
using tl::make_unexpected;

namespace cl {

template<typename R>
static R row_from_statement(sqlite3_stmt* stmt);

template<>
Task row_from_statement(sqlite3_stmt* stmt)
{
    int i = 0;

    int id = sqlite3_column_int(stmt, i++);

    std::string task_start_str(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++)));
    std::string task_stop_str(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++)));
    std::string task_project(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++)));
    std::string task_description(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++)));
    std::string task_tags_str(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++)));
    std::string task_comment(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++)));

    return Task(
        id,
        {
            TaskSchedule::create(task_start_str, task_stop_str).value(),
            task_project,
            task_description,
            task_tags_str,
            task_comment
        });
}

template<>
json row_from_statement(sqlite3_stmt* stmt)
{
    int i = 0;

    json json_task;

    json_task[Task::PROPERTY_ID] = sqlite3_column_int(stmt, i++);
    json_task[Task::PROPERTY_START] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++));
    json_task[Task::PROPERTY_STOP] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++));
    json_task[Task::PROPERTY_PROJECT] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++));
    json_task[Task::PROPERTY_DESCRIPTION] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++));

    std::string task_tags_str(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++)));
    std::set<std::string> tags;
    utils::split(task_tags_str, ',', tags);
    json_task[Task::PROPERTY_TAGS] = std::move(tags);
    json_task[Task::PROPERTY_COMMENT] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i++));

    return json_task;
}

template<class R, typename... As>
struct ReusableStatementHandler
{
    using Tuple = typename QueryArgs<R, As...>::Tuple;

    ReusableStatementHandler(sqlite3_stmt* stmt, const QueryArgs<R, As...>& query_args)
        : m_stmt(stmt)
    {
        bind_args<0>(m_stmt, query_args.args());
    }

    ~ReusableStatementHandler()
    {
        if (m_stmt != nullptr) {
            sqlite3_clear_bindings(m_stmt);
            sqlite3_reset(m_stmt);
        }
    }

    ReusableStatementHandler(const ReusableStatementHandler&) = delete;
    ReusableStatementHandler& operator=(const ReusableStatementHandler&) = delete;

    bool exec() { return sqlite3_step(m_stmt) == SQLITE_DONE; }

    bool has_next() { return sqlite3_step(m_stmt) == SQLITE_ROW; }

    R from_row() { return row_from_statement<R>(m_stmt); }

    std::optional<R> maybe_from_row()
    {
        if (has_next()) {
            return from_row();
        } else {
            return std::nullopt;
        }
    }

    void visit_rows(std::function<bool(R&&)> visitor)
    {
        bool step = true;
        while (step && has_next()) {
            step = visitor(from_row());
        }
    }

    template <int N>
    constexpr static void bind_args(sqlite3_stmt* stmt, const Tuple& tuple)
    {
        if constexpr(N < std::tuple_size_v<Tuple>) {
            using Arg = std::tuple_element_t<N, Tuple>;
            const auto& value = std::get<N>(tuple);
            if constexpr(std::is_same<Arg, int>::value) {
                sqlite3_bind_int(stmt, N+1, value);
            } else if constexpr(std::is_same<Arg, std::string>::value) {
                sqlite3_bind_text(stmt, N+1, value.c_str(), -1, NULL);
            }
            bind_args<N+1>(stmt, tuple);
        }
    }

    sqlite3_stmt* m_stmt;
};

struct NoResult {};

template<class R, typename... As>
expected<void, std::string> DbSqlite::exec(QueryKey query_key, const QueryArgs<R, As...>& args)
{
    ReusableStatementHandler handler(m_statements[query_key], args);

    if (!handler.exec()) {
        return make_unexpected(
            std::string("Failed to execute the statement: ").append(sqlite3_errmsg(m_db)));
    }

    return expected<void, std::string>();
}

template<class R, typename... As>
expected<void, std::string> DbSqlite::do_visit(
    QueryKey query_key, 
    const QueryArgs<R, As...>& args, 
    std::function<bool(R&&)> visitor)
{
    ReusableStatementHandler handler(m_statements[query_key], args);

    handler.visit_rows(visitor);

    return expected<void, std::string>();
}

template<class R, typename... As>
std::optional<R> DbSqlite::maybe_find(QueryKey query_key, const QueryArgs<R, As...>& args)
{
    ReusableStatementHandler handler(m_statements[query_key], args);

    return handler.maybe_from_row();
}


enum DbSqlite::QueryKey : unsigned int
{
    INSERT_QUERY,
    UPDATE_QUERY,
    DELETE_FROM_ID_QUERY,
    SELECT_ALL_QUERY,
    FIND_FROM_ID_QUERY,
    FIND_LATEST_QUERY,
    FIND_LATEST_FOR_DAY_QUERY,
    FIND_FOR_DAY_QUERY,
    FIND_FROM_DESCRIPTION_QUERY,
    FIND_AT_QUERY
};

DbSqlite::DbSqlite(std::string&& path)
    : m_exists(false)
    , m_db_path(std::move(path))
    , m_db(nullptr)
{
    if (m_db_path.size() > 0) {
        std::cout << "Database path: " << m_db_path << std::endl;
        m_exists = fs::exists(fs::status(m_db_path));
    } else {
        std::cout << "In memory database" << std::endl;
        m_exists = false;
    }
}

DbSqlite::~DbSqlite()
{
    if (m_db == nullptr) {
        return;
    }

    for (const auto& pair : m_statements) {
        sqlite3_finalize(pair.second);
    }
    m_statements.clear();

    int close_res = sqlite3_close(m_db);
    if (close_res == SQLITE_OK) {
        std::cout << "Database closed" << std::endl;
    } else {
        std::cerr << "Database partially closed (" << close_res << ")" << std::endl;
    }
    m_db = nullptr;
}

expected<void, std::string> DbSqlite::open()
{
    if (m_db != nullptr) {
        return make_unexpected("Database already opened");
    }

    std::string uri;
    if (m_db_path.size() > 0) {
        uri = std::string("file:").append(m_db_path);
    } else {
        uri = ":memory:";
    }
    int res = sqlite3_open(uri.c_str(), &m_db);
    if (res != SQLITE_OK) {
        return make_unexpected(
            std::string("Cannot open database: ").append(sqlite3_errmsg(m_db)));
    }

    return expected<void, std::string>();
}

expected<void, std::string> DbSqlite::init_db()
{
    if (!m_exists) {
        if (auto res = exec_query(
                " CREATE TABLE tasks ( "
                    " task_id INTEGER PRIMARY KEY, "
                    " task_start TEXT NOT NULL CHECK (task_start <> ''), "
                    " task_stop TEXT NOT NULL CHECK (task_stop <> ''), "
                    " task_project TEXT NOT NULL CHECK (task_project <> ''), "
                    " task_description TEXT NOT NULL CHECK (task_description <> ''), "
                    " task_tags TEXT, "
                    " task_comment TEXT "
                " ) "); !res) {
            return make_unexpected(std::string("Failed to initialize the database: ").append(res.error()));
        }
        std::cout << "Created table tasks" << std::endl;
    }

    if (auto res = prepare_query(DbSqlite::INSERT_QUERY,
            " INSERT INTO tasks ( "
            "    task_start, task_stop, task_project, task_description, task_tags, task_comment) "
            " VALUES ( "
            "    ?, ?, ?, ?, ?, ?) "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_query(DbSqlite::UPDATE_QUERY,
            " UPDATE tasks SET "
            " task_start = ?, task_stop = ?, "
            " task_project = ?, task_description = ?, "
            " task_tags = ?, task_comment = ? "
            " WHERE task_id = ? "); !res) {
        return make_unexpected(res.error());
    }  

    if (auto res = prepare_query(DbSqlite::DELETE_FROM_ID_QUERY,
            " DELETE FROM tasks WHERE task_id = ? "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(DbSqlite::SELECT_ALL_QUERY, ""); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(DbSqlite::FIND_FROM_ID_QUERY,
            " WHERE task_id = ? "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(DbSqlite::FIND_LATEST_QUERY,
            " ORDER BY DATETIME(task_stop) DESC LIMIT ? "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(DbSqlite::FIND_LATEST_FOR_DAY_QUERY,
            " WHERE task_stop LIKE ? ORDER BY DATETIME(task_stop) DESC LIMIT 1 "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(DbSqlite::FIND_FOR_DAY_QUERY,
            " WHERE task_stop LIKE ? ORDER BY DATETIME(task_stop) ASC "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(DbSqlite::FIND_FROM_DESCRIPTION_QUERY,
            " WHERE task_description LIKE ? ORDER BY DATETIME(task_stop) DESC "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(DbSqlite::FIND_AT_QUERY,
            " WHERE DATETIME(?) BETWEEN DATETIME(task_start) AND DATETIME(task_stop) "); !res) {
        return make_unexpected(res.error());
    }

    return expected<void, std::string>();
}

expected<void, std::string> DbSqlite::delete_all()
{
    if (auto res = exec_query("DELETE FROM tasks"); !res) {
        return make_unexpected(std::string("Failed to delete tasks: ").append(res.error()));
    }
    return expected<void, std::string>();
}

expected<void, std::string> DbSqlite::delete_from_id(const Task::TaskId& id)
{
    return exec(DbSqlite::DELETE_FROM_ID_QUERY, QueryArgs<NoResult, Task::TaskId>(id));
}

expected<void, std::string> DbSqlite::insert(const Task& task)
{
    QueryArgs<NoResult, std::string, std::string, std::string, std::string, std::string, std::string> args(
        task.start_str(),
        task.stop_str(),
        task.project(),
        task.description(),
        task.joined_tags(),
        task.comment());

    return exec(DbSqlite::INSERT_QUERY, args);
}

expected<void, std::string> DbSqlite::insert(const json& json_task)
{
    auto task_res = task_from_json(json_task);
    if (!task_res) {
        return make_unexpected(task_res.error());
    }

    QueryArgs<NoResult, std::string, std::string, std::string, std::string, std::string, std::string> args(
        task_res.value().start_str(),
        task_res.value().stop_str(),
        task_res.value().project(),
        task_res.value().description(),
        task_res.value().joined_tags(),
        task_res.value().comment());

    return exec(DbSqlite::INSERT_QUERY, args);
}

expected<void, std::string> DbSqlite::update(const json& json_task)
{
    auto task_res = task_from_json(json_task);
    if (!task_res) {
        return make_unexpected(task_res.error());
    }

    QueryArgs<NoResult, std::string, std::string, std::string, std::string, std::string, std::string, int> args(
        task_res.value().start_str(),
        task_res.value().stop_str(),
        task_res.value().project(),
        task_res.value().description(),
        task_res.value().joined_tags(),
        task_res.value().comment(),
        task_res.value().id());

    return exec(DbSqlite::UPDATE_QUERY, args);
}

std::optional<Task> DbSqlite::find_from_id(QueryArgs<Task, Task::TaskId>&& arg)
{
    return maybe_find(DbSqlite::FIND_FROM_ID_QUERY, arg);
}

std::optional<json> DbSqlite::find_from_id(QueryArgs<json, Task::TaskId>&& arg)
{
    return maybe_find(DbSqlite::FIND_FROM_ID_QUERY, arg);
}

expected<void, std::string> DbSqlite::visit_all(std::function<bool(Task&&)> visitor)
{
    return do_visit(DbSqlite::SELECT_ALL_QUERY, QueryArgs<Task>(), visitor);
}

std::optional<Task> DbSqlite::find_latest()
{
    return maybe_find(DbSqlite::FIND_LATEST_QUERY, QueryArgs<Task, int>(1));
}

expected<void, std::string> DbSqlite::visit_n_latest(int count, std::function<bool(Task&&)> visitor)
{
    if (count <= 0) {
        return make_unexpected("Invalid count: " + count);
    }

    return do_visit(DbSqlite::FIND_LATEST_QUERY, QueryArgs<Task, int>(count), visitor);
}

std::optional<Task> DbSqlite::find_latest_for_day(const std::string& y_m_d_str)
{
    return maybe_find(DbSqlite::FIND_LATEST_FOR_DAY_QUERY, QueryArgs<Task, std::string>(y_m_d_str + "%"));
}

expected<void, std::string> DbSqlite::visit_for_day(
    const std::string& y_m_d_str,
    std::function<bool(Task&&)> visitor)
{
    return do_visit(DbSqlite::FIND_FOR_DAY_QUERY, QueryArgs<Task, std::string>(y_m_d_str + "%"), visitor);
}

expected<void, std::string> DbSqlite::visit_for_day(
    const std::string& y_m_d_str,
    std::function<bool(json&&)> visitor)
{
    return do_visit(DbSqlite::FIND_FOR_DAY_QUERY, QueryArgs<json, std::string>(y_m_d_str + "%"), visitor);
}

std::optional<Task> DbSqlite::find_at(const std::string& y_m_d_hh_mm_str)
{
    return maybe_find(DbSqlite::FIND_AT_QUERY, QueryArgs<Task, std::string>(y_m_d_hh_mm_str));
}

expected<void, std::string> DbSqlite::visit_from_description(
        const std::string& partial_descr,
        std::function<bool(Task&&)> visitor)
{
    return do_visit(DbSqlite::FIND_FROM_DESCRIPTION_QUERY, QueryArgs<Task, std::string>("%" + partial_descr + "%"), visitor);
}

expected<void, std::string> DbSqlite::exec_query(std::string&& query_str)
{
    char *err_msg = nullptr;
    int res = sqlite3_exec(m_db, query_str.c_str(), 0, 0, &err_msg);
    if (res != SQLITE_OK ) {
        auto e = make_unexpected(std::string(err_msg));
        sqlite3_free(err_msg);
        return e;
    }
    return expected<void, std::string>();
}

expected<sqlite3_stmt*, std::string> DbSqlite::prepare_query(QueryKey key, std::string&& query_str)
{
    if (m_statements.find(key) != m_statements.end()) {
        return make_unexpected("Query already prepared");
    }

    sqlite3_stmt* stmt;
    int res = sqlite3_prepare_v2(m_db, query_str.c_str(), -1, &stmt, nullptr);
    if (res != SQLITE_OK) {
        return make_unexpected(
            std::string("Failed to prepare statement: ").append(sqlite3_errmsg(m_db)));
    }
    m_statements[key] = stmt;
    return stmt;
}

expected<sqlite3_stmt*, std::string> DbSqlite::prepare_task_select_query(QueryKey key, std::string&& sub_query_str)
{
    return prepare_query(key,
        " SELECT task_id, task_start, task_stop, task_project, task_description, task_tags, task_comment FROM tasks "
        + sub_query_str);
}

} // end namespace
