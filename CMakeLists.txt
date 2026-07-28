cmake_minimum_required(VERSION 3.21)
project(umbra CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)

# Platform-independent core: config/data modules with no Win32 dependency.
# Win32-dependent modules (desktop/, engines/, power/, app/) are added here
# once implemented, guarded behind WIN32 checks.
add_library(umbra_core
    src/config/settings.cpp
    src/config/wallpaper_profile.cpp
)
target_include_directories(umbra_core PUBLIC src)
target_link_libraries(umbra_core PUBLIC nlohmann_json::nlohmann_json)

enable_testing()
add_subdirectory(tests)
