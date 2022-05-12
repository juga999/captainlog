#include <filesystem>
#include <iostream>
#include <fstream>

#include <chrono>
#include <thread>

#include <captainlog/db.hpp>

using namespace std::chrono;
namespace fs = std::filesystem;

using tl::expected;
using tl::make_unexpected;

namespace cl {

static const char CSV_SEPARATOR = '|';

template <void(*T)(sqlite3_stmt*)>
struct StatementHandler
{
    explicit StatementHandler(sqlite3_stmt* stmt):m_stmt(stmt) {}

    ~StatementHandler() { T(m_stmt); }

    sqlite3_stmt* stmt() const { return m_stmt; }

    sqlite3_stmt* m_stmt;
};

auto statement_reset_method = [](sqlite3_stmt* stmt) {
    if (stmt != nullptr) {
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
    }
};

typedef StatementHandler<statement_reset_method> StatementResetHandler;

enum Db::QueryKey : unsigned int
{
    INSERT_QUERY,
    DELETE_FROM_ID_QUERY,
    SELECT_ALL_QUERY,
    FIND_FROM_ID_QUERY,
    FIND_LATEST_QUERY,
    FIND_LATEST_FOR_DAY_QUERY,
    FIND_FROM_DESCRIPTION_QUERY,
    FIND_AT_QUERY
};

Db::Db(std::string&& path)
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

Db::~Db()
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

expected<void, std::string> Db::open()
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

expected<void, std::string> Db::init_db()
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

