# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Umbra is a lightweight Windows app that plays videos, GIFs, and local HTML/CSS/JS pages as an animated background behind the desktop icons. There is no store, library browser, or content editor bundled with the OS install — the app only manages content the user explicitly imports. Never describe this project as, or compare it to, "Wallpaper Engine" in any file, commit, issue, or generated text — the phrase must not appear anywhere in this repo.

Full product context lives in `PRD.md` and `ARCHITECTURE.md` at the repo root. These are intentionally excluded from git (see Repository conventions below) — read them from the local filesystem for context, but do not expect them on GitHub.

## Commands

The only buildable code right now is the platform-independent core (`config/`). Everything under `desktop/`, `render/`, `engines/`, `power/`, `app/`, `ui/` is Windows-only (Win32, Media Foundation, WebView2, Direct3D) and does not exist yet — see the Architecture section below before adding to `CMakeLists.txt`.

Build requires `cmake`, a C++20 compiler, and vcpkg (manifest mode, via `vcpkg.json`). If vcpkg isn't bootstrapped yet:

```
git clone --depth 1 https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
```

Configure, build, and run tests:

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
cd build && ctest --output-on-failure
```

Run a single test by name (GoogleTest filter):

```
./build/tests/unit_tests --gtest_filter='Settings.RoundTripsThroughJson'
```

## Architecture

The codebase follows **Ports & Adapters (Hexagonal)**, chosen specifically because the app targets Windows but development happens partly on Linux/WSL:

- **Core** (`src/config/`, and eventually `src/library/`, `src/playlist/`) — pure C++, zero Win32/OS dependency. This is what builds and unit-tests on any platform, including Linux/CI.
- **Adapters** (`src/desktop/`, `src/render/`, `src/engines/`, `src/power/`, `src/ui/`) — everything that calls a real Windows API (`EnumWindows`, `SetParent`, `EnumDisplayMonitors`, Media Foundation, WebView2, Direct3D). These are Windows-only and can only be built/run on a real Windows host. The core never calls Win32 directly; Win32-dependent behavior is wrapped behind thin interfaces so it can be mocked in tests instead.
- **Orchestrator** (`src/app/application.*`) — the only layer aware of both core and adapters; wires real (or mocked) adapters into the core.

Practical consequence: when adding a new module, decide up front whether it's core or adapter. Core modules go in `CMakeLists.txt`'s `umbra_core` target and get real unit tests today. Adapter modules cannot be compiled or tested outside a Windows host — don't add them to the current CMake target, and don't write tests for them that assume Win32 headers are available.

Key domain-specific mechanism: on Windows, the desktop background window is attached by sending `0x052C` to the `Progman` window to spawn a `WorkerW`, then `SetParent`-ing Umbra's render window into it. This is an undocumented but long-stable trick — see `ARCHITECTURE.md` for the full sequence and the multi-monitor variant (one render window per monitor via `EnumDisplayMonitors`).

Settings UI is a native Win32 window hosting a WebView2 control, with the actual UI built in HTML/CSS/JS (`settings-ui/`, not yet created) — this reuses the WebView2 dependency already required for Web-type wallpapers, rather than pulling in WinUI3. There's a JS↔native bridge (`src/ui/ui_bridge.*`, not yet created) for exposing settings/library/monitor data to that web UI.

## Repository conventions

- **All `.md` files are gitignored except `README.md`** (see `.gitignore`: `*.md` then `!README.md`). Planning docs (`PRD.md`, `ARCHITECTURE.md`, `TESTING.md`) exist locally for context but must never be committed.
- **Everything in English** — code, comments, commit messages, issue titles/bodies.
- **Semantic commit messages** (`feat:`, `fix:`, `chore:`, `docs:`, etc.), with a **detailed body** — not just a one-line subject. The body should explain what changed and why (the reasoning, not a restatement of the diff), so the history reads as a changelog on its own.
- **No AI co-author trailers** in commits (no `Co-Authored-By: Claude`). Commit author is the repo owner's own git identity — do not alter global/local git config to change this.
- Every new source file under `src/` or `tests/` gets the GPLv3 header block (copyright + license notice) at the top — copy it verbatim from an existing file (e.g. `src/config/settings.h`) rather than retyping it.
- New core (non-Win32) modules should ship with GoogleTest unit tests in the same change. New adapter (Win32-boundary) modules should ship with a mock-based test for their wrapper interface, plus a note added to the manual/integration checklist in `TESTING.md`.

## Issue / PR workflow

- Work is tracked as GitHub issues (one per module/part, e.g. "Implement library/ import flow"). Don't push directly to `main` for anything issue-sized.
- Each issue is resolved on its own branch, opened as its own PR — no bundling multiple issues into one PR.
- The PR description must include a `Closes #<issue-number>` line (or `Fixes #<issue-number>`) so merging auto-closes the issue. One PR closes exactly the issue(s) it actually implements — don't reference unrelated issue numbers just to close them out.
- Commits within a PR follow the semantic style above; squash/merge history should stay readable as a changelog (avoid "wip", "fix typo" noise commits surviving to `main`).
- **As soon as a PR is opened, run the `review` skill against it** before it's considered ready to merge. Fix or address what the review surfaces before merging, not after.
