cmake_minimum_required(VERSION 3.14)

project(
  CaptainLog
  VERSION 1.0.0
  DESCRIPTION "Tasks log application"
  LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  message("Build Type not set, defaulting to Debug...")
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  # Let's ensure -std=c++xx instead of -std=g++xx
  set(CMAKE_CXX_EXTENSIONS OFF)

  # Let's nicely support folders in IDEs
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  include(CTest)
endif()

include(FetchContent)

list(APPEND CMAKE_MODULE_PATH "${CaptainLog_SOURCE_DIR}/cmake")

find_package(SQLite3)
find_package(Libevent 2.2 REQUIRED COMPONENTS extra)

FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
message("Git hash: ${GIT_HASH}")

# The compiled library code is here
add_subdirectory(lib)

# The executable code is here
add_subdirectory(app)

# Testing only available if this is the main app
# Emergency override MODERN_CMAKE_BUILD_TESTING provided as well
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING)
   AND BUILD_TESTING)
  add_subdirectory(tests)
endif()

