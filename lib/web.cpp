#include <csignal>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <memory>
#include <regex>

#include <event2/buffer.h>

#include "app/app_config.h"

#include <captainlog/web.hpp>

using tl::make_unexpected;

namespace cl {

using EventBufferPtr = std::unique_ptr<evbuffer, decltype(&evbuffer_free)>;

static void handle_generic_request(struct evhttp_request* req, void *arg);
static void send_ok_json_response(struct evhttp_request* req, const json& json_response);
static void set_json_content_type(struct evhttp_request* req);
static json get_json_body(struct evhttp_request* req);

static void do_term(int sig, short events, void *arg);

static const json JSON_SUCCESS_RESPONSE{{"success", "true"}};

const std::regex API_ENDPOINT_TASK_FROM_ID(
    "/api/task/id/(\\d+)",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

const std::regex API_ENDPOINT_TASKS_FOR_DAY(
    "/api/tasks/(\\d{4})/(\\d{2})/(\\d{2})/",
    std::regex_constants::ECMAScript | std::regex_constants::icase);


WebServer::WebServer(cl::Db& db)
: m_db(db)
, m_cfg(nullptr)
, m_base(nullptr)
, m_http(nullptr)
{
    event_enable_debug_logging(EVENT_DBG_ALL);
}

WebServer::~WebServer()
{
    if (m_term_evt) {
        event_free(m_term_evt);
        m_term_evt = nullptr;
    }
    if (m_http) {
        evhttp_free(m_http);
        m_http = nullptr;
    }
    if (m_base) {
        event_base_free(m_base);
        m_base = nullptr;
    }
    if (m_cfg) {
        event_config_free(m_cfg);
        m_cfg = nullptr;
    }
}

expected<void, std::string> WebServer::init_server()
{
    m_cfg = event_config_new();
    m_base = event_base_new_with_config(m_cfg);
    if (!m_base) {
        return make_unexpected("Failed to create an event_base");
	}

    m_http = evhttp_new(m_base);
    if (!m_http) {
        return make_unexpected("Failed to create evhttp");
    }

    evhttp_set_gencb(m_http, handle_generic_request, this);

    std::string addr{"0.0.0.0"};
    int port = 9090;
    auto handle = evhttp_bind_socket_with_handle(m_http, addr.c_str(), port);
    if (!handle) {
        std::ostringstream oss;
        oss << "Failed to bind to " << addr << ":" << port;
        return make_unexpected(oss.str());
    }

    m_term_evt = evsignal_new(m_base, SIGINT, do_term, m_base);
    if (!m_term_evt) {
        return make_unexpected("Failed to create SIGINT handler");
    }
    if (event_add(m_term_evt, NULL)) {
        return make_unexpected("Failed to create SIGINT handler");
    }

    return expected<void, std::string>();
}

void WebServer::start()
{
    event_base_dispatch(m_base);
}

expected<bool, std::string> WebServer::handle_get_info_request(struct evhttp_request* req)
{
    json json_response;
    json_response["name"] = APP_NAME;
    json_response["version"] = APP_VERSION;
    json_response["build_type"] = APP_BUILD_TYPE;
    json_response["git_hash"] = APP_GIT_HASH;

    return send_ok_json_response(req, json_response);
}

expected<bool, std::string> WebServer::handle_get_task_request(struct evhttp_request* req, int id)
{
    auto maybe_task = m_db.find_from_id<json>(id);
    if (!maybe_task) {
        return false;
    }

    return send_ok_json_response(req, maybe_task.value());
}

expected<bool, std::string> WebServer::handle_delete_task_request(struct evhttp_request* req, int id)
{
    return m_db.delete_from_id(id).map([&]() {
        return send_ok_json_response(req, JSON_SUCCESS_RESPONSE);
    })
    .map_error([&](auto err) {
        return err;
    });
}

expected<bool, std::string> WebServer::handle_get_tasks_for_day_request(struct evhttp_request* req, std::string&& y_m_d_str)
{
    json json_response = json::array();

    return m_db.visit_for_day<json>(y_m_d_str, [&](auto json_task) {
        json_response.push_back(json_task);
        return true;
    })
    .map([&]() {
        return send_ok_json_response(req, json_response);
    })
    .map_error([&](auto err) {
        return err;
    });
}

expected<bool, std::string> WebServer::handle_create_update_task_request(struct evhttp_request* req)
{
    json json_task = get_json_body(req);

    expected<void, std::string> db_res;
    if (json_task.contains("id")) {
        db_res = m_db.update(json_task);
    } else {
        db_res = m_db.insert(json_task);
    }

    return db_res.map([&]() {
        return send_ok_json_response(req, JSON_SUCCESS_RESPONSE);
    })
    .map_error([&](auto err) {
        return err;
    });
}

bool WebServer::send_ok_json_response(struct evhttp_request* req, const json& json_response)
{
    m_response_data = json_response.dump();

    EventBufferPtr evb(evbuffer_new(), &evbuffer_free);

    //evbuffer_add(evb.get(), data.c_str(), data.size());
    evbuffer_add_reference(evb.get(), m_response_data.c_str(), m_response_data.size(), [](const void*, size_t, void* ptr) {
        WebServer* self = static_cast<WebServer*>(ptr);
        self->m_response_data.clear();
    }, this);

    set_json_content_type(req);

    evhttp_send_reply(req, HTTP_OK, "OK", evb.get());

    return true;
}

bool WebServer::send_internal_error_json_response(struct evhttp_request* req, const std::string& error_msg)
{
    json json_response;
    json_response["error"] = error_msg;

    m_response_data = json_response.dump();

    EventBufferPtr evb(evbuffer_new(), &evbuffer_free);

    //evbuffer_add(evb.get(), data.c_str(), data.size());
    evbuffer_add_reference(evb.get(), m_response_data.c_str(), m_response_data.size(), [](const void*, size_t, void* ptr) {
        WebServer* self = static_cast<WebServer*>(ptr);
        self->m_response_data.clear();
    }, this);

    set_json_content_type(req);

    evhttp_send_reply(req, HTTP_INTERNAL , "Internal error", evb.get());

    return true;
}

static void handle_generic_request(struct evhttp_request* req, void *arg)
{
    WebServer* self = static_cast<WebServer*>(arg);

    auto uri = std::string(evhttp_request_get_uri(req));
    auto command = evhttp_request_get_command(req);

    auto response_result = expected<bool, std::string>(false);
    std::smatch m;

    if (command == EVHTTP_REQ_GET) {
        if (uri == "/api/info") {
            response_result = self->handle_get_info_request(req);
        } else if (std::regex_match(uri, m, API_ENDPOINT_TASK_FROM_ID)) {
            auto id = std::strtol(m[1].str().c_str(), nullptr, 10);
            response_result = self->handle_get_task_request(req, id);
        } else if (std::regex_match(uri, m, API_ENDPOINT_TASKS_FOR_DAY)) {
            std::string param =  m[1].str() + "-" + m[2].str() + "-" + m[3].str();
            response_result = self->handle_get_tasks_for_day_request(req, std::move(param));
        }
    } else if (command == EVHTTP_REQ_DELETE) {
        if (std::regex_match(uri, m, API_ENDPOINT_TASK_FROM_ID)) {
            auto id = std::strtol(m[1].str().c_str(), nullptr, 10);
            response_result = self->handle_delete_task_request(req, id);
        }
    } else if (command == EVHTTP_REQ_POST) {
        if (uri == "/api/task") {
            response_result = self->handle_create_update_task_request(req);
        }
    }

    if (!response_result) {
        self->send_internal_error_json_response(req, response_result.error());
    } else if (!response_result.value()) {
        evhttp_send_error(req, HTTP_NOTFOUND , 0);
    }
}

class RequestBodyStreamBuf : public std::streambuf {
public:
    RequestBodyStreamBuf(struct evhttp_request* req)
    : m_evb(nullptr) {
        m_evb = evhttp_request_get_input_buffer(req);
    }

    int underflow() {
        if (m_evb == nullptr) {
            return std::char_traits<char>::eof();
        }

        if (gptr() == egptr()) {
            auto c = evbuffer_remove(m_evb, &m_buffer, sizeof(m_buffer));
            if (c == -1) {
                return std::char_traits<char>::eof();
            }
            m_read += c;
            setg(&m_buffer[0], &m_buffer[0], &m_buffer[0] + c);
        }
        return gptr() == egptr()
            ? std::char_traits<char>::eof()
            : std::char_traits<char>::to_int_type(*gptr());
    }

private:
    char m_buffer[1024] = {0};
    struct evbuffer* m_evb;
    unsigned int m_read;
};

static json get_json_body(struct evhttp_request* req)
{
    RequestBodyStreamBuf sbuf(req);
    std::istream in(&sbuf);
    json body;
    in >> body;
    return std::move(body);
}

static void set_json_content_type(struct evhttp_request* req)
{
    evhttp_add_header(evhttp_request_get_output_headers(req),
		"Content-Type", "application/json; charset=utf-8");
    evhttp_add_header(evhttp_request_get_output_headers(req),
		"Cache-Control", "no-store");
}

static void do_term(int sig, short events, void *arg)
{
	event_base* base = static_cast<event_base*>(arg);
	event_base_loopbreak(base);
    std::cerr << "Got " << sig << ", terminating" << std::endl;
}

}
