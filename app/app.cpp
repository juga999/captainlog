#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <memory>
#include <optional>
#include <chrono>

#include <getopt.h>

#include <sqlite3.h>

#include <event2/event.h>

#include <nlohmann/json.hpp>

#include "app/app_config.h"

#include <captainlog/utils.hpp>
#include <captainlog/task.hpp>
#include <captainlog/db.hpp>
#include <captainlog/rest.hpp>

namespace fs = std::filesystem;

using json = nlohmann::json;


static json config_json;

static void show_version()
{
    std::cout
        << APP_NAME
        << " version "
        << APP_VERSION
        << " (" << APP_BUILD_TYPE << ")"
        << " [" << APP_GIT_HASH << "]"
        << "\n";
    std::cout << "* SQLite version:   " << sqlite3_libversion() << std::endl;
    std::cout << "* Libevent version: " << event_get_version() << std::endl;
}

static void show_help()
{
    std::cout << "Options" << "\n\n";
    std::cout << "  --help,-h              " << " = Print usage information and exit.\n";
    std::cout << "  --version,-v           " << " = Print version information and exit.\n";
    std::cout << "  --config,-c <json file>" << " = Location of the configuration file.\n";
    std::cout << "  --web,-w               " << " = Run as a web server on the port 'web_port' from the configuration.\n";
    std::cout << std::endl;
}

static bool read_config_from_default_path()
{
    fs::path home_dir_path;
    if (const char* p = std::getenv("HOME")) {
        if (fs::is_directory(fs::status(p))) {
            home_dir_path = p;
        }
    }
    if (home_dir_path.empty()) {
        std::cerr << "$HOME not found" << std::endl;
        return false;
    }

    fs::path config_dir_path = home_dir_path / ".config";
    if (!fs::is_directory(fs::status(config_dir_path))) {
        std::cerr << config_dir_path << " not found" << std::endl;
        return false;
    }

    std::string conf_file_name;
    if (is_debug_profile()) {
        conf_file_name = "captainlog-dev.conf";
    } else {
        conf_file_name = "captainlog.conf";
    }

    fs::path app_config_path = config_dir_path / conf_file_name;
    if (!fs::exists(fs::status(app_config_path))) {
        std::cerr << app_config_path << " not found" << std::endl;
        return false;
    }

    std::ifstream config_stream(app_config_path);
    config_stream >> config_json;

    std::cout << "Configuration file: " << app_config_path << std::endl;

    return true;
}

static bool read_config_from_path(const std::string& p)
{
    if (!fs::exists(fs::status(p))) {
        std::cerr << p << " not found" << std::endl;
        return false;
    }

    fs::path app_config_path(p);

    std::ifstream config_stream(app_config_path);
    config_stream >> config_json;

    std::cout << "Configuration file: " << app_config_path << std::endl;

    return true;
}

static void start_web_server(cl::Db& db)
{
    cl::WebServer web_server(config_json, db);

    auto init_result = web_server.init_server();
    if (!init_result) {
        std::cerr << "An error occured during the server initialization: ";
        std::cerr << init_result.error() << std::endl;
        ::exit(EXIT_FAILURE);
        return;
    }

    web_server.start();
}

int main(int argc, char* argv[])
{
    const struct option longopts[] =
    {
        {"version", no_argument,        0, 'v'},
        {"help",    no_argument,        0, 'h'},
        {"config",  required_argument,  0, 'c'},
        {"web",     no_argument,        0, 'w'},
        {0,0,0,0},
    };

    std::string config_arg;
    bool web_mode = false;

    int index;
    int iarg = 0;
    while (iarg != -1) {
        iarg = getopt_long(argc, argv, "hvc:w", longopts, &index);
        switch (iarg) {
            case 'h':
                show_help();
                ::exit(EXIT_SUCCESS);
                break;
            case 'v':
                show_version();
                ::exit(EXIT_SUCCESS);
                break;
            case 'c':
                config_arg = optarg;
                break;
            case 'w':
                web_mode = true;
                break;
            case '?':
                show_help();
                ::exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }

    if (!config_arg.empty()) {
        if (!read_config_from_path(config_arg)) {
            ::exit(EXIT_FAILURE);
        }
    } else {
        if (!read_config_from_default_path()) {
            ::exit(EXIT_FAILURE);
        }
    }

    std::string db_path;
    if (is_debug_profile() && config_json.contains("database_dev")) {
        db_path = config_json["database_dev"].get<std::string>();
    }
    if (db_path.empty()) {
        if (config_json.contains("database")) {
            db_path = config_json["database"].get<std::string>();
        } else {
            std::cerr << "Invalid configuration: no 'database' entry found" << std::endl;
            ::exit(EXIT_FAILURE);
        }
    }

    cl::Db db(std::move(db_path));
    db.open()
        .and_then([&](){
            return db.init_db();})
        .map_error([&](auto err) {
            std::cerr << "Error: " << err << std::endl;
            ::exit(EXIT_FAILURE);
        });

    if (web_mode) {
        start_web_server(db);
    }

    return EXIT_SUCCESS;
}
