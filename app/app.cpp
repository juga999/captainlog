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
#include <captainlog/web.hpp>

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
    std::cout << "  --import,-i <csv file> " << " = Import a CSV file and exit.\n";
    std::cout << "  --export,-e <csv file> " << " = Export to a CSV file and exist.\n";
    std::cout << "  --resume,-r <text>     " << " = Resume a task from a partial description.\n";
    std::cout << "  --tail,-t <count>      " << " = Print the last <count> tasks.\n";
    std::cout << "  --delete,-d <YYYY-MM-DD HH:mm>|<id>"
              << " = Delete the task for the given date-time or the given id.\n";
    std::cout << "  --web,-w               " << " = Run as a web server on the port 'web_port' from the configuration.\n";
    std::cout << std::endl;
}

static std::string prompt_for_input(const std::string& msg)
{
    std::cout << msg << ": ";
    std::string value;
    std::getline(std::cin, value);
    return cl::utils::trim(value);
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

static bool import_legacy_csv(cl::Db& db, const std::string& filename)
{
    auto result = db.delete_all()
        .and_then([&]() { 
            return db.import_legacy_csv(filename);
        })
        .map([&](auto count) {
            std::cout << "Imported " << count << " entries" << std::endl;
            return true;
        })
        .map_error([&](auto err) {
            std::cerr << "Error: " << err << std::endl;
            return false;
        });
    return result.has_value();
}

static bool export_legacy_csv(cl::Db& db, const std::string& filename)
{
    auto result = db.export_legacy_csv(filename)
        .map([](auto count) {
            std::cout << "Exported " << count << " entries" << std::endl;
            return true;
        })
        .map_error([&](auto err) {
            std::cerr << "Error: " << err << std::endl;
            return false;
        });
    return result.has_value();
}

static bool display_n_latest(cl::Db& db, const std::string& tail_arg)
{
    int count = std::strtol(tail_arg.c_str(), nullptr, 10);
    if (count <= 0) {
        return false;
    }
    auto result = db.visit_n_latest(count, [](cl::Task&& task) {
        std::cout << task << std::endl;
        return true;
    });

    return result.has_value();
}

static bool delete_task(cl::Db& db, const std::string& delete_arg)
{
    std::optional<cl::Task> maybe_task;
    if (delete_arg.find_first_of("-:.")!= std::string::npos) {
        auto normalized_result = cl::utils::normalize_yyyy_mm_dd_hh_mm_date_time(delete_arg);
        if (normalized_result) {
            std::optional<cl::Task> maybe_from_date = db.find_at(normalized_result.value());
            maybe_task.swap(maybe_from_date);
        } else {
            std::cerr << normalized_result.error() << std::endl;
            return false;
        }
    } else {
        int id = std::strtol(delete_arg.c_str(), nullptr, 10);
        auto maybe_from_id = db.find_from_id<cl::Task>(id);
        maybe_task.swap(maybe_from_id);
    }

    if (!maybe_task.has_value()) {
        std::cout << "No task found matching '" << delete_arg << "'" << std::endl;
        return false;
    }
    std::cout << "Delete the following task ?" << std::endl;
    std::cout << '\t' << *maybe_task << std::endl;
    std::string decision = prompt_for_input("[y/n]");
    if (decision == "y" || decision == "Y") {
        auto result = db.delete_from_id(maybe_task->id());
        if (result) {
            std::cout << "Task deleted." << std::endl;
            return true;
        } else {
            std::cerr << result.error() << std::endl;
            return false;
        }
    } else {
        return false;
    }
}

static std::string now_year_month_day_str()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), cl::DATE_FORMAT.c_str());
    return ss.str();
}

