cmake_minimum_required(VERSION 3.21)

# On Windows, vcpkg's default dynamic triplet builds gmock.dll with its own
# statically-embedded copy of gtest, separate from gtest.dll. Loading both
# into the same test binary creates two independent test registries: TEST()
# macros register against one, RUN_ALL_TESTS() runs the other, and every
# test silently vanishes ("This test program does NOT link in any test
# case") with no build error. The static triplet links everything into one
# binary with a single registry, avoiding the conflict entirely — and, as a
# side benefit, means the shipped app doesn't need to carry vcpkg DLLs
# alongside it.
if(WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
    set(VCPKG_CRT_LINKAGE "static" CACHE STRING "")
endif()

project(umbra CXX)

# Match the static CRT that vcpkg's x64-windows-static triplet builds
# dependencies against — otherwise MSVC's linker rejects the mix of
# /MD(d) (CMake's own default) and /MT(d) (vcpkg's) with LNK2038.
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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
