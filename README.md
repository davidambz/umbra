# Umbra

[![CI](https://github.com/davidambz/umbra/actions/workflows/ci.yml/badge.svg)](https://github.com/davidambz/umbra/actions/workflows/ci.yml)
[![License: GPLv3](https://img.shields.io/badge/license-GPLv3-blue.svg)](LICENSE)

A lightweight animated desktop background player for Windows. Bring your own video, GIF, or local HTML/CSS/JS page and Umbra plays it behind your desktop icons — no store, no library browser, no bundled content, just whatever you explicitly import.

## Features

- **Video, image, and web backgrounds** — mp4/webm video, static or animated (GIF/APNG) images, or a folder/zip with its own `index.html`.
- **Per-monitor assignment** — a different background (or none) on each display, detected automatically as monitors are connected or disconnected.
- **Playlists** — rotate through several backgrounds on a timer, sequentially or shuffled, per monitor.
- **Power-aware playback** — pauses automatically when a fullscreen app/game has focus or the machine is on battery, so it doesn't compete for GPU/CPU when it matters.
- **Low footprint** — no capture, no compositing beyond what's needed to paint one static/animated layer per monitor.
- **Lock screen sync** — optionally mirrors the current wallpaper as a static lock screen image.
- **Autostart** — launches quietly at sign-in and stays out of the way until you open its Settings window from the tray icon.

## Status

Actively developed. The core rendering/orchestration pipeline, desktop integration (Windows' WorkerW trick), and the Settings UI are all in place and usable day-to-day; see [open issues](https://github.com/davidambz/umbra/issues) for what's still in flight (versioned releases, in particular, aren't set up yet — see below).

## Installing

There's no packaged release yet — for now, build it yourself from source (see below) and run the generated installer.

## Building from source

Umbra is developed on Windows, from a WSL2 shell — the core config/library logic builds and tests on Linux, but the actual app (Win32, Direct3D, Media Foundation, WebView2) needs a real Windows host to build and run.

**Requirements:**
- [CMake](https://cmake.org/) 3.21+
- A C++20 compiler (MSVC via Visual Studio Build Tools, on Windows)
- [vcpkg](https://github.com/microsoft/vcpkg) in manifest mode
- [Node.js](https://nodejs.org/) 22+ (to build `settings-ui/`, the Settings window's front-end)
- [Inno Setup 6](https://jrsoftware.org/isinfo.php) (to build the installer)

```sh
git clone --depth 1 https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics

cmake -B build -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Then build the Settings UI and package the installer:

```sh
cd settings-ui
npm ci
npm run build
cd ../installer
iscc umbra.iss
```

The finished installer lands in `installer/output/`.

## Architecture

Umbra follows a Ports & Adapters (Hexagonal) architecture specifically so its core logic builds and tests on any platform, while every Win32/Direct3D/Media Foundation/WebView2 call sits behind a thin, mockable interface. See `ARCHITECTURE.md` (kept locally, not committed) for the full breakdown, and `TESTING.md` for what's unit-tested versus manually verified.

## License

GPLv3 — see [LICENSE](LICENSE).
