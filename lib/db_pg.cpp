#include <captainlog/db_pg.hpp>
#include <captainlog/utils.hpp>

using tl::expected;
using tl::make_unexpected;

namespace cl {

template<int EXPECTED_RES = PGRES_TUPLES_OK>
struct PgRes {
    PgRes(PGresult* res):m_res(res) {}
    ~PgRes() {
        if (m_res != nullptr) {
            PQclear(m_res);
            m_res = nullptr;
        }
    }

    bool operator!() const {
        return PQresultStatus(m_res) != EXPECTED_RES;
    }

    std::string get_string(int row_number, int column_number) const {
        std::string value = PQgetvalue(m_res, row_number, column_number);
        return value;
    }

    int get_int(int row_number, int column_number) const {
        std::string value = PQgetvalue(m_res, row_number, column_number);
        int result = 0;
        std::istringstream(value) >> result;
        return result;
    }

    bool get_bool(int row_number, int column_number) const {
        auto value = get_string(row_number, column_number);
        return value[0] == 't';
    }

    int size() const {
        return PQntuples(m_res);
    }

    PGresult* get() const {
        return m_res;
    }

    PGresult* m_res;
};

Task row_from_res(const PgRes<PGRES_TUPLES_OK>& res, int row_index)
{
    int i = 0;

    int id = res.get_int(row_index, i++);

    std::string task_start_str{res.get_string(row_index, i++)};
    std::string task_stop_str{res.get_string(row_index, i++)};
    std::string task_project{res.get_string(row_index, i++)};
    std::string task_description{res.get_string(row_index, i++)};
    std::string task_tags_str{res.get_string(row_index, i++)};
    std::string task_comment{res.get_string(row_index, i++)};

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

DbPg::DbPg(std::string&& params)
    : m_params(std::move(params))
    , m_ready(false)
    , m_exists(false)
    , m_conn(nullptr)
{

}

DbPg::~DbPg()
{
    close();
}

void DbPg::close()
{
    if (m_conn != nullptr) {
        PQfinish(m_conn);
        m_conn = nullptr;
        m_ready = false;
    }
}

std::string DbPg::get_conn_error() const
{
    return std::string(PQerrorMessage(m_conn));
}

expected<void, std::string> DbPg::open()
{
    if (m_ready) {
        return make_unexpected("Database already opened");
    }

    m_ready = false;

    m_conn = PQconnectdb(m_params.c_str());

    if (PQstatus(m_conn) != CONNECTION_OK) {
        return make_unexpected(get_conn_error());
    }

    if (PgRes res = PQexec(m_conn,
            " SELECT pg_catalog.set_config('search_path', '', false) "); !res) {
        auto e = make_unexpected(get_conn_error());
        close();
        return e;
    }

    if (PgRes res = PQexec(m_conn,
            " SELECT EXISTS ( "
                " SELECT FROM pg_tables "
                " WHERE schemaname = 'public' AND tablename  = 'tasks' "
            " ) "); !!res) {
        m_exists = res.get_bool(0, 0);        
    } else {
        auto e = make_unexpected(get_conn_error());
        close();
        return e;
    }

    m_ready = true;

    return expected<void, std::string>();
}

expected<void, std::string> DbPg::init_db()
{
    if (!m_ready) {
        return make_unexpected("The database is not available");
    }

    if (!m_exists) {
        if (PgRes<PGRES_COMMAND_OK> res = PQexec(m_conn, 
                " CREATE TABLE public.tasks ( "
                    " task_id SERIAL PRIMARY KEY, "
                    " task_start TIMESTAMP WITHOUT TIME ZONE NOT NULL, "
                    " task_stop TIMESTAMP WITHOUT TIME ZONE NOT NULL, "
                    " task_project TEXT NOT NULL CHECK (task_project <> ''), "
                    " task_description TEXT NOT NULL CHECK (task_description <> ''), "
                    " task_tags TEXT, "
                    " task_comment TEXT "
                " ) "); !res) {
            return make_unexpected(std::string("Failed to initialize the database: ").append(get_conn_error()));
        }
    }

    return expected<void, std::string>();
}

expected<void, std::string> DbPg::clear_db()
{
    if (PgRes<PGRES_COMMAND_OK> res = PQexec(m_conn, " DROP TABLE IF EXISTS public.tasks "); !res) {
        return make_unexpected(std::string("Failed to clear the database: ").append(get_conn_error()));
    }

    return expected<void, std::string>();
}

expected<void, std::string> DbPg::insert(const Task& task)
{
    const char* query_params[6] = {0};

    query_params[0] = task.start_str().c_str();
    query_params[1] = task.stop_str().c_str();
    query_params[2] = task.project().c_str();
    query_params[3] = task.description().c_str();
    std::string joined_tags = task.joined_tags();
    query_params[4] = joined_tags.c_str();
    query_params[5] = task.comment().c_str();

    std::string query_str = 
        " INSERT INTO public.tasks ( "
            " task_start, task_stop, task_project, task_description, task_tags, task_comment "
        " ) "
        " VALUES ( "
            " $1, $2, $3, $4, $5, $6"
        " ) ";
    if (PgRes<PGRES_COMMAND_OK> res = PQexecParams(
            m_conn, query_str.c_str(), 6, NULL, query_params, NULL, NULL, 0); !res) {
        return make_unexpected(std::string("Failed to insert task: ").append(get_conn_error()));    
    }
    return expected<void, std::string>();
}

std::optional<Task> DbPg::find_latest()
{
    std::string query_str = get_task_select_statement() + " ORDER BY task_stop DESC LIMIT 1 ";
    if (PgRes res = PQexec(m_conn, query_str.c_str()); !!res) {
        if (res.size() == 1) {
            return row_from_res(res, 0);
        } else {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
}

std::string DbPg::get_task_select_statement() const
{
    std::string statement =
        " SELECT task_id, task_start, task_stop, task_project, task_description, task_tags, task_comment "
        " FROM public.tasks ";
    return statement;
}

}
