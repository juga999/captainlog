FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.9
)
FetchContent_MakeAvailable(Catch2)

# Tests need to be added as executables first
add_executable(test_task test_task.cpp)
add_executable(test_db test_db.cpp)
add_executable(test_db_pg test_db_pg.cpp)

target_compile_features(test_task PRIVATE cxx_std_17)
target_compile_features(test_db PRIVATE cxx_std_17)
target_compile_features(test_db_pg PRIVATE cxx_std_17)

target_link_libraries(test_task PRIVATE captainlog_task Catch2::Catch2)
target_link_libraries(test_db PRIVATE captainlog_db captainlog_import_export Catch2::Catch2)
target_link_libraries(test_db_pg PRIVATE captainlog_db captainlog_import_export Catch2::Catch2)

add_test(NAME task COMMAND test_task)
add_test(NAME db COMMAND test_db)
add_test(NAME db_pg COMMAND test_db_pg)
