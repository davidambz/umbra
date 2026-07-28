find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests
    config/settings_test.cpp
    config/wallpaper_profile_test.cpp
)

target_link_libraries(unit_tests PRIVATE umbra_core GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
