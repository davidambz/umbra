find_package(GTest CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)

add_executable(unit_tests
    config/settings_test.cpp
    config/wallpaper_profile_test.cpp
    library/library_manager_test.cpp
    playlist/playlist_test.cpp
    desktop/monitor_manager_test.cpp
    desktop/workerw_host_test.cpp
)
target_include_directories(unit_tests PRIVATE .)

target_link_libraries(unit_tests PRIVATE umbra_core GTest::gtest GTest::gtest_main GTest::gmock libzip::zip)

include(GoogleTest)
gtest_discover_tests(unit_tests)
