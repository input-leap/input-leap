# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Quick debug build (recommended):
```bash
./clean_build.sh
```

Manual CMake build:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -GNinja
cmake --build build --parallel
```

Key CMake options:
- `-DINPUTLEAP_BUILD_GUI=ON|OFF` — Qt GUI (default ON)
- `-DINPUTLEAP_BUILD_TESTS=ON|OFF` — test suite (default ON)
- `-DINPUTLEAP_BUILD_X11=ON|OFF` — X11/Linux (default ON)
- `-DINPUTLEAP_BUILD_LIBEI=ON|OFF` — Wayland/libei (default OFF)
- `-DQT_DEFAULT_MAJOR_VERSION=5|6`
- `-DCMAKE_UNITY_BUILD=1` — faster unity builds

Binaries land in `build/bin/`, libraries in `build/lib/`.

## Tests

```bash
# All tests
ctest --test-dir build --verbose

# Unit tests only
./build/bin/unittests

# Integration tests only
./build/bin/integrationtests
```

Debug builds treat warnings as errors (`-Werror`). C++ standard is C++17 (Qt6) or C++14 (Qt5).

## Release Notes

Every change requires a towncrier fragment in `doc/newsfragments/` with extension `.feature`, `.bugfix`, `.security`, `.doc`, or `.removal`. See `doc/newsfragments/README.md`.

## Architecture

Input Leap is a software KVM: one machine (server) shares its keyboard/mouse with other machines (clients) over TCP/SSL.

### Processes

| Binary | Role |
|--------|------|
| `input-leaps` | Server — runs on the primary machine, captures and distributes input |
| `input-leapc` | Client — runs on secondary machines, receives input events |
| `input-leap` | Qt GUI — configuration frontend for both server and client |

### Communication

- **Server ↔ Client:** Custom Synergy protocol over TCP with optional SSL/TLS (`src/lib/net/`)
- **GUI ↔ Daemon:** IPC on Windows/macOS (`src/lib/ipc/`)
- **Clipboard:** Serialized and sent over the network protocol (not available on Linux/Wayland)

### Library layout (`src/lib/`)

| Directory | Responsibility |
|-----------|----------------|
| `arch/` | OS-level abstractions (file system, threads, sockets) split into `win32/` and `unix/` |
| `base/` | Event loop, logging, string utilities |
| `mt/` | Threading primitives (mutexes, conditions) |
| `io/` | Stream I/O abstractions |
| `net/` | TCP/UDP sockets, SSL/TLS, protocol framing |
| `ipc/` | Inter-process communication channel |
| `client/` | Client-side protocol state machine |
| `server/` | Server-side protocol state machine, client registry |
| `platform/` | Platform input/output drivers — `MSWindows/`, `XWindows/`, `OSX/`, `Ei/` (libei/Wayland) |
| `inputleap/` | High-level app objects: `ClientApp`, `ServerApp`, config parsing |
| `common/` | Shared data structures and utilities |

### GUI (`src/gui/`)

Qt application (QML + C++) that launches and monitors the daemon processes, manages server configuration (screen layout grid), and handles auto-discovery via Bonjour.

### Test layout (`src/test/`)

| Directory | Contents |
|-----------|----------|
| `unittests/` | Unit tests, platform-specific files compiled conditionally |
| `integtests/` | Integration tests (network, IPC, clipboard) |
| `global/` | Shared test fixtures and utilities |
| `mock/` | GMock-based mocks for interfaces |

### Platform gating

Platform-specific code uses preprocessor guards (`INPUTLEAP_PLATFORM_WINDOWS`, `INPUTLEAP_PLATFORM_MACOS`, `INPUTLEAP_PLATFORM_LINUX`) and is contained inside the matching subdirectory of `src/lib/platform/`.
