#include <csignal>
#include <iostream>
#include <sstream>
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

static void do_term(int sig, short events, void *arg);

const std::regex API_ENDPOINT_TASK_FROM_ID(
    "/api/task/id/(\\d+)",
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

bool WebServer::handle_get_info_request(struct evhttp_request* req)
{
    json json_response;
    json_response["name"] = APP_NAME;
    json_response["version"] = APP_VERSION;
    json_response["build_type"] = APP_BUILD_TYPE;
    json_response["git_hash"] = APP_GIT_HASH;

    send_ok_json_response(req, json_response);

    return true;
}

bool WebServer::handle_get_task_request(struct evhttp_request* req, int id)
{
    auto maybe_task = m_db.find_from_id<json>(id);
    if (!maybe_task) {
        return false;
    }

    send_ok_json_response(req, maybe_task.value());

    return true;
}

void WebServer::send_ok_json_response(struct evhttp_request* req, const json& json_response)
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
}

static void handle_generic_request(struct evhttp_request* req, void *arg)
{
    WebServer* self = static_cast<WebServer*>(arg);

    auto uri = std::string(evhttp_request_get_uri(req));
    auto command = evhttp_request_get_command(req);

    bool processed = false;
    std::smatch m;

    if (command == EVHTTP_REQ_GET) {
        if (uri == "/api/info") {
            processed = self->handle_get_info_request(req);
        } else if (std::regex_match(uri, m, API_ENDPOINT_TASK_FROM_ID)) {
            auto id = std::strtol(m[1].str().c_str(), nullptr, 10);
            processed = self->handle_get_task_request(req, id);
        }
    }

    if (!processed) {
        evhttp_send_error(req, HTTP_NOTFOUND , 0);
    }
}

static void set_json_content_type(struct evhttp_request* req)
{
    evhttp_add_header(evhttp_request_get_output_headers(req),
		"Content-Type", "application/json; charset=utf-8");
}

static void do_term(int sig, short events, void *arg)
{
	event_base* base = static_cast<event_base*>(arg);
	event_base_loopbreak(base);
    std::cerr << "Got " << sig << ", terminating" << std::endl;
}

}
