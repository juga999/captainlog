set(COMMON_HEADER_LIST
  "${CaptainLog_SOURCE_DIR}/include/captainlog/expected.hpp"
  "${CaptainLog_SOURCE_DIR}/include/captainlog/common.hpp")

set(TASK_HEADER_LIST
  ${COMMON_HEADER_LIST}
  "${CaptainLog_SOURCE_DIR}/include/captainlog/utils.hpp"
  "${CaptainLog_SOURCE_DIR}/include/captainlog/task.hpp")

set(DB_HEADER_LIST
  ${COMMON_HEADER_LIST}
  "${CaptainLog_SOURCE_DIR}/include/captainlog/db.hpp"
  "${CaptainLog_SOURCE_DIR}/include/captainlog/db_sqlite.hpp"
  "${CaptainLog_SOURCE_DIR}/include/captainlog/db_pg.hpp")

set(IMPORT_EXPORT_HEADER_LIST
  ${COMMON_HEADER_LIST}
  "${CaptainLog_SOURCE_DIR}/include/captainlog/import_export.hpp")

set(REST_HEADER_LIST
  ${COMMON_HEADER_LIST}
  "${CaptainLog_SOURCE_DIR}/include/captainlog/rest.hpp")

# Make an automatic library - will be static or dynamic based on user setting
add_library(captainlog_utils utils.cpp "${CaptainLog_SOURCE_DIR}/include/captainlog/utils.hpp")
add_library(captainlog_task task.cpp ${TASK_HEADER_LIST})
add_library(captainlog_db db.cpp db_sqlite.cpp db_pg.cpp ${DB_HEADER_LIST})
add_library(captainlog_import_export import_export.cpp ${IMPORT_EXPORT_HEADER_LIST})
add_library(captainlog_rest rest.cpp ${REST_HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(captainlog_utils PUBLIC ../include)
target_include_directories(captainlog_task PUBLIC ../include)
target_include_directories(captainlog_db PUBLIC ../include ${SQLite3_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS})
target_include_directories(captainlog_import_export PUBLIC ../include)
target_include_directories(captainlog_rest PUBLIC ../include ${LIBEVENT_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})

target_link_libraries(captainlog_task captainlog_utils)
target_link_libraries(captainlog_db captainlog_task ${SQLite3_LIBRARIES} ${PostgreSQL_LIBRARIES} nlohmann_json::nlohmann_json)
target_link_libraries(captainlog_import_export captainlog_db)
target_link_libraries(captainlog_rest captainlog_db ${Libevent_LIBRARIES} nlohmann_json::nlohmann_json)

# All users of this library will need at least C++11
target_compile_features(captainlog_utils PUBLIC cxx_std_17)
target_compile_features(captainlog_task PUBLIC cxx_std_17)
target_compile_features(captainlog_db PUBLIC cxx_std_17)
target_compile_features(captainlog_import_export PUBLIC cxx_std_17)
target_compile_features(captainlog_rest PUBLIC cxx_std_17)

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${TASK_HEADER_LIST} ${DB_HEADER_LIST}  ${REST_HEADER_LIST})

