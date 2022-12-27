#pragma once

#include <event2/event.h>
#include <event2/http.h>

#include <nlohmann/json.hpp>

#include <captainlog/expected.hpp>
#include <captainlog/common.hpp>

using tl::expected;
using json = nlohmann::json;

namespace cl {

class Db;

class WebServer
{
public:
    WebServer(const json& config_json, cl::Db&);
    ~WebServer();

    expected<void, std::string> init_server() CL_MUST_USE_RESULT;

    void start();

    expected<bool, std::string> handle_get_info_request(struct evhttp_request* req);

    expected<bool, std::string> handle_get_task_request(struct evhttp_request* req, int id);

    expected<bool, std::string> handle_delete_task_request(struct evhttp_request* req, int id);

    expected<bool, std::string> handle_get_tasks_for_day_request(struct evhttp_request* req, std::string&& y_m_d_str);

    expected<bool, std::string> handle_create_update_task_request(struct evhttp_request* req);

    bool send_internal_error_json_response(struct evhttp_request* req, const std::string& error_msg);

    void on_request_completion();

private:
    bool send_ok_json_response(struct evhttp_request* req, const json& json_response);

private:
    json m_config_json;
    cl::Db& m_db;
    event_config* m_cfg;
    event_base* m_base;
    evhttp* m_http;
    event* m_term_evt;
};

}
