configure_file(app_config.h.in app_config.h)

add_executable(captainlog app.cpp)
add_executable(captainlog_cli app_cli.cpp)
target_compile_features(captainlog PRIVATE cxx_std_17)
target_compile_features(captainlog_cli PRIVATE cxx_std_17)
target_include_directories(captainlog PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(captainlog_cli PRIVATE ${PROJECT_BINARY_DIR})

target_link_libraries(captainlog PRIVATE
  captainlog_task
  captainlog_db
  captainlog_rest
  ${SQLite3_LIBRARIES}
  ${LIBEVENT_LIBRARIES}
  nlohmann_json::nlohmann_json)

target_link_libraries(captainlog_cli PRIVATE
  captainlog_task
  captainlog_db
  ${SQLite3_LIBRARIES}
  nlohmann_json::nlohmann_json)

target_compile_options(captainlog PRIVATE -Wall -Wextra)
target_compile_options(captainlog_cli PRIVATE -Wall -Wextra)
