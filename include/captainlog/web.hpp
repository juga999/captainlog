#pragma once

#include <event2/event.h>
#include <event2/http.h>

#include <nlohmann/json.hpp>

#include <captainlog/expected.hpp>
#include <captainlog/common.hpp>
#include <captainlog/db.hpp>

using tl::expected;
using json = nlohmann::json;

namespace cl {

class WebServer
{
public:
    WebServer(cl::Db&);
    ~WebServer();

    expected<void, std::string> init_server() CL_MUST_USE_RESULT;

    void start();

    bool handle_get_info_request(struct evhttp_request* req);

    bool handle_get_task_request(struct evhttp_request* req, int id);

private:
    void send_ok_json_response(struct evhttp_request* req, const json& json_response);

private:
    cl::Db& m_db;
    event_config* m_cfg;
    event_base* m_base;
    evhttp* m_http;
    event* m_term_evt;
    evhttp_bound_socket* handle;

    std::string m_response_data;
};

}