static std::optional<std::string>
read_task_date(const std::string& prompt, const std::optional<std::string>& default_date)
{
    std::string msg;
    if (default_date.has_value()) {
        msg = prompt + " [" + *default_date + "]";
    } else {
        msg = prompt;
    }
    auto value = prompt_for_input(msg);
    if (!value.empty()) {
        auto normalized_result = cl::utils::normalize_yyyy_mm_dd_date(value);
        if (!normalized_result) {
            std::cerr << normalized_result.error() << std::endl;
            return std::nullopt;
        }
        auto normalized_value = normalized_result.value();
        std::tm t = {};
        std::istringstream iss(normalized_value);
        iss >> std::get_time(&t, cl::DATE_FORMAT.c_str());
        if (iss.fail()) {
            std::cerr << "Invalid date. Please enter a date like '2020-01-31'" << std::endl;
            return std::nullopt;
        } else {
            return normalized_value;
        }
    } else {
        return default_date;
    }
}

static std::optional<std::string>
read_time(const std::string& prompt, const std::optional<std::string>& default_time)
{
    std::string msg;
    if (default_time.has_value()) {
        msg = prompt + " [" + *default_time + "]";
    } else {
        msg = prompt;
    }
    auto value = prompt_for_input(msg);
    if (!value.empty()) {
        auto normalized_result = cl::utils::normalize_hh_mm_time(value);
        if (!normalized_result) {
            std::cerr << normalized_result.error() << std::endl;
            return std::nullopt;
        }
        auto normalized_value = normalized_result.value();
        std::tm t = {};
        std::istringstream iss(normalized_value);
        iss >> std::get_time(&t, cl::TIME_FORMAT.c_str());
        if (iss.fail()) {
            std::cerr << "Invalid time. Please enter a time like '9:50' or '17.30'" << std::endl;
            return std::nullopt;
        } else {
            return normalized_value;
        }
    } else {
        return default_time;
    }
}

