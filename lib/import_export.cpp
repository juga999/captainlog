#include <iostream>
#include <fstream>

#include <captainlog/import_export.hpp>
#include <captainlog/db.hpp>

using tl::make_unexpected;

namespace cl {

static const char CSV_SEPARATOR = '|';

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

expected<unsigned int, std::string> Importer::import_legacy_csv(std::istream& is)
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
                return m_db.insert(Task(
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

expected<unsigned int, std::string> Importer::import_legacy_csv(const std::string& filename)
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

expected<unsigned int, std::string> Exporter::export_legacy_csv(const std::string& filename)
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
    auto visit_res = m_db.visit_all([&](const Task& task) {
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

}
