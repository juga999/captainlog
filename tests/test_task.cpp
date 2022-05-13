#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include <iostream>

using namespace std;

#include <captainlog/utils.hpp>
#include <captainlog/task.hpp>

TEST_CASE("parse hh:mm and hh.mm time", "[parse_time]") {
    {
        std::string input = "9:5";
        auto result = cl::utils::normalize_hh_mm_time(input);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "09:05");
    }
    {
        std::string input = "9.5";
        auto result = cl::utils::normalize_hh_mm_time(input);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "09:05");
    }
    {
        std::string input = "9:50";
        auto result = cl::utils::normalize_hh_mm_time(input);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "09:50");
    }
    {
        std::string input = "19:50";
        auto result = cl::utils::normalize_hh_mm_time(input);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "19:50");
    }
    {
        std::string input = "19.50";
        auto result = cl::utils::normalize_hh_mm_time(input);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "19:50");
    }
    {
        std::string input = "09 50";
        auto result = cl::utils::normalize_hh_mm_time(input);
        REQUIRE(!result.has_value());
    }
}

TEST_CASE("parse YYYY-MM-DD and YYYY.MM.DD time", "[parse_date]") {
    {
        std::string input = "2022-5-3";
        auto result = cl::utils::normalize_yyyy_mm_dd_date(input);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "2022-05-03");
    }
    {
        std::string input = "2022-4-27";
        auto result = cl::utils::normalize_yyyy_mm_dd_date(input);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "2022-04-27");
    }
    {
        std::string input = "2022.2.20";
        auto result = cl::utils::normalize_yyyy_mm_dd_date(input);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "2022-02-20");
    }
    {
        std::string input = "2022-a-20";
        auto result = cl::utils::normalize_yyyy_mm_dd_date(input);
        REQUIRE(!result.has_value());
    }
}

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

