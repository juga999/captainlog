#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <csignal>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <memory>
#include <regex>
#include <chrono>

#include <event2/buffer.h>

#include "app/app_config.h"

#include <captainlog/web.hpp>

using tl::make_unexpected;

namespace cl {

using EventBufferPtr = std::unique_ptr<evbuffer, decltype(&evbuffer_free)>;

class JsonResponseBodyStreamBuf : public std::streambuf
{
public:
    JsonResponseBodyStreamBuf() {
        m_evb = evbuffer_new();
        setp(m_buffer, m_buffer + sizeof(m_buffer) - 1);
    }

    virtual ~JsonResponseBodyStreamBuf() {
        evbuffer_free(m_evb);
    }

    struct evbuffer* get() {
        return m_evb;
    }

    int overflow(int c) override {
        if (m_evb == nullptr) {
            return std::char_traits<char>::eof();
        }

        if (c != std::char_traits<char>::eof()) {
            *pptr() = c;
            pbump(1);
            if (do_flush()) {
                return c;
            }
        }
        return std::char_traits<char>::eof();
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override {
        if (m_evb == nullptr) {
            return std::char_traits<char>::eof();
        }

        std::ptrdiff_t c = pptr() - pbase();
        if (c > 0 && (pptr() + n) >= epptr()) {
            if (!do_flush()) {
                return std::char_traits<char>::eof();
            }
        }
        if (n >= sizeof(m_buffer) - 1) {
            return evbuffer_add(m_evb, s, n) == 0 ? n : std::char_traits<char>::eof();
        } else {
            std::memcpy(pptr(), s, n);
            pbump(n);
            return n;
        }
    }

    int sync() override {
        return do_flush() ? 0 : -1;
    }

private:
    bool do_flush() {
        std::ptrdiff_t n = pptr() - pbase();
        pbump(-n);
        return evbuffer_add(m_evb, pptr(), n) == 0;
    }

private:
    char m_buffer[1024] = {0};
    struct evbuffer* m_evb;
};

static void on_request_completion_cb(struct evhttp_request* req, void* arg);
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

const std::regex CONTENT_ENDPOINT_FOR_DAY(
    "/day\\?year=(\\d{4})&month=(\\d{2})&day=(\\d{2})",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

const std::regex CONTENT_ENDPOINT_ABOUT(
    "/about",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

const std::regex CSS_ENDPOINT(
    ".*/css/([a-zA-Z0-9\\.\\-_]+\\.css)\\??.*",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

const std::regex JS_ENDPOINT(
    ".*/js/([a-zA-Z0-9\\.\\-_]+\\.js)\\??.*",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

const std::regex SVG_ENDPOINT(
    ".*/svg/([a-zA-Z0-9\\.\\-_]+\\.svg)\\??.*",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

WebServer::WebServer(const json& config_json, cl::Db& db)
: m_config_json(config_json)
, m_db(db)
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
    if (!m_config_json.contains("web_port")) {
        return make_unexpected("Port not configured ('web_port')");
    }

    if (!m_config_json.contains("web_root")) {
        return make_unexpected("Web root directory not configured ('web_root')");
    }

    m_web_root = m_config_json["web_root"].get<std::string>();

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
    int port = m_config_json["web_port"].get<int>();
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

expected<bool, std::string> WebServer::handle_redirect_today(struct evhttp_request* req)
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "/day?year=%Y&month=%m&day=%d");
    std::string location = ss.str();

    evhttp_add_header(evhttp_request_get_output_headers(req),
		"Location", location.c_str());

    EventBufferPtr evb(evbuffer_new(), &evbuffer_free);

    evhttp_send_reply(req, HTTP_MOVETEMP, "Redirect", evb.get());

    return true;
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

expected<bool, std::string> WebServer::handle_get_day_request(struct evhttp_request* req)
{
    std::string resource_path = m_web_root + "/day.html";
    return serve_resource(req, resource_path, "text/html; charset=utf-8");
}

expected<bool, std::string> WebServer::handle_about_request(struct evhttp_request* req)
{
    std::string resource_path = m_web_root + "/about.html";
    return serve_resource(req, resource_path, "text/html; charset=utf-8");
}

expected<bool, std::string> WebServer::handle_get_css_request(struct evhttp_request* req, std::string&& css_file)
{
    std::string resource_path = m_web_root + "/css/" + css_file;
    return serve_resource(req, resource_path, "text/css; charset=utf-8", true);
}

expected<bool, std::string> WebServer::handle_get_js_request(struct evhttp_request* req, std::string&& js_file)
{
    std::string resource_path = m_web_root + "/js/" + js_file;
    return serve_resource(req, resource_path, "application/javascript; charset=utf-8", true);
}

expected<bool, std::string> WebServer::handle_get_svg_request(struct evhttp_request* req, std::string&& svg_file)
{
    std::string resource_path = m_web_root + "/svg/" + svg_file;
    return serve_resource(req, resource_path, "image/svg+xml", true);
}

expected<bool, std::string> WebServer::serve_resource(
    struct evhttp_request* req,
    const std::string& path,
    const std::string& content_type,
    bool cache)
{
    auto fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return make_unexpected("Failed to open " + path);
    }

    struct stat st;
    if (::fstat(fd, &st) < 0) {
        ::close(fd);
        return make_unexpected("Failed to stat " + path);
    }

    EventBufferPtr evb(evbuffer_new(), &evbuffer_free);

    evbuffer_add_file(evb.get(), fd, 0, st.st_size);

    auto headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", content_type.c_str());
    if (cache) {
        evhttp_add_header(headers, "Cache-Control", "public, max-age=604800, immutable");
    } else {
        evhttp_add_header(headers, "Cache-Control", "no-store");
    }

    evhttp_send_reply(req, HTTP_OK, "OK", evb.get());

    return true;
}

bool WebServer::send_ok_json_response(struct evhttp_request* req, const json& json_response)
{
    evhttp_request_set_on_complete_cb(req, on_request_completion_cb, this);

    JsonResponseBodyStreamBuf sbuf;
    std::ostream os(&sbuf);
    os << json_response << std::flush;

    set_json_content_type(req);

    evhttp_send_reply(req, HTTP_OK, "OK", sbuf.get());

    return true;
}

bool WebServer::send_internal_error_json_response(struct evhttp_request* req, const std::string& error_msg)
{
    json json_response;
    json_response["error"] = error_msg;

    JsonResponseBodyStreamBuf sbuf;
    std::ostream os(&sbuf);
    os << json_response << std::flush;

    set_json_content_type(req);

    evhttp_send_reply(req, HTTP_INTERNAL , "Internal error", sbuf.get());

    return true;
}

void WebServer::on_request_completion()
{
}


static void on_request_completion_cb(struct evhttp_request* req, void* arg)
{
    WebServer* self = static_cast<WebServer*>(arg);
    self->on_request_completion();
}

static void handle_generic_request(struct evhttp_request* req, void* arg)
{
    WebServer* self = static_cast<WebServer*>(arg);

    auto uri = std::string(evhttp_request_get_uri(req));
    auto command = evhttp_request_get_command(req);

    auto response_result = expected<bool, std::string>(false);
    std::smatch m;

    std::cout << "\t URI " << uri << std::endl;

    if (command == EVHTTP_REQ_GET) {
        if (uri == "/") {
            response_result = self->handle_redirect_today(req);
        } else if (uri == "/api/info") {
            response_result = self->handle_get_info_request(req);
        } else if (std::regex_match(uri, m, API_ENDPOINT_TASK_FROM_ID)) {
            auto id = std::strtol(m[1].str().c_str(), nullptr, 10);
            response_result = self->handle_get_task_request(req, id);
        } else if (std::regex_match(uri, m, API_ENDPOINT_TASKS_FOR_DAY)) {
            std::string param =  m[1].str() + "-" + m[2].str() + "-" + m[3].str();
            response_result = self->handle_get_tasks_for_day_request(req, std::move(param));
        } else if (std::regex_match(uri, m, CONTENT_ENDPOINT_FOR_DAY)) {
            response_result = self->handle_get_day_request(req);
        } else if (std::regex_match(uri, m, CONTENT_ENDPOINT_ABOUT)) {
            response_result = self->handle_about_request(req);
        } else if (std::regex_match(uri, m, CSS_ENDPOINT)) {
            response_result = self->handle_get_css_request(req, std::move(m[1].str()));
        } else if (std::regex_match(uri, m, JS_ENDPOINT)) {
            response_result = self->handle_get_js_request(req, std::move(m[1].str()));
        } else if (std::regex_match(uri, m, SVG_ENDPOINT)) {
            response_result = self->handle_get_svg_request(req, std::move(m[1].str()));
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

class RequestBodyStreamBuf : public std::streambuf
{
public:
    RequestBodyStreamBuf(struct evhttp_request* req)
    : m_evb(nullptr) {
        m_evb = evhttp_request_get_input_buffer(req);
    }

    int underflow() override {
        if (m_evb == nullptr) {
            return std::char_traits<char>::eof();
        }

        if (gptr() == egptr()) {
            auto c = evbuffer_remove(m_evb, &m_buffer[0], sizeof(m_buffer));
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
    auto headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", "application/json; charset=utf-8");
    evhttp_add_header(headers, "Cache-Control", "no-store");
}

static void do_term(int sig, short events, void *arg)
{
	event_base* base = static_cast<event_base*>(arg);
	event_base_loopbreak(base);
    std::cerr << "Got " << sig << ", terminating" << std::endl;
}

}
