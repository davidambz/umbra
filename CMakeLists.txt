cmake_minimum_required(VERSION 3.21)
project(umbra CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)

# Platform-independent core: config/data/library modules with no Win32
# dependency. Win32-dependent modules (desktop/, engines/, power/, app/) are
# added here once implemented, guarded behind WIN32 checks.
add_library(umbra_core
    src/config/settings.cpp
    src/config/wallpaper_profile.cpp
    src/library/library_manager.cpp
    src/playlist/playlist.cpp
    src/desktop/monitor_manager.cpp
    src/desktop/workerw_host.cpp
)
target_include_directories(umbra_core PUBLIC src)
target_link_libraries(umbra_core PUBLIC nlohmann_json::nlohmann_json libzip::zip)

# Concrete Win32 adapters (real EnumDisplayMonitors/WorkerW-hack calls) —
# only buildable on Windows. Their thin interfaces (IMonitorEnumerator,
# IWorkerWApi) live in umbra_core above and are unit-tested with mocks;
# these implementations are verified manually (see TESTING.md).
if(WIN32)
    target_sources(umbra_core PRIVATE
        src/desktop/win32_monitor_enumerator.cpp
        src/desktop/win32_workerw_api.cpp
    )
endif()

enable_testing()
add_subdirectory(tests)
