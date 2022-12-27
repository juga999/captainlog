#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include <iostream>

using namespace std;

#include <captainlog/task.hpp>
#include <captainlog/db_sqlite.hpp>
#include <captainlog/import_export.hpp>

class DbTestsFixture {
public:
    DbTestsFixture():m_db("") {
        auto res = m_db.open()
        .and_then([&]() {
            return m_db.init_db();
        })
        .map_error([&](auto err) {
            cerr << "Error: " << err << endl;
        });
        m_open_res.emplace(res);
    }

    expected<unsigned int, std::string> import_legacy_csv(const std::string& data) {
        cl::Importer importer(m_db);

        istringstream iss(data);
        auto import_res = importer
            .import_legacy_csv(iss)
            .map_error([&](auto err) {
                std::cerr << "Error: " << err << std::endl;
                return err;
            });
        return import_res;
    }

protected:
    cl::DbSqlite m_db;
    expected<bool, std::string> m_open_res;
};

TEST_CASE_METHOD(DbTestsFixture, "open database in memory", "[db]") {
    REQUIRE(m_open_res.has_value());
}

TEST_CASE_METHOD(DbTestsFixture, "insert task", "[db]") {
    REQUIRE(m_open_res.has_value());

    auto schedule_result = cl::TaskSchedule::create("2022-04-15 15:00:00", "2022-04-15 15:02:30");
    REQUIRE(schedule_result.has_value());

    cl::Task task(*schedule_result, "MyProject", "code cleanup", "dev,refactoring", "my comment");
    auto insert_res = m_db.insert(task);
    REQUIRE(insert_res.has_value());

    auto maybe_latest = m_db.find_latest();
    REQUIRE(maybe_latest.has_value());

    REQUIRE(maybe_latest->id() == 1);
    REQUIRE(maybe_latest->start_str() == "2022-04-15 15:00:00");
    REQUIRE(maybe_latest->tags() == std::set<std::string>{"dev", "refactoring"});
}

TEST_CASE_METHOD(DbTestsFixture, "import legacy CSV", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-05-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.has_value());
    REQUIRE(import_res.value() == 2);
}

TEST_CASE_METHOD(DbTestsFixture, "import legacy CSV error", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-05-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|09:00|08:30|my description|my project|preview,complex|my comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(!import_res.has_value());
}

TEST_CASE_METHOD(DbTestsFixture, "find latest", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-05-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.has_value());

    auto maybe_latest = m_db.find_latest();
    REQUIRE(maybe_latest.has_value());

    REQUIRE(maybe_latest->id() == 1);
    REQUIRE(maybe_latest->start_str() == "2022-05-20 09:00:00");
    REQUIRE(maybe_latest->stop_str() == "2022-05-20 10:30:00");
    REQUIRE(maybe_latest->project() == "my project");
    REQUIRE(maybe_latest->description() == "my description");
    REQUIRE(maybe_latest->comment() == "my comment");
    REQUIRE(maybe_latest->tags() == std::set<std::string>{"preview", "complex"});
}

TEST_CASE_METHOD(DbTestsFixture, "find from id", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-05-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.has_value());

    auto maybe_task = m_db.find_from_id(cl::QueryArgs<cl::Task, int>(2));
    REQUIRE(maybe_task.has_value());

    REQUIRE(maybe_task->id() == 2);
    REQUIRE(maybe_task->start_str() == "2022-04-20 09:00:00");
    REQUIRE(maybe_task->stop_str() == "2022-04-20 10:30:00");
    REQUIRE(maybe_task->project() == "my project");
    REQUIRE(maybe_task->description() == "my description");
    REQUIRE(maybe_task->comment() == "my comment");
    REQUIRE(maybe_task->tags() == std::set<std::string>{"preview", "complex"});
}

TEST_CASE_METHOD(DbTestsFixture, "find latest for day", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|11:00|11:30|my other description|my other project|simple|my other comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.has_value());

    auto maybe_latest = m_db.find_latest_for_day(std::string("2022-04-20"));
    REQUIRE(maybe_latest.has_value());

    REQUIRE(maybe_latest->project() == "my other project");
    REQUIRE(maybe_latest->description() == "my other description");
    REQUIRE(maybe_latest->comment() == "my other comment");
    REQUIRE(maybe_latest->tags() == std::set<std::string>{"simple"});
}

