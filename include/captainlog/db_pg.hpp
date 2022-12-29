#pragma once

#include <libpq-fe.h>

#include <captainlog/db.hpp>

namespace cl {

class DbPg {
public:
    DbPg(std::string&& params);

    ~DbPg();

    expected<void, std::string> open();

    expected<void, std::string> init_db();

    expected<void, std::string> clear_db();

    expected<void, std::string> insert(const Task& task);

    std::optional<Task> find_latest();

private:
    void close();

    std::string get_conn_error() const;

    std::string get_task_select_statement() const;

private:
    std::string m_params;
    bool m_ready;
    bool m_exists;
    PGconn* m_conn;
};

}
