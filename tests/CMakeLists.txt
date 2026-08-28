find_package(GTest CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)

add_executable(unit_tests
    config/settings_test.cpp
    config/wallpaper_profile_test.cpp
    library/library_manager_test.cpp
    playlist/playlist_test.cpp
    desktop/monitor_manager_test.cpp
    desktop/workerw_host_test.cpp
    render/fit_rect_test.cpp
    engines/playback_clock_test.cpp
    engines/frame_scheduler_test.cpp
    power/power_state_test.cpp
    power/fullscreen_watcher_test.cpp
    power/power_watcher_test.cpp
    app/monitor_assignment_test.cpp
    app/render_policy_test.cpp
    app/render_tick_test.cpp
    app/autostart_test.cpp
    app/tray_strings_test.cpp
    app/version_compare_test.cpp
    ui/ui_bridge_test.cpp
)
target_include_directories(unit_tests PRIVATE .)

# updater.cpp (Updater's implementation) is only compiled into umbra_core on
# Windows (see the top-level CMakeLists.txt's WIN32 guard) — its test can
# only link/build there too.
if(WIN32)
    target_sources(unit_tests PRIVATE app/updater_test.cpp)
endif()

target_link_libraries(unit_tests PRIVATE umbra_core GTest::gtest GTest::gtest_main GTest::gmock libzip::zip)

include(GoogleTest)
gtest_discover_tests(unit_tests)
