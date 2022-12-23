configure_file(app_config.h.in app_config.h)

add_executable(captainlog app.cpp)
target_compile_features(captainlog PRIVATE cxx_std_17)
target_include_directories(captainlog PRIVATE ${PROJECT_BINARY_DIR})

target_link_libraries(captainlog PRIVATE
  captainlog_task
  captainlog_db
  captainlog_rest
  ${SQLite3_LIBRARIES}
  ${LIBEVENT_LIBRARIES}
  nlohmann_json::nlohmann_json)

target_compile_options(captainlog PRIVATE -Wall -Wextra)