TEST_CASE_METHOD(DbTestsFixture, "find all", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-05-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.value() == 2);

    std::set<std::string> starts;
    auto visit_res = m_db.visit_all([&](auto task) {
        starts.insert(task.start_str());
        return true;
    });
    REQUIRE(visit_res.has_value());
    REQUIRE(starts == std::set<std::string>{"2022-05-20 09:00:00", "2022-04-20 09:00:00"});
}

TEST_CASE_METHOD(DbTestsFixture, "visit from description", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|11:00|11:30|my other description|my other project|simple|my other comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.has_value());

    std::optional<cl::Task> maybe_matched_task;
    auto find_res = m_db.visit_from_description("description", [&](auto task) {
        std::optional temp = std::optional<cl::Task>(std::move(task));
        maybe_matched_task.swap(temp);
        return false;
    });
    REQUIRE(find_res.has_value());
    REQUIRE(maybe_matched_task.has_value());

    cl::Task expected_task(
        cl::TaskSchedule::create("2022-04-20", "11:00", "11:30").value(),
        "my other project",
        "my other description",
        "simple",
        "my other comment"
    );
    REQUIRE(maybe_matched_task.value() == expected_task);
}

TEST_CASE_METHOD(DbTestsFixture, "find at date-time", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|11:00|11:30|my other description|my other project|simple|my other comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.has_value());

    auto maybe_task = m_db.find_at(std::string("2022-04-20 09:15"));
    REQUIRE(maybe_task.has_value());

    REQUIRE(maybe_task->project() == "my project");
    REQUIRE(maybe_task->description() == "my description");
    REQUIRE(maybe_task->comment() == "my comment");
    REQUIRE(maybe_task->tags() == std::set<std::string>{"preview","complex"});

    auto none_task = m_db.find_at(std::string("2020-04-20 09:15"));
    REQUIRE(!none_task.has_value());
}

TEST_CASE_METHOD(DbTestsFixture, "delete task", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";
    oss << "2022-04-20|11:00|11:30|my other description|my other project|simple|my other comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.has_value());

    auto maybe_task =  m_db.find_at(std::string("2022-04-20 09:15"));
    REQUIRE(maybe_task.has_value());

    auto delete_res = m_db.delete_from_id(1);
    REQUIRE(delete_res.has_value());

    auto none_task =  m_db.find_at(std::string("2022-04-20 09:15"));
    REQUIRE(!none_task.has_value());
}

TEST_CASE_METHOD(DbTestsFixture, "update task from json", "[db]") {
    REQUIRE(m_open_res.has_value());

    ostringstream oss;
    oss << "task_date|task_start|task_stop|task_description|task_project|task_tags|task_comment\n";
    oss << "2022-04-20|09:00|10:30|my description|my project|preview,complex|my comment\n";

    auto import_res = import_legacy_csv(oss.str());
    REQUIRE(import_res.has_value());

    auto maybe_task = m_db.find_from_id(cl::QueryArgs<cl::Task, int>(1));
    REQUIRE(maybe_task.has_value());

    json json_task{
        {"id", 1},
        {cl::Task::PROPERTY_START, "2022-04-21 09:15:00"},
        {cl::Task::PROPERTY_STOP, "2022-04-21 10:20:00"},
        {cl::Task::PROPERTY_PROJECT, "my other project"},
        {cl::Task::PROPERTY_DESCRIPTION, "my real description"},
        {cl::Task::PROPERTY_TAGS, {"simple"}},
        {cl::Task::PROPERTY_COMMENT, "changed comment"}
    };

    auto update_res = m_db.update(json_task);
    REQUIRE(update_res.has_value());

    auto maybe_updated_task = m_db.find_from_id(cl::QueryArgs<cl::Task, int>(1));
    REQUIRE(maybe_updated_task.has_value());

    cl::Task expected_task(
        cl::TaskSchedule::create("2022-04-21", "09:15", "10:20").value(),
        "my other project",
        "my real description",
        "simple",
        "changed comment"
    );
    REQUIRE(maybe_updated_task.value() == expected_task);
}