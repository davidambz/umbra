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
    # The codebase calls the *W (wide) Win32 APIs explicitly everywhere, but
    # a handful of SDK macros (IDI_APPLICATION, MAKEINTRESOURCE, ...)
    # branch on UNICODE/_UNICODE to pick their ANSI vs. wide expansion
    # regardless of which explicit API variant is called — without these
    # defined, IDI_APPLICATION resolves as an LPSTR and fails to convert
    # to LoadIconW's LPCWSTR parameter.
    add_compile_definitions(UNICODE _UNICODE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)
if(WIN32)
    find_package(unofficial-webview2 CONFIG REQUIRED)
endif()

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
    src/render/fit_rect.cpp
    src/engines/playback_clock.cpp
    src/engines/frame_scheduler.cpp
    src/power/power_state.cpp
    src/power/power_watcher.cpp
    src/power/fullscreen_watcher.cpp
    src/app/monitor_assignment.cpp
    src/app/render_policy.cpp
    src/app/render_tick.cpp
    src/app/autostart.cpp
    src/ui/ui_bridge.cpp
)
target_include_directories(umbra_core PUBLIC src)
target_link_libraries(umbra_core PUBLIC nlohmann_json::nlohmann_json libzip::zip)

# Concrete Win32/Direct3D 11 adapters — only buildable on Windows. Their
# thin interfaces (IMonitorEnumerator, IWorkerWApi) or extracted pure logic
# (fit_rect) live in umbra_core above and are unit-tested; render_surface
# and compositor's D3D11 device/pipeline setup are verified manually (see
# TESTING.md), since a real GPU device can't be meaningfully faked in a
# unit test. d3d11/dxgi/d3dcompiler are Windows SDK components, no vcpkg
# dependency needed.
if(WIN32)
    target_sources(umbra_core PRIVATE
        src/desktop/win32_monitor_enumerator.cpp
        src/desktop/win32_workerw_api.cpp
        src/render/render_surface.cpp
        src/render/compositor.cpp
        src/engines/win32_text.cpp
        src/engines/video_engine.cpp
        src/engines/image_engine.cpp
        src/engines/web_engine.cpp
        src/power/win32_power_api.cpp
        src/power/win32_fullscreen_api.cpp
        src/app/win32_registry_api.cpp
        src/app/application.cpp
        src/ui/settings_window.cpp
        src/desktop/win32_lock_screen_api.cpp
        src/desktop/lock_screen_sync.cpp
        src/engines/wic_png_encoder.cpp
    )
    target_link_libraries(umbra_core PUBLIC
        d3d11 dxgi d3dcompiler
        mfplat mfreadwrite mfuuid
        windowscodecs propsys
        unofficial::webview2::webview2
        dwmapi
        ole32
        windowsapp
    )
    # application.cpp loads the tray icon from resources/umbra.rc's
    # IDI_APP_ICON by id, defined in resources/resource.h.
    target_include_directories(umbra_core PRIVATE resources)

    # The actual GUI app (tray icon, no console window) — everything else
    # is exercised through unit_tests; this target just needs to link and
    # is verified manually against a live desktop session (see TESTING.md).
    # resources/umbra.rc gives the built .exe itself the same violet icon
    # (Explorer, taskbar, Alt-Tab) rather than a generic default one.
    add_executable(umbra WIN32 src/main.cpp resources/umbra.rc)
    target_link_libraries(umbra PRIVATE umbra_core shell32)
endif()

enable_testing()
add_subdirectory(tests)