static std::string build_projects_prompt(const std::vector<std::string>& projects)
{
    std::vector<std::string> entries;

    for(std::size_t i = 0; i < projects.size(); ++i) {
        std::ostringstream oss;
        oss << (i+1) << " -> " << projects[i];
        entries.push_back(oss.str());
    }
    entries.push_back("autre");

    return cl::utils::join(entries, ", ");
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
        {"import",  required_argument,  0, 'i'},
        {"export",  required_argument,  0, 'e'},
        {"resume",  required_argument,  0, 'r'},
        {"tail",    required_argument,  0, 't'},
        {"delete",  required_argument,  0, 'd'},
        {"web",     no_argument,        0, 'w'},
        {0,0,0,0},
    };

    std::string import_arg;
    std::string export_arg;
    std::string config_arg;
    std::string resume_arg;
    std::string tail_arg;
    std::string delete_arg;
    bool web_mode = false;

    int index;
    int iarg = 0;
    while (iarg != -1) {
        iarg = getopt_long(argc, argv, "hvc:i:e:r:t:d:w", longopts, &index);
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
            case 'i':
                import_arg = optarg;
                break;
            case 'e':
                export_arg = optarg;
                break;
            case 'r':
                resume_arg = optarg;
                break;
            case 't':
                tail_arg = optarg;
                break;
            case 'd':
                delete_arg = optarg;
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
        ::exit(EXIT_SUCCESS);
    }

    if (!import_arg.empty()) {
        if (import_legacy_csv(db, import_arg)) {
            ::exit(EXIT_SUCCESS);
        } else {
            ::exit(EXIT_FAILURE);
        }        
    }

    if (!export_arg.empty()) {
        if (export_legacy_csv(db, export_arg)) {
            ::exit(EXIT_SUCCESS);
        } else {
            ::exit(EXIT_FAILURE);
        }      
    }

    if (!tail_arg.empty()) {
        if (display_n_latest(db, tail_arg)) {
            ::exit(EXIT_SUCCESS);
        } else {
            ::exit(EXIT_FAILURE);
        }
    }

    if (!delete_arg.empty()) {
        if (delete_task(db, delete_arg)) {
            ::exit(EXIT_SUCCESS);
        } else {
            ::exit(EXIT_FAILURE);
        }
    }

    std::optional<cl::Task> maybe_matched_task;
    if (resume_arg.size() > 0) {
        auto find_res = db.visit_from_description(resume_arg, [&](cl::Task&& task) {
            std::optional temp = std::optional<cl::Task>(std::move(task));
            maybe_matched_task.swap(temp);
            return false;
        });
        if (!find_res) {
            std::cerr << "An error occured while trying to find a matching task: ";
            std::cerr << "\t" << find_res.error();
        }
    }
    if (maybe_matched_task) {
        std::ostringstream oss;
        oss << "Resuming";
        oss << " [" << maybe_matched_task->project() << "]";
        oss << " \"" << maybe_matched_task->description() << "\" ?";
        oss << " [Y/n]";
        auto answer = prompt_for_input(oss.str());
        if (answer == "n") {
            maybe_matched_task.reset();
        }
    }

    std::optional<std::string> task_date;
    auto today = std::optional(now_year_month_day_str());
    while (!task_date.has_value()) {
        task_date = read_task_date("* Date", today);
    }

    std::optional<std::string> default_task_start_time;
    std::optional<cl::Task> maybe_latest = db.find_latest_for_day(*task_date);
    if (maybe_latest) {
        auto [year, month, day, minute, hour] = maybe_latest->schedule().broken_down_end_time();
        default_task_start_time.emplace(minute + ":" + hour);
    }

    std::optional<std::string> task_start_time;
    while (!task_start_time.has_value()) {
        task_start_time = read_time("* Start time", default_task_start_time);
    }

    std::optional<std::string> task_stop_time;
    while (!task_stop_time.has_value()) {
        task_stop_time = read_time("* Stop time", std::nullopt);
    }

    auto task_schedule_result = cl::TaskSchedule::create(*task_date, *task_start_time, *task_stop_time);
    if (!task_schedule_result) {
        std::cerr << "Error:\n";
        std::cerr << "Invalid chronology: ";
        std::cerr << *task_start_time << " -> " << *task_stop_time << std::endl;
        ::exit(EXIT_FAILURE);
    }

    std::string task_project = "";

    if (maybe_matched_task) {
        task_project = maybe_matched_task->project();
        std::cout << "* Projet: " << task_project << std::endl;
    } else {
        std::vector<std::string> favorite_projects;
        if (auto entry = config_json["projects"]; !entry.is_null()) {
            favorite_projects = entry.get<std::vector<std::string>>();
        }

        std::string projects_prompt = build_projects_prompt(favorite_projects);

        int project_choice_index = 0;
        std::string task_project_choice;
        while (task_project_choice.empty()) {
            task_project_choice = prompt_for_input("* Project [" + projects_prompt + "]");
            if (task_project_choice.empty()) {
                std::cerr << "\tThe project must be entered" << std::endl;
            }
            project_choice_index = std::strtol(task_project_choice.c_str(), nullptr, 10);
            if (project_choice_index < 0 || static_cast<unsigned int>(project_choice_index) > favorite_projects.size()) {
                std::cerr << "\tEnter a project number or the name of the project" << std::endl;
                task_project_choice = "";
            }
        }
        if (project_choice_index > 0) {
            task_project = favorite_projects[project_choice_index-1];
        } else {
            task_project = task_project_choice;
        }
    }

    std::string task_description;
    if (maybe_matched_task) {
        task_description = maybe_matched_task->description();
        std::cout << "* Description: " << task_description << std::endl;
    } else {
        while (task_description.empty()) {
            task_description = prompt_for_input("* Description");
            if (task_description.size() == 0) {
                std::cerr << "\tThe description must be entered" << std::endl;
            }
        }
    }

    std::string task_tags_str;
    if (maybe_matched_task) {
        task_tags_str = maybe_matched_task->joined_tags();
        std::cout << "  Tags: " << task_tags_str << std::endl;
    } else {
        task_tags_str = prompt_for_input("  Tags (separated by ,)");
    }

    std::string task_comment = prompt_for_input("  Comment");

    cl::Task task(*task_schedule_result, task_project, task_description, task_tags_str, task_comment);

    auto insert_res = db.insert(task);
    if (!insert_res) {
        std::cerr << "ERROR: " << insert_res.error() << std::endl;
        ::exit(EXIT_FAILURE);
    }

    std::cout << task << std::endl;

    return EXIT_SUCCESS;
}



