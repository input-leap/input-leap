# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Input Leap is a cross-platform software KVM switch (keyboard/mouse sharing) — a maintained fork of Barrier. A **server** machine shares its keyboard and mouse with one or more **client** machines over TCP (default port 24800).

## Build Commands

**Quick build (recommended):**
```bash
./clean_build.sh
```

**Manual build:**
```bash
git submodule update --init --recursive
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build
cmake --build build --parallel
```

**Key CMake options:**
- `INPUTLEAP_BUILD_GUI` — build Qt GUI (ON by default)
- `INPUTLEAP_BUILD_TESTS` — build tests (ON by default)
- `INPUTLEAP_BUILD_LIBEI` — Wayland/libei support (OFF by default)
- `QT_DEFAULT_MAJOR_VERSION` — Qt5 (5.9+) or Qt6 (6.2+, default)
- `CMAKE_BUILD_TYPE` — Debug (enables `-Werror`) or Release

## Running Tests

```bash
# All tests
ctest --test-dir build --verbose

# Specific test binary directly
./build/bin/unittests
./build/bin/integtests
```

Tests use Google Test. Unit tests are in `src/test/unittests/`, integration tests in `src/test/integtests/`, mock objects in `src/test/mock/`.

## Code Style

Defined by `.editorconfig`: 4-space indentation, LF line endings, UTF-8, trim trailing whitespace. Debug builds use `-Wall -Wextra -Werror`, so warnings are build failures.

## Architecture

### Client-Server Model

- `input-leaps` (server) runs on the machine with the physical keyboard and mouse. It captures OS input events and routes them to connected clients based on the configured screen layout.
- `input-leapc` (client) connects to the server and injects received input events into its local OS.
- `input-leap` is the Qt GUI that manages configuration and launches the daemon processes.

### Source Layout

```
src/
  lib/
    base/       — logging, events, exceptions, string utilities
    common/     — shared config/options data structures
    net/        — TCP socket layer and protocol framing
    ipc/         — inter-process communication (GUI ↔ daemon)
    inputleap/  — core protocol: message types, client/server handshake
    client/     — client-side input injection logic
    server/     — server-side screen management and event routing
    platform/   — OS-specific implementations (see below)
    arch/        — CPU/OS detection
    io/          — I/O abstractions
    mt/          — threading primitives
  client/       — input-leapc binary entry point
  server/       — input-leaps binary entry point
  daemon/       — Windows service wrapper
  gui/          — Qt GUI application (Qt5 or Qt6)
  test/         — unit tests, integration tests, mocks
```

### Platform Abstraction

`src/lib/platform/` contains OS-specific implementations behind shared interfaces:
- **Windows:** Win32 API (MSWindows* classes)
- **macOS:** Carbon/Cocoa (OSX* classes)
- **Linux/X11:** Xlib/XInput2 (XWindows* classes)
- **Linux/Wayland:** libei + libportal (EI* classes, opt-in via `INPUTLEAP_BUILD_LIBEI`)

Each platform implements interfaces for: screen management, keyboard/mouse injection, clipboard, screen saver detection, and cursor handling.

### Key Dependencies

- **OpenSSL 1.1.1+** — encrypted connections between server and clients
- **Qt 5.9+ or Qt6.2+** — GUI only
- **Linux:** libx11, libxext, libxrandr, libxinerama, libxtst (X11); optionally libei + libportal (Wayland)
- **Bonjour/mDNS:** avahi-compat-libdns-sd (Linux), built-in on macOS/Windows

## Contributing Requirements

User-facing changes **require** a newsfragment file in `doc/newsfragments/` named `<issue-or-pr-number>.<type>` where type is one of: `feature`, `bugfix`, `security`, `doc`, `removal`. This is enforced by the PR template.
