#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include <iostream>

using namespace std;

#include <captainlog/task.hpp>
#include <captainlog/db_pg.hpp>
#include <captainlog/import_export.hpp>

class DbPgTestsFixture {
public:
    DbPgTestsFixture()
        : m_db("postgresql://captainlogtest:test1234@localhost:25432/captainlogtest-db")
        , m_db_ready(false)
    {
        m_db.open()
        .and_then([&]() {
            return m_db.init_db();
        })
        .map([&]() {
            m_db_ready = true;
        })
        .map_error([&](auto err) {
            cerr << "Error: " << err << endl;
        });
    }

    ~DbPgTestsFixture() {
        m_db.clear_db();
    }

protected:
    cl::DbPg m_db;
    bool m_db_ready;
};

TEST_CASE_METHOD(DbPgTestsFixture, "open database", "[db]") {
    REQUIRE(m_db_ready);
}

TEST_CASE_METHOD(DbPgTestsFixture, "insert task", "[db]") {
    REQUIRE(m_db_ready);

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