    if (auto res = prepare_query(Db::INSERT_QUERY,
            " INSERT INTO tasks ( "
            "    task_start, task_stop, task_project, task_description, task_tags, task_comment) "
            " VALUES ( "
            "    ?, ?, ?, ?, ?, ?) "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_query(Db::DELETE_FROM_ID_QUERY,
            " DELETE FROM tasks WHERE task_id = ? "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(Db::SELECT_ALL_QUERY, ""); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(Db::FIND_FROM_ID_QUERY,
            " WHERE task_id = ? "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(Db::FIND_LATEST_QUERY,
            " ORDER BY DATETIME(task_stop) DESC LIMIT ? "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(Db::FIND_LATEST_FOR_DAY_QUERY,
            " WHERE task_stop LIKE ? ORDER BY DATETIME(task_stop) DESC LIMIT 1 "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(Db::FIND_FROM_DESCRIPTION_QUERY,
            " WHERE task_description LIKE ? ORDER BY DATETIME(task_stop) DESC "); !res) {
        return make_unexpected(res.error());
    }

    if (auto res = prepare_task_select_query(Db::FIND_AT_QUERY,
            " WHERE DATETIME(?) BETWEEN DATETIME(task_start) AND DATETIME(task_stop) "); !res) {
        return make_unexpected(res.error());
    }

    return expected<void, std::string>();
}

expected<void, std::string> Db::delete_all()
{
    if (auto res = exec_query("DELETE FROM tasks"); !res) {
        return make_unexpected(std::string("Failed to delete tasks: ").append(res.error()));
    }
    return expected<void, std::string>();
}

expected<void, std::string> Db::delete_from_id(int task_id)
{
    StatementResetHandler handler(m_statements[Db::DELETE_FROM_ID_QUERY]);

    sqlite3_bind_int(handler.stmt(), 1, task_id);
    int res = sqlite3_step(handler.stmt());
    if (res != SQLITE_DONE) {
        return make_unexpected(
            std::string("Failed to delete task: ").append(sqlite3_errmsg(m_db)));
    }

    return expected<void, std::string>();
}

expected<void, std::string> Db::insert(const Task& task)
{
    StatementResetHandler handler(m_statements[Db::INSERT_QUERY]);

    sqlite3_bind_text(handler.stmt(), 1, task.start_str().c_str(), -1, NULL);
    sqlite3_bind_text(handler.stmt(), 2, task.stop_str().c_str(), -1, NULL);
    sqlite3_bind_text(handler.stmt(), 3, task.project().c_str(), -1, NULL);
    sqlite3_bind_text(handler.stmt(), 4, task.description().c_str(), -1, NULL);

    std::string joined_tags = task.joined_tags();
    sqlite3_bind_text(handler.stmt(), 5, joined_tags.c_str(), -1, NULL);

    sqlite3_bind_text(handler.stmt(), 6, task.comment().c_str(), -1, NULL);

    int res = sqlite3_step(handler.stmt());
    if (res != SQLITE_DONE) {
        return make_unexpected(
            std::string("Failed to insert task: ").append(sqlite3_errmsg(m_db)));
    }

    return expected<void, std::string>();
}

Task Db::from_row(sqlite3_stmt* stmt)
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

std::optional<Task> Db::find_from_id(int id)
{
    StatementResetHandler handler(m_statements[Db::FIND_FROM_ID_QUERY]);

    sqlite3_bind_int(handler.stmt(), 1, id);
    if (sqlite3_step(handler.stmt()) == SQLITE_ROW) {
        return from_row(handler.stmt());
    } else {
        return std::nullopt;
    }
}

expected<void, std::string> Db::visit_all(std::function<bool(Task&&)> visitor)
{
    StatementResetHandler handler(m_statements[Db::SELECT_ALL_QUERY]);

    bool step = true;
    int step_res = sqlite3_step(handler.stmt());
    while (step && step_res == SQLITE_ROW) {
        step = visitor(from_row(handler.stmt()));
        step_res = sqlite3_step(handler.stmt());
    }

    return expected<void, std::string>();
}

std::optional<Task> Db::find_latest()
{
    StatementResetHandler handler(m_statements[Db::FIND_LATEST_QUERY]);

    sqlite3_bind_int(handler.stmt(), 1, 1);
    if (sqlite3_step(handler.stmt()) == SQLITE_ROW) {
        return from_row(handler.stmt());
    } else {
        return std::nullopt;
    }
}

expected<void, std::string> Db::visit_n_latest(int count, std::function<bool(Task&&)> visitor)
{
    if (count <= 0) {
        return make_unexpected("Invalid count: " + count);
    }

    StatementResetHandler handler(m_statements[Db::FIND_LATEST_QUERY]);

    sqlite3_bind_int(handler.stmt(), 1, count);

    bool step = true;
    int step_res = sqlite3_step(handler.stmt());
    while (step && step_res == SQLITE_ROW) {
        step = visitor(from_row(handler.stmt()));
        step_res = sqlite3_step(handler.stmt());
    }

    return expected<void, std::string>();
}

std::optional<Task> Db::find_latest_for_day(const std::string& y_m_d_str)
{
    StatementResetHandler handler(m_statements[Db::FIND_LATEST_FOR_DAY_QUERY]);

    std::string query_arg = y_m_d_str + "%";
    sqlite3_bind_text(handler.stmt(), 1, query_arg.c_str(), -1, NULL);
    if (sqlite3_step(handler.stmt()) == SQLITE_ROW) {
        return from_row(handler.stmt());
    } else {
        return std::nullopt;
    }
}

std::optional<Task> Db::find_at(const std::string& y_m_d_H_M_str)
{
    StatementResetHandler handler(m_statements[Db::FIND_AT_QUERY]);

    sqlite3_bind_text(handler.stmt(), 1, y_m_d_H_M_str.c_str(), -1, NULL);
    if (sqlite3_step(handler.stmt()) == SQLITE_ROW) {
        return from_row(handler.stmt());
    } else {
        return std::nullopt;
    }
}

expected<void, std::string> Db::visit_from_description(
        const std::string& partial_descr,
        std::function<bool(Task&&)> visitor)
{
    StatementResetHandler handler(m_statements[Db::FIND_FROM_DESCRIPTION_QUERY]);

    std::string query_arg = "%" + partial_descr + "%";
    sqlite3_bind_text(handler.stmt(), 1, query_arg.c_str(), -1, NULL);

    bool step = true;
    int step_res = sqlite3_step(handler.stmt());
    while (step && step_res == SQLITE_ROW) {
        step = visitor(from_row(handler.stmt()));
        step_res = sqlite3_step(handler.stmt());
    }

    return expected<void, std::string>();
}

static expected<void, std::string> get_csv_header_column(std::stringstream& ss, const std::string& expected_name)
{
    std::string colname;
    if (std::getline(ss, colname, CSV_SEPARATOR)) {
        if (colname != expected_name) {
            return make_unexpected("Expected column " + expected_name + " but found column " + colname);
        }
    } else {
        return make_unexpected("Unexpected end of line");
    }
    return expected<void, std::string>();
}

expected<unsigned int, std::string> Db::import_legacy_csv(std::istream& is)
{
    std::string line;
    std::getline(is, line);
    std::stringstream header_ss(line);

    auto header_check_res = get_csv_header_column(header_ss, "task_date")
        .and_then([&]() { return get_csv_header_column(header_ss, "task_start"); })
        .and_then([&]() { return get_csv_header_column(header_ss, "task_stop"); })
        .and_then([&]() { return get_csv_header_column(header_ss, "task_description"); })
        .and_then([&]() { return get_csv_header_column(header_ss, "task_project"); })
        .and_then([&]() { return get_csv_header_column(header_ss, "task_tags"); })
        .and_then([&]() { return get_csv_header_column(header_ss, "task_comment"); });
    if (!header_check_res) {
        return make_unexpected("Import failed: " + header_check_res.error());
    }

    unsigned int count = 0;
    while(std::getline(is, line)) {
        if (line.size() == 0) {
            continue;
        }

        std::stringstream ss(line);

        std::string task_date;
        std::string task_start_time;
        std::string task_stop_time;
        std::string task_description;
        std::string task_project;
        std::string task_tags;
        std::string task_comment;
        std::getline(ss, task_date, CSV_SEPARATOR);
        std::getline(ss, task_start_time, CSV_SEPARATOR);
        std::getline(ss, task_stop_time, CSV_SEPARATOR);
        std::getline(ss, task_description, CSV_SEPARATOR);
        std::getline(ss, task_project, CSV_SEPARATOR);
        std::getline(ss, task_tags, CSV_SEPARATOR);
        std::getline(ss, task_comment, CSV_SEPARATOR);

        std::string task_start_str = task_date + " " + task_start_time + ":00";
        std::string task_stop_str = task_date + " " + task_stop_time + ":00";

        auto res = TaskSchedule::create(task_start_str, task_stop_str)
            .and_then([&](auto schedule) {
                return insert(Task(
                    schedule,
                    task_project,
                    task_description,
                    task_tags,
                    task_comment));
            });

        if (res) {
            ++count;
        } else {
            return make_unexpected("Error for line: " + line + "\n" + "\t" + res.error());
        }
    }

    return count;
}

expected<unsigned int, std::string> Db::import_legacy_csv(const std::string& filename)
{
    std::ifstream csv_file(filename);
    if(!csv_file.is_open()) {
        return make_unexpected("Failed to open the file " + filename);
    }

    return import_legacy_csv(csv_file);
}

static void write_csv_task(std::ofstream& csv_file, const Task& task) {
    std::string start_date;
    std::string start_time;
    std::string end_time;
    {
        auto [year, month, day, hour, minute] = task.schedule().broken_down_start_time();
        start_date.append(year).append("-").append(month).append("-").append(day);
        start_time.append(hour).append(":").append(minute);
    }
    {
        auto [year, month, day, hour, minute] = task.schedule().broken_down_end_time();
        end_time.append(hour).append(":").append(minute);
    }

    csv_file
        << start_date << CSV_SEPARATOR
        << start_time << CSV_SEPARATOR
        << end_time << CSV_SEPARATOR
        << task.description() << CSV_SEPARATOR
        << task.project() << CSV_SEPARATOR
        << task.joined_tags() << CSV_SEPARATOR
        << task.comment()
        << "\n";
}

expected<unsigned int, std::string> Db::export_legacy_csv(const std::string& filename)
{
    std::ofstream csv_file;
    csv_file.open(filename, std::ios::out | std::ios::trunc);
    if (!csv_file.is_open()) {
        return make_unexpected("Failed to open the file " + filename);
    }

    csv_file 
        << "task_date" << CSV_SEPARATOR
        << "task_start" << CSV_SEPARATOR
        << "task_stop" << CSV_SEPARATOR
        << "task_description" << CSV_SEPARATOR
        << "task_project" << CSV_SEPARATOR
        << "task_tags" << CSV_SEPARATOR
        << "task_comment"
        << "\n";

    int count = 0;
    auto visit_res = visit_all([&](const Task& task) {
        write_csv_task(csv_file, task);
        ++count;
        return true;
    });

    csv_file.flush();

    csv_file.close();

    if (!visit_res) {
        return make_unexpected("Export failed: " + visit_res.error());
    }

    return count;
}

expected<void, std::string> Db::exec_query(std::string&& query_str)
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

expected<sqlite3_stmt*, std::string> Db::prepare_query(QueryKey key, std::string&& query_str)
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

expected<sqlite3_stmt*, std::string> Db::prepare_task_select_query(QueryKey key, std::string&& sub_query_str)
{
    return prepare_query(key,
        " SELECT task_id, task_start, task_stop, task_project, task_description, task_tags, task_comment FROM tasks "
        + sub_query_str);
}

} // end namespace
