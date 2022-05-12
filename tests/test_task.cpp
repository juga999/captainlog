#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include <iostream>

using namespace std;

#include <captainlog/task.hpp>

TEST_CASE( "create valid task schedule", "[task_schedule]" ) {
    auto result = cl::TaskSchedule::create("2022-04-15 15:00:00", "2022-04-15 15:02:00");
    result.map_error([&](auto err) {
        cerr << "Error: " << err << endl;
    });
    REQUIRE(result.has_value());
    auto duration = result->duration_sec();
    REQUIRE(duration == 120);
}

TEST_CASE( "create valid task schedule 3 args", "[task_schedule]" ) {
    auto result = cl::TaskSchedule::create("2022-04-15", "15:00", "15:02");
    result.map_error([&](auto err) {
        cerr << "Error: " << err << endl;
    });
    REQUIRE(result.has_value());
    auto duration = result->duration_sec();
    REQUIRE(duration == 120);
}

TEST_CASE( "create invalid task chronology", "[task_schedule]" ) {
    auto result = cl::TaskSchedule::create("2022-04-15 15:00:00", "2021-04-15 15:02:00");
    REQUIRE(!result.has_value());
}

TEST_CASE( "create invalid task start date", "[task_schedule]" ) {
    auto result = cl::TaskSchedule::create("2022-04-32 15:00:00", "2022-04-15 15:02:00");
    REQUIRE(!result.has_value());
}

TEST_CASE( "create invalid task end date", "[task_schedule]" ) {
    auto result = cl::TaskSchedule::create("2022-04-15 15:00:00", "2022-13-15 15:02:00");
    REQUIRE(!result.has_value());
}

TEST_CASE( "get broken down time", "[task_schedule]" ) {
    auto result = cl::TaskSchedule::create("2022-04-15 15:00:00", "2022-04-15 15:02:30");
    REQUIRE(result.has_value());
    auto [year, month, day, hour, minute] = result->broken_down_start_time();
    REQUIRE(year == "2022");
    REQUIRE(month == "04");
    REQUIRE(day == "15");
    REQUIRE(hour == "15");
    REQUIRE(minute == "00");
}

