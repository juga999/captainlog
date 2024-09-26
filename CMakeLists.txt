cmake_minimum_required(VERSION 3.10)

project(CaptainLog VERSION 1.0 LANGUAGES CXX)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)

include(FetchContent)

FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP True)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
  fmt
  URL https://github.com/fmtlib/fmt/releases/download/11.0.2/fmt-11.0.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP True)
FetchContent_MakeAvailable(fmt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEV_ENV")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DPROD_ENV")

# Optionally, set build types (Debug, Release)
#set(CMAKE_BUILD_TYPE Release)

set(SOURCES src/main.cpp)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  list(APPEND SOURCES src/prod_setup.cpp)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND SOURCES src/dev_setup.cpp)
endif()

set(TARGET_NAME captainlog)

add_executable(${TARGET_NAME} ${SOURCES})

target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -pedantic)

target_include_directories(${TARGET_NAME} PRIVATE ${WEBKIT2GTK_INCLUDE_DIRS} ${LIBARCHIVE_INCLUDE_DIRS} include)
target_link_libraries(${TARGET_NAME} ${WEBKIT2GTK_LIBRARIES} ${LIBARCHIVE_LIBRARIES} fmt::fmt nlohmann_json::nlohmann_json)
target_compile_options(${TARGET_NAME} PRIVATE ${WEBKIT2GTK_CFLAGS_OTHER} ${LIBARCHIVE_CFLAGS_OTHER})

