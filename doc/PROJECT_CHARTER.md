# SeamGate — Project Charter

**Version:** 0.2

**Date:** 2026-05-27

**Status:** Pre-development — planning phase

---

## Table of Contents

- [1. Project Overview](#1-project-overview)
- [2. Context & Background](#2-context--background)
  - [Lineage](#lineage)
  - [Why Fork Rather Than Rewrite](#why-fork-rather-than-rewrite)
  - [License](#license)
- [3. High-Level Goals](#3-high-level-goals)
- [4. Working Style](#4-working-style)
  - [Iterative Delivery](#iterative-delivery)
  - [Human + LLM Agent Collaboration](#human--llm-agent-collaboration)
  - [Merge Policy](#merge-policy)
  - [Feedback Loop](#feedback-loop)
- [5. Principal Resources](#5-principal-resources)
- [6. Standards & Principles](#6-standards--principles)
  - [6.1 C++ Coding Standard](#61-c-coding-standard)
  - [6.2 Rust Coding Standard](#62-rust-coding-standard)
  - [6.3 Architectural Design Principles](#63-architectural-design-principles)
  - [6.4 Test Coverage Standards](#64-test-coverage-standards)
- [7. Architectural Goals](#7-architectural-goals)
  - [7.1 Platform Abstraction Layer](#71-platform-abstraction-layer)
  - [7.2 Protocol Layer](#72-protocol-layer)
  - [7.3 Network Layer](#73-network-layer)
  - [7.4 Rust Integration Strategy](#74-rust-integration-strategy)
- [8. Phase Plan](#8-phase-plan)
  - [Phase 0 — Fork & Foundation](#phase-0--fork--foundation)
  - [Phase 1 — Test Baseline](#phase-1--test-baseline)
  - [Phase 2 — Known Fixes](#phase-2--known-fixes)
  - [Phase 3 — X11 / Wayland Isolation](#phase-3--x11--wayland-isolation)
  - [Phase 4 — C++20 Modernization](#phase-4--c20-modernization)
  - [Phase 5 — Rust: Network / Protocol Layer](#phase-5--rust-network--protocol-layer)
  - [Phase 6+ — Feature Development & Further Rust Migration](#phase-6--feature-development--further-rust-migration)
- [9. Effort Summary](#9-effort-summary)
- [10. Open Decisions](#10-open-decisions)
- [11. Special Considerations](#11-special-considerations)
  - [11.1 Security Model](#111-security-model)
  - [11.2 Wire Protocol Isolation & Versioning](#112-wire-protocol-isolation--versioning)
  - [11.3 Developer Committee & SPOF Prevention](#113-developer-committee--spof-prevention)
  - [11.4 Contributor License Agreement](#114-contributor-license-agreement)
- [12. Governance](#12-governance)
  - [12.1 Guiding Principle](#121-guiding-principle)
  - [12.2 Roles & Merge Rights](#122-roles--merge-rights)
  - [12.3 Branch Strategy](#123-branch-strategy)
  - [12.4 Pull Request Policy](#124-pull-request-policy)
  - [12.5 Issue Triage](#125-issue-triage)
  - [12.6 Continuity](#126-continuity)
  - [12.7 Code of Conduct](#127-code-of-conduct)
- [13. Tools — Now and Later](#13-tools--now-and-later)
  - [13.1 Vetting Criteria](#131-vetting-criteria)
  - [13.2 Current Tools (Active)](#132-current-tools-active)
  - [13.3 Planned Tools (Phase-Gated)](#133-planned-tools-phase-gated)
- [14. Transparent Media & Community Communications](#14-transparent-media--community-communications)
- [15. Performance Baseline & Metrics](#15-performance-baseline--metrics)

---

## 1. Project Overview

**SeamGate** is a cross-platform, open-source KVM software tool enabling a single keyboard and mouse to control multiple computers by moving the cursor across the invisible seam between screens. It is a deliberate, quality-first fork of [Input Leap](https://github.com/input-leap/input-leap), itself descended from Barrier and Synergy.

The name reflects the core concept: the seam between screens becomes a gate — a precise, reliable, bidirectional passage for input and clipboard data.

---

## 2. Context & Background

### Lineage

```
Synergy (2002) → Barrier → Input Leap → SeamGate
```

Input Leap is effectively unmaintained as of late 2025: its primary technical gatekeeper (p12tic) has been absent for 6+ months, leaving 26 unmerged PRs and 780+ open issues. A co-maintainer (shymega) has acknowledged declining motivation. The community is fragmenting toward Deskflow, which has its own architectural regressions.

SeamGate forks Input Leap to preserve and improve its simplicity while addressing the maintenance void through a disciplined, test-driven, iterative engineering approach.

### Why Fork Rather Than Rewrite

- Input Leap's 508-file C++ codebase is mid-sized and structurally sound at the macro level
- The wire protocol compatibility with the existing Barrier/Input Leap ecosystem is an asset — existing deployments become test clients immediately
- Several high-value unmerged upstream PRs (Wayland fixes, low-latency input) provide immediate early wins
- A working cross-platform deliverable from day one avoids the months-long gap of a ground-up rewrite

### License

Input Leap is licensed **GPL-2.0**. SeamGate will be licensed **GPL-2.0-or-later**, preserving compatibility with Input Leap's codebase while allowing future adoption of GPL-3.0 libraries and enabling Rust crate integration under compatible licenses.

---

## 3. High-Level Goals

1. **Reliability** — a KVM tool that works without intervention, reconnects cleanly, and does not lose input events at machine boundaries
2. **Quality** — every module unit-tested, every feature integration-tested; no merge without coverage
3. **Platform parity** — all supported platforms (Windows, macOS, Linux X11, Linux Wayland) are first-class citizens with equivalent test coverage
4. **Architectural clarity** — clean separation between protocol, platform, and UI layers; no platform-specific ifdefs leaking into core logic
5. **Incremental Rust adoption** — migrate high-value modules (network/protocol layer, Wayland/EIS) to Rust as interfaces mature; no flag-day rewrite
6. **Deliverable at every phase** — each phase ends with a shippable, tested binary on all supported platforms

---

## 4. Working Style

### Iterative Delivery
Development proceeds in defined phases. Each phase has explicit acceptance criteria and produces a working, tested release on all platforms before the next phase begins. No phase is considered complete until CI is green on all targets.

### Human + LLM Agent Collaboration
The primary engineering loop is:
1. LLM Agent executes implementation tasks within a phase
2. Human provides architectural judgment, behavioral feedback, and phase-gate approval
3. CI validates correctness mechanically (build, tests, coverage, sanitizers)

The human does not review every line change — that is CI's role. Human judgment is reserved for: "does this feel right?", "is this the correct architecture?", "does this actually work on my machines?"

### Merge Policy
- No merge without passing CI on all platforms
- No merge without tests covering the changed or added behavior
- PRs are small and purposeful — one logical change per PR
- Coverage floor must be maintained or improved on every merge

### Feedback Loop
- Each phase produces a binary the human can run against their real environment (Threadripper 3960X, Fedora 43, Wayland/KWin)
- Bugs found during human testing are filed as issues and triaged into the current or next phase before phase closure

---

## 5. Principal Resources

| Resource | Role |
|---|---|
| [input-leap/input-leap](https://github.com/input-leap/input-leap) | Upstream source — fork origin, protocol reference, bug tracker reference |
| Unmerged PR #2376 | Wayland malfunction fix (srlinuxme) |
| Unmerged PR #2372 | Missing EIS compilation guard in EiScreen.cpp (srlinuxme) |
| Unmerged PR #2378 | Low-latency input mode + macOS mouse fixes (liuli0212) |
| kwin_wayland libeis bug | `eis_device_stop_emulating` missing `eis_device_frame()` call — known issue, fix required in Phase 2 |
| [Deskflow](https://github.com/deskflow/deskflow) | Parallel fork — monitor for protocol changes and bug fixes; do not merge code directly (license compatibility TBD) |
| LiteLLM invalid HTTP probe | Local environment issue (not a SeamGate codebase issue) — document separately |

---

## 6. Standards & Principles

### 6.1 C++ Coding Standard

- **Language version:** C++20 minimum; C++23 features adopted when compiler support is universal across CI targets
- **Style:** LLVM Coding Standards as baseline; enforced via `clang-format` (configuration committed to repo)
- **Static analysis:** `clang-tidy` with a committed `.clang-tidy` configuration; violations block merge
- **Compiler flags:** `-Wall -Wextra -Wpedantic -Werror` on all targets; no suppressed warnings without documented justification
- **Memory management:**
  - No raw owning pointers in non-platform code — use `std::unique_ptr` / `std::shared_ptr`
  - RAII for all resource acquisition (file handles, sockets, platform handles)
  - `std::optional` over nullable pointers for optional values
- **Concurrency:** `std::jthread` and structured concurrency; no raw `std::thread` in new code; `std::atomic` over manual locking where applicable
- **Sanitizers in CI:** AddressSanitizer, ThreadSanitizer, UndefinedBehaviorSanitizer — all must pass on Linux builds

### 6.2 Rust Coding Standard

- **Edition:** Rust 2021+
- **Style:** `rustfmt` with committed `rustfmt.toml`; enforced in CI
- **Lints:** `clippy` with `#![deny(warnings, clippy::all, clippy::pedantic)]`; suppressions require documented justification
- **Safety:** No `unsafe` block without:
  - A comment explaining why it is necessary
  - A comment explaining why it is sound
  - A reviewer sign-off in the PR
- **Dependencies:** `cargo audit` in CI; no dependencies with known vulnerabilities; minimize dependency surface
- **API design:** Follow the [Rust API Guidelines](https://rust-lang.github.io/api-guidelines/) for all public interfaces
- **Error handling:** `thiserror` for library errors; `anyhow` for application-level errors; no `.unwrap()` in library code

### 6.3 Architectural Design Principles

**SOLID**
- *Single Responsibility:* Each class/module has one reason to change. Platform input capture, protocol encoding, and clipboard handling are separate modules.

- *Open/Closed:* Core logic is open for extension (new platforms, new protocol features) without modification via abstract interfaces.

- *Liskov Substitution:* Platform implementations are substitutable — the server does not know or care whether the client is Windows or Wayland.

- *Interface Segregation:* Platform interfaces are split by capability (input capture, input injection, clipboard, display topology) — a platform implements only what it supports.

- *Dependency Inversion:* Core logic depends on abstract platform interfaces, never on concrete platform implementations. Dependency injection, not global state.

**DRY (Don't Repeat Yourself)**
Protocol encoding, platform detection, and configuration parsing exist in one place. Platform-specific branches live in platform modules, not scattered through core logic.

**GRASP**
- *Information Expert:* Assign responsibility to the module that has the information needed to fulfill it.

- *Low Coupling / High Cohesion:* Protocol layer has no knowledge of UI; platform layer has no knowledge of protocol wire format.

- *Controller:* A single session controller owns the state machine for a client or server session.

**Clean Architecture**
Dependencies flow inward only:
```
UI / CLI
    ↓
Application (session management, configuration)
    ↓
Domain (protocol, input events, clipboard model)
    ↓
Platform Interfaces (abstract)
    ↑
Platform Implementations (Win32 / CoreGraphics / X11 / Wayland-EIS)
```
No inner layer imports from an outer layer.

**Isolate, Encapsulate & Version (IEV Pattern)**
This is SeamGate's recurring solution to boundary and compatibility problems. Wherever a subsystem must evolve independently or interoperate with external parties, apply the same three-step approach:
- *Isolate:* draw a hard boundary — no symbols leak across it
- *Encapsulate:* all access goes through a defined interface or API
- *Version:* the interface carries an explicit version; negotiation happens at connection time

This pattern applies to: the wire protocol, configuration file format, the platform abstraction layer, and any future plugin or extension point. It is the mechanism that makes migration paths possible without breaking existing deployments.

**Additional**
- *Composition over inheritance:* Prefer trait implementations (Rust) and interface composition (C++) over deep inheritance hierarchies
- *Explicit over implicit:* Configuration, platform selection, and feature flags are explicit and testable
- *Fail fast:* Invalid state is caught at construction time, not discovered at runtime

### 6.4 Test Coverage Standards

| Test Type | Scope | Minimum Coverage | Enforcement |
|---|---|---|---|
| Unit | Pure logic: protocol encoding, config parsing, clipboard formats, crypto, event mapping | 80% line coverage on `core/`, `protocol/`, `base/`, `net/` | CI blocks merge below floor |
| Integration | Feature-level: connect, keystroke delivery, clipboard round-trip, reconnect | One test per documented feature, all platforms | CI blocks merge on failure |
| End-to-end | Full stack: server + client on loopback, real input injection | Happy path per platform; two-VM tests in CI for Linux | CI blocks merge on failure |
| Regression | Every bug fix ships with a test that would have caught it | 100% of fixed bugs have a covering test | PR template enforces |

**Coverage tooling:** `llvm-cov` / `lcov` for C++; `cargo-llvm-cov` for Rust. Coverage reports generated on every CI run; trend tracked over time.

---

## 7. Architectural Goals

### 7.1 Platform Abstraction Layer

The platform layer is the largest source of complexity (120 files in upstream). The target structure:

```
src/platform/
├── interface/          ← pure abstract C++ interfaces (no platform symbols)
│   ├── IInputCapture.h
│   ├── IInputInjector.h
│   ├── IClipboard.h
│   └── IDisplayTopology.h
├── linux/
│   ├── x11/            ← complete X11 implementation, no Wayland symbols
│   ├── wayland/        ← complete EIS/libeis implementation, no X11 symbols
│   └── common/         ← shared Linux primitives
├── windows/            ← Win32 implementation
└── macos/              ← CoreGraphics/Cocoa implementation
```

X11 and Wayland builds are mutually exclusive compile targets — no `#ifdef HAVE_X11` scattered through shared code.

### 7.2 Protocol Layer

The wire protocol is isolated from both platform and UI. It must be:
- Independently testable without any platform present (mock injection)
- Backward compatible with Input Leap 3.x / Barrier clients
- The primary candidate for Rust rewrite in Phase 5

Protocol isolation, encapsulation, and versioning (IEV Pattern) are applied as an active pre-Phase 5 initiative rather than deferred to the Rust rewrite. This means capability negotiation, version fields, and clean module boundaries are established in C++ first, so the Rust rewrite inherits a well-understood interface rather than a tangled one. See [§11.2](#112-wire-protocol-isolation--versioning) for the full treatment.

### 7.3 Network Layer

Async I/O using platform-appropriate primitives in C++20 (or Tokio in Rust). SSL/TLS support retained. The network layer has no knowledge of input event semantics — it is a typed message transport.

### 7.4 Rust Integration Strategy

The C++/Rust boundary is at the platform interface layer:
- Rust modules expose a C FFI that satisfies the abstract C++ platform interfaces
- `cbindgen` generates C headers from Rust; `bindgen` generates Rust bindings from C headers where needed
- The boundary is narrow and explicit — not a pervasive Rust-in-C++ interop

Target modules for Rust (in priority order):
1. Network/protocol layer (Tokio async, strong type safety on message framing)
2. Wayland/EIS platform implementation (libeis bindings already exist as Rust crates)
3. TLS/crypto layer

---

## 8. Phase Plan

---

### Phase 0 — Fork & Foundation

**Epic:** Establish SeamGate as an independent, building, CI-verified project

**Scope:**
- Fork input-leap; rename project to SeamGate throughout (binary names, config paths, IPC identifiers)
- Configure self-hosted CI runners: macOS and Linux runners are available; Windows runner to be provisioned (deferred — GitHub Actions hosted Windows runner used in interim)
- Set up CI matrix: Windows (MSVC), macOS (latest), Linux X11 (Ubuntu LTS), Linux Wayland (Fedora current)
- Commit `clang-format`, `clang-tidy`, and coverage tooling configurations
- Establish baseline coverage report — measure what exists before changing anything
- Establish pre-Phase-0 performance baseline: measure input latency end-to-end on all platforms before any code changes (see §15)
- License header update to GPL-2.0-or-later across all source files
- Audit config file format: document current schema, confirm full read/write compatibility is preserved
- Inventory current TLS implementation; document all weaknesses as tracked issues (see §11.1)
- Evaluate and select documentation infrastructure (options reviewed with human before committing)
- Create `CODE_OF_CONDUCT.md` and `CONTRIBUTING.md` before first public PR
- Create GitHub org `seamgate`; establish branch protections, merge quorum rules, and issue labels

**Acceptance Criteria:**
- CI green on all four platform targets
- `clang-format` and `clang-tidy` pass with zero violations (or all existing violations documented as tech debt tickets)
- Coverage baseline report published as CI artifact
- Performance baseline report published as CI artifact (pre-change benchmark, all platforms)
- No behavioral difference from upstream Input Leap 3.x — wire protocol compatible
- Config file from upstream Input Leap 3.x loads correctly in SeamGate without modification
- `CODE_OF_CONDUCT.md`, `CONTRIBUTING.md`, and `GOVERNANCE.md` committed

**Effort:** 1–2 weeks (human + LLM Agent)

---

### Phase 1 — Test Baseline

**Epic:** Establish a test suite that makes future changes safe

**Scope:**
- Audit existing 47 test files — classify each as: meaningful / scaffolding / delete
- Write unit tests for all modules in `base/`, `net/`, `io/`, `common/` — these are pure logic with no platform dependency
- Write unit tests for protocol encoding/decoding (message framing, all message types)
- Write integration tests: server starts → client connects → keystroke delivered → verified, on all platforms
- Build mock platform layer (uinput on Linux, SendInput hooks on Windows, CGEvent on macOS) for integration test injection
- Set and enforce coverage floor: **70% line coverage on logic modules** (`base/`, `net/`, `io/`, `protocol/`)

**Acceptance Criteria:**
- Coverage floor met and enforced in CI
- Integration test "connect + keystroke + clipboard" passes on all four CI targets
- No feature changes — binary behavior identical to Phase 0
- Performance report published; no latency regression vs Phase 0 baseline

**Effort:** 2–4 weeks (human + LLM Agent)
*Note: Mock platform layer for Windows/macOS end-to-end testing is the highest-risk item in this phase. Requires human validation on physical machines.*

---

### Phase 2 — Known Fixes & Protocol Foundation

**Epic:** Apply upstream fixes, resolve known bugs, and establish protocol IEV boundaries

**Scope:**
- Cherry-pick and adapt unmerged upstream PRs: #2376 (Wayland fix), #2372 (EIS guard), #2378 (low-latency + macOS mouse)
- Fix `eis_device_stop_emulating` missing `eis_device_frame()` call (kwin_wayland libeis protocol violation)
- Fix LiteLLM-adjacent invalid HTTP health check probe (local environment, not codebase)
- Each fix must include a test that would have caught the bug before the fix was applied
- Enforce TLS-on-by-default; gate plaintext mode behind an explicit opt-in flag with visible warning
- Begin protocol IEV work (see §11.2): isolate protocol module behind a clean interface; add version field and capability negotiation to the handshake; existing behavior is preserved — this is structural groundwork only
- Publish performance delta report: Phase 2 vs Phase 0 baseline

**Acceptance Criteria:**
- All cherry-picked fixes pass CI on all platforms
- `eis_device_frame()` bug no longer appears in kernel logs on Wayland systems
- Every fixed bug has a regression test
- TLS is enabled by default; plaintext requires explicit `--allow-plaintext` flag
- Protocol module has a defined interface boundary; handshake includes version and capability fields
- Backward compatibility test passes: SeamGate ↔ upstream Input Leap 3.x client connects successfully
- Performance report published; no latency regression vs Phase 0 baseline
- Coverage floor maintained or improved

**Effort:** 2–3 weeks (human + LLM Agent)

---

### Phase 3 — X11 / Wayland Isolation

**Epic:** Decouple platform implementations into clean, independently-testable modules

**Scope:**
- Refactor `src/platform/linux/` into the target structure (x11/, wayland/, common/)
- Define and implement the abstract platform interface layer (`IInputCapture`, `IInputInjector`, `IClipboard`, `IDisplayTopology`)
- Enforce build-time isolation: X11 build has no Wayland symbols; Wayland build has no X11 symbols
- Mock implementations of all interfaces for use in unit and integration tests
- Windows and macOS platform code reviewed and aligned to the same interface contracts

**Acceptance Criteria:**
- Separate CMake targets for X11 and Wayland builds — both pass CI
- All platform code sits behind abstract interfaces — no platform symbols in `core/` or `protocol/`
- Mock platform implementations available for all interfaces
- Unit test coverage on interface-dependent code increases (mocks make it testable)
- No behavioral regression on any platform
- Performance report published; no latency regression vs Phase 0 baseline

**Effort:** 3–6 weeks (human + LLM Agent)
*This is the highest architectural risk phase — the Phase 1 test suite is what makes it safe.*

---

### Phase 4 — C++20 Modernization

**Epic:** Bring the codebase to modern C++20 quality standards

**Scope:**
- Replace all raw owning pointers with `std::unique_ptr` / `std::shared_ptr`
- RAII for all resource management (audit for missing destructors, manual cleanup)
- Replace old threading patterns with `std::jthread`, structured concurrency
- `std::optional` over nullable pointers for optional values
- `std::variant` / `std::expected` for error handling in new code
- Sanitizers (ASan, TSan, UBSan) pass cleanly on Linux CI
- Remove deprecated patterns flagged by `clang-tidy`
- Raise coverage floor to **80% on logic modules**

**Acceptance Criteria:**
- Sanitizer-clean Linux CI (ASan + TSan + UBSan)
- Zero raw `new`/`delete` outside of platform-specific code that requires it
- `clang-tidy` passes with no suppressions added beyond those documented in Phase 0
- Coverage floor raised to 80% on `base/`, `net/`, `io/`, `protocol/`
- No behavioral regression — Phase 1 integration tests all pass
- Performance report published; no latency regression vs Phase 0 baseline

**Effort:** 4–8 weeks (human + LLM Agent)

---

### Phase 5 — Rust: Network / Protocol Layer

**Epic:** Replace the network and protocol layer with a Rust implementation

**Scope:**
- New Rust crate: `seamgate-protocol` — wire protocol encoding/decoding, all message types, property tested
- New Rust crate: `seamgate-net` — async TCP transport via Tokio, TLS via rustls
- C FFI boundary generated via `cbindgen`; C++ core calls Rust network layer through the interface
- Wire protocol compatibility test: SeamGate server ↔ upstream Input Leap 3.x client and vice versa
- `cargo-llvm-cov` coverage integrated into CI; Rust modules meet 80% coverage floor

**Acceptance Criteria:**
- Wire protocol compatibility test passes (cross-version integration test in CI)
- All existing Phase 1 integration tests pass against the Rust network layer
- Rust modules are sanitizer-clean (`cargo miri` where applicable)
- Performance report published; input latency equal to or better than Phase 0 baseline
- No C++ network code remains in the non-platform path

**Effort:** 8–16 weeks (human + LLM Agent)
*The FFI boundary and cross-version compatibility testing are the highest-risk items.*

---

### Phase 6+ — Feature Development & Further Rust Migration

**Epic:** Extend SeamGate's capabilities and continue targeted Rust adoption

**Candidate scope (prioritized by community impact):**
- Wayland clipboard support (currently broken in all forks)
- Rust rewrite of Wayland/EIS platform implementation (libeis Rust bindings exist)
- Improved reconnection and session resilience
- Per-client configuration (hotkeys, clipboard policy, latency tuning)
- Packaging: Flatpak, WinGet, Homebrew, RPM/DEB

Each feature in Phase 6+ follows the same gate: implementation + unit test + integration test + CI green on all platforms before merge.

**Effort:** Ongoing — per-feature estimates at phase entry

---

## 9. Effort Summary

| Phase | Description | Effort (Human + LLM Agent) |
|---|---|---|
| 0 | Fork & Foundation | 1–2 weeks |
| 1 | Test Baseline | 2–4 weeks |
| 2 | Known Fixes & Protocol Foundation | 2–3 weeks |
| 3 | X11/Wayland Isolation | 3–6 weeks |
| 4 | C++20 Modernization | 4–8 weeks |
| 5 | Rust: Network/Protocol | 8–16 weeks |
| 6+ | Features & Further Rust | Ongoing |
| **Total to Phase 5** | | **20–39 weeks** |

**Effort model:** Human provides architectural decisions, phase-gate approvals, and real-machine validation. LLM Agent handles implementation, refactoring, test writing, and PR preparation. Human review time per phase estimated at 20–30% of total phase effort.

---

## 10. Open Decisions

| Decision | Options | Status |
|---|---|---|
| GUI toolkit long-term | Keep Qt6 / migrate to Slint (Rust-native) | Deferred to Phase 6 |
| CI runners — Linux & macOS | Self-hosted runners available | **Resolved — use self-hosted** |
| CI runners — Windows | Self-hosted (eventual) / GitHub Actions hosted (interim) | **Resolved — GitHub hosted interim; self-hosted when ready** |
| macOS end-to-end test harness | CGEvent injection in CI / physical machine required | Resolve in Phase 1 |
| `seamgate` crate name reservation | Register on crates.io early | Action before Phase 5 |
| GitHub org creation | `seamgate` name confirmed available | **Action — create org at github.com/organizations/plan before Phase 0** |
| Repository visibility | Public from day one | **Resolved — public** |
| Documentation infrastructure | GitHub Pages / ReadTheDocs / Docusaurus | **Resolved — GitHub Pages** |
| Config file format | Full upstream compatibility (default); IEV versioning for future migration | **Resolved — full compatibility; IEV pattern for evolution** |
| CMake + Cargo integration tool | Corrosion (`cmake-rs/corrosion`) pending vetting | Evaluate in Phase 4; must meet vetting criteria (§13.1) |
| Contributor License Agreement | Require CLA / no CLA (GPL sufficient) | **Resolved — no CLA; GPL-2.0-or-later is sufficient** |
| Dependency security scanner | Dependabot / OSV-Scanner | **Resolved — Dependabot (GitHub default)** |
| Wire protocol design | Follow charter IEV plan; revisit detail pre-Phase 3 | **Resolved — proceed as designed; full review before Phase 3** |

---

## 11. Special Considerations

The table below provides a prioritised at-a-glance view of all active special considerations. Full detail is in the subsections below.

| # | Consideration | Priority | Status |
|---|---|---|---|
| 11.1 | Security Model | **Critical** | Active — phased implementation |
| 11.2 | Wire Protocol Isolation & Versioning | **High** | Active — pre-Phase 5 work begins Phase 2 |
| 11.3 | Developer Committee & SPOF Prevention | **High** | Active — addressed in §12 Governance |
| 11.4 | Contributor License Agreement | **Medium** | Pending offline discussion — not blocking |

---

### 11.1 Security Model

KVM software sits in a uniquely sensitive position: a compromised or misconfigured server can inject arbitrary keystrokes, read clipboard contents, and observe all input activity on every connected client machine. This is a higher-trust boundary than most application software and must be treated as such from the first commit.

**Current State (Input Leap Inherited)**

Input Leap's TLS implementation has several weaknesses:
- Self-signed certificates with no chain of trust
- No mutual authentication — the client verifies the server, but the server does not verify the client
- First-connect trust is entirely manual (user must visually compare certificate fingerprints)
- No protection against a rogue server on the same LAN intercepting a client's first connection
- Clipboard data is encrypted in transit only if TLS is enabled; TLS is optional, not enforced by default

These weaknesses are acceptable in a trusted home LAN but are not acceptable as a stated security posture for a project that aims to do things right.

**SeamGate Security Requirements**

| Requirement | Detail |
|---|---|
| TLS mandatory | TLS is on by default; plaintext mode requires explicit opt-in with a visible warning |
| TOFU with pinning | Trust-On-First-Use for initial connection; certificate pinned thereafter; any change requires explicit re-approval |
| Mutual authentication | Server and client each present a certificate; neither accepts an uncredentialed peer |
| MITM resistance | Pinned certificates detect substitution on subsequent connections; fingerprint displayed and loggable |
| Clipboard encryption | All data in transit — keystrokes, clipboard, and control messages — protected by the same TLS session |
| Network exposure | Server binds to localhost by default; LAN binding requires explicit configuration |
| Credential storage | Private keys stored with OS-appropriate permissions (600 on Linux/macOS, ACL-restricted on Windows); never in world-readable locations |
| Audit logging | All connection events (connect, disconnect, auth failure, cert mismatch) logged at INFO level; security events logged at WARN |

**Implementation Approach**

The TLS layer is an early Rust migration candidate, independent of the Phase 5 network/protocol rewrite:

- **`seamgate-tls` crate** — wraps `rustls` (memory-safe, no OpenSSL dependency) with SeamGate's TOFU-with-pinning model
- Replaces the inherited OpenSSL C++ binding as a self-contained FFI module
- Can be introduced as an optional parallel implementation in Phase 2 or 3, replacing the C++ TLS path entirely before Phase 5 begins
- `rustls` eliminates the OpenSSL CVE surface and removes a significant C++ dependency from the build

**Certificate Management UX**

The user experience for certificate trust must not be an afterthought:
- On first server start: generate a self-signed certificate, display the fingerprint prominently
- On first client connect: display the server fingerprint, require explicit user confirmation before pinning
- On fingerprint mismatch (subsequent connect): hard block with a clear warning — do not silently accept
- CLI and GUI both expose `seamgate verify-cert <server>` for out-of-band fingerprint confirmation

**Phase Implications**

| Phase | Security Work |
|---|---|
| 0 | Inventory current TLS implementation; document all known weaknesses as tracked issues |
| 1 | Add security-specific integration tests: rejected plaintext connect, cert mismatch rejection, key permission checks |
| 2 | Enforce TLS-on-by-default; remove or gate plaintext mode behind explicit flag |
| 3–4 | No regression on security tests during platform isolation and C++20 modernization |
| 5 | Introduce `seamgate-tls` Rust crate; replace C++ TLS path; mutual authentication implemented |

---

### 11.2 Wire Protocol Isolation & Versioning

**Priority:** High — active pre-Phase 5 initiative
**Status:** Groundwork begins Phase 2; full encapsulation complete by end of Phase 4

The wire protocol connects SeamGate to all existing Barrier, Input Leap, and Deskflow clients in the field. Any change to the protocol without a versioning and negotiation mechanism risks silently breaking those connections. Equally, without clean module isolation, the Phase 5 Rust rewrite would need to understand and replicate tangled C++ internals rather than replacing a well-defined interface.

Applying the IEV Pattern (§6.3) to the protocol means:

- **Isolate:** protocol encoding/decoding lives entirely within `src/protocol/` — no protocol framing logic in transport, platform, or UI code
- **Encapsulate:** all access to the protocol module goes through a versioned C++ interface (`IProtocolSession`) with no direct struct access from outside
- **Version:** the handshake message carries a `protocol_version` field and a `capabilities` bitmask; peers negotiate the highest mutually supported version on connect; unknown capabilities are ignored gracefully

**Backward compatibility contract:**
- SeamGate must successfully connect to an unmodified Input Leap 3.x server or client for the lifetime of the project
- New protocol features are additive only and gated behind capability bits
- A compatibility test (SeamGate ↔ upstream Input Leap 3.x) runs in CI on every merge to `dev`

**Note:** This work is intentionally scoped as structural groundwork in Phase 2 — the user has flagged this as an area requiring further study before full design decisions are made. The Phase 2 scope is the minimum necessary to avoid locking in the current tangled state. Full protocol design will be reviewed with the human before Phase 3 begins.

**Phase implications:**

| Phase | Protocol Work |
|---|---|
| 2 | Add `IProtocolSession` interface; version + capability fields in handshake; compatibility test in CI |
| 3 | Protocol module fully isolated; no protocol symbols outside `src/protocol/` |
| 4 | Protocol interface finalized and documented; ready for Rust rewrite |
| 5 | Rust `seamgate-protocol` crate implements `IProtocolSession` via FFI |

---

### 11.3 Developer Committee & SPOF Prevention

**Priority:** High — structural requirement before first public PR
**Status:** Active — addressed in §12 Governance

Input Leap's failure was a single point of failure in its review and merge process. SeamGate eliminates this by design. The governance model in §12 establishes:

- A two-Maintainer quorum requirement for all merges to `main`
- An auto-promotion mechanism if the Maintainer count drops to zero
- Credentials held by minimum two people at all times

As SeamGate grows, the goal is to establish a small **Maintainer Committee** of 3–5 people with distributed expertise (platform knowledge across Windows, macOS, Linux; C++ and Rust; security). No single committee member's absence should block the project.

Recruitment of the committee is a Phase 0 / Phase 1 priority alongside technical work. The human founder leads this initially, with the explicit goal of not remaining the only Maintainer past Phase 1.

---

### 11.4 Contributor License Agreement

**Priority:** Medium
**Status:** Resolved — no CLA required

GPL-2.0-or-later governs contribution terms and is sufficient for SeamGate's current goals. No CLA will be required from contributors.

If future circumstances change this calculus — dual-licensing a module, foundation hosting, or commercial involvement — the decision can be revisited at that time. Retrofitting a CLA is difficult but not impossible if the contributor base remains small in early phases. The decision is recorded here so it is not made accidentally by omission.

---

## 12. Governance

### 12.1 Guiding Principle

Input Leap's collapse was a governance failure, not a technical one. A single gatekeeper — p12tic — became the only path to `master`, and when he went quiet, 26 PRs and 780 issues stalled indefinitely. SeamGate explicitly designs against this from day one: no single person can block progress, and no single person's absence can stall the project.

### 12.2 Roles & Merge Rights

| Role | Rights | How Earned |
|---|---|---|
| **Maintainer** | Merge to `main`, approve PRs, manage releases, triage issues | Sustained quality contributions over multiple phases; granted by existing Maintainers by consensus |
| **Committer** | Merge to `dev`, approve PRs (counts toward quorum) | Consistent contributions with demonstrated code quality and test discipline; nominated by a Maintainer |
| **Contributor** | Open PRs, comment, participate in design discussions | Anyone — no approval required |
| **LLM Agent** | Opens PRs, never merges directly | All LLM-generated changes go through the normal PR + human approval process |

**Minimum quorum to merge:**
- To `dev`: one Committer or Maintainer approval (excluding the PR author)
- To `main`: two Maintainer approvals (excluding the PR author)
- Security-related changes: two Maintainer approvals + one independent security review comment

The LLM Agent is a contributor, not an approver. It opens PRs; humans merge them.

### 12.3 Branch Strategy

```
main        ← stable, release-tagged; protected, requires PR + quorum
dev         ← integration branch; CI must be green before merge to main
feature/*   ← short-lived feature branches off dev
fix/*       ← bug fix branches off dev (or main for hotfixes)
phase/*     ← phase-scoped working branches; merged to dev at phase completion
```

- `main` and `dev` are protected branches — no direct push, ever
- Force push is permanently disabled on both
- `main` is tagged at each phase completion release; tags are signed

### 12.4 Pull Request Policy

Every PR must satisfy all of the following before merge:

- [ ] CI passes on all platform targets (Windows, macOS, Linux X11, Linux Wayland)
- [ ] Test coverage maintained or improved — coverage floor never drops on merge
- [ ] New behavior has unit tests; new features have integration tests
- [ ] Bug fixes include a regression test that would have caught the bug
- [ ] `clang-format` / `rustfmt` clean — no style violations
- [ ] `clang-tidy` / `clippy` clean — no lint suppressions added without documented justification
- [ ] PR description explains *why*, not just *what* (the diff explains what)
- [ ] Breaking changes or protocol changes flagged explicitly in the PR description
- [ ] Required approvals obtained per quorum rules above

PRs that touch the security model, TLS implementation, or certificate handling require an explicit security review note from a Maintainer separate from the approval.

### 12.5 Issue Triage

Issues are triaged on a weekly cadence by any Maintainer or Committer. Triage assigns:
- **Severity label:** `critical` / `high` / `medium` / `low`
- **Phase label:** which phase this belongs to (`phase-0` through `phase-6+` or `backlog`)
- **Type label:** `bug` / `feature` / `chore` / `security` / `docs`

Issues older than 90 days with no activity and no phase assignment are labelled `stale` and closed after a 14-day notice comment if no response.

The upstream Input Leap issue tracker is monitored for bugs that affect SeamGate. Any upstream bug confirmed to affect SeamGate is filed as a new issue (not referenced by URL only — upstream issues may disappear).

### 12.6 Continuity

To prevent a recurrence of the Input Leap situation:

- A minimum of **two active Maintainers** is the target at all times
- If only one Maintainer is active for more than 60 days, an open call for Committer promotion or new Maintainer recruitment is posted in the project's community channel
- If the project reaches a state with zero active Maintainers, the most active Committer(s) are automatically elevated to Maintainer by virtue of sustained activity — this is documented here and requires no additional vote
- All credentials (signing keys, release tokens, CI secrets) are held by at least two Maintainers at all times; single-holder credentials are a governance bug to be fixed immediately

### 12.7 Code of Conduct

SeamGate adopts the [Contributor Covenant v2.1](https://www.contributor-covenant.org/version/2/1/code_of_conduct/) as its Code of Conduct. It applies to all project spaces: GitHub issues, PRs, discussions, IRC/chat, and any other SeamGate-affiliated venue.

Reports are handled by the Maintainer group. A Maintainer who is the subject of a report recuses themselves from that process.

A `CODE_OF_CONDUCT.md` and `CONTRIBUTING.md` are Phase 0 deliverables, committed before the first public PR is accepted.

---

## 13. Tools — Now and Later

### 13.1 Vetting Criteria

All tooling adopted by SeamGate must satisfy the following before being committed to the project:

| Criterion | Requirement |
|---|---|
| **Active maintenance** | Last release within 12 months; open issues acknowledged |
| **Security posture** | No known unpatched CVEs; dependency audit clean |
| **Reliability** | Used in production by comparable projects; not experimental |
| **Drama-free** | No licence controversies, abandoned maintainer situations, or hostile forks in recent history |
| **Fit** | Solves the problem without introducing excessive complexity or new language dependencies |

Tools in "TBD" status below must be vetted against these criteria before adoption. Human sign-off required before any TBD tool is introduced into CI or the build.

### 13.2 Current Tools (Active)

| Tool | Purpose | Language | Status |
|---|---|---|---|
| `clang-format` | C++ code formatting | C++ | Active — Phase 0 |
| `clang-tidy` | C++ static analysis | C++ | Active — Phase 0 |
| `llvm-cov` / `lcov` | C++ coverage reporting | C++ | Active — Phase 0 |
| `rustfmt` | Rust code formatting | Rust | Active — Phase 5 |
| `clippy` | Rust linting | Rust | Active — Phase 5 |
| `cargo-llvm-cov` | Rust coverage reporting | Rust | Active — Phase 5 |
| `cargo audit` | Rust dependency vulnerability scan | Rust | Active — Phase 5 |
| `cbindgen` | Rust → C header generation for FFI | Rust/C++ | Active — Phase 5 |
| `cmake` | Build system | C++ | Active — Phase 0 |
| Qt6 | GUI framework | C++ | Active — inherited from Input Leap |

### 13.3 Planned Tools (Phase-Gated)

| Tool | Purpose | Phase | Status |
|---|---|---|---|
| **Corrosion** (`cmake-rs/corrosion`) | Integrates Cargo into CMake build | Phase 5 | **TBD — vetting required before adoption** |
| `cargo miri` | Rust undefined behaviour detection | Phase 5 | TBD — evaluate against vetting criteria |
| `bindgen` | C → Rust binding generation | Phase 5 | TBD |
| `tokio` | Async runtime for Rust network layer | Phase 5 | TBD — widely used, expected to pass vetting |
| `rustls` | TLS implementation (replaces OpenSSL) | Phase 2–3 | TBD — widely used, expected to pass vetting |
| OSV-Scanner / Dependabot | C++ dependency vulnerability scanning | Phase 0 | TBD — select one before Phase 0 closes |
| Documentation platform | User + developer docs hosting | Phase 0 | TBD — human decision required (see §10) |
| Slint | Rust-native GUI framework (potential Qt replacement) | Phase 6 | TBD — deferred |

---

## 14. Transparent Media & Community Communications

SeamGate operates in the open. Development decisions, architectural choices, performance results, and phase milestones are published as they happen — not summarised retrospectively.

### What We Publish

| Content | Audience | Cadence |
|---|---|---|
| Phase completion announcements | General public / KVM software community | At each phase gate |
| Performance benchmark reports | Technical community | Every phase (pre and post) |
| Architecture decision records (ADRs) | Contributors and technical users | As decisions are made |
| Security advisories | All users | Immediately on confirmed vulnerability |
| Roadmap updates | All users | When phase plan changes materially |
| Release notes | All users | Every tagged release |

### Where We Publish

Channels to be selected in Phase 0. Candidates:

| Channel | Purpose | Status |
|---|---|---|
| GitHub Releases | Binaries + release notes | Confirmed — Phase 0 |
| GitHub Discussions | Community Q&A, roadmap discussion | Confirmed — Phase 0 |
| Project website / docs | Architecture docs, user guides, ADRs | TBD — platform decision pending (§10) |
| Mastodon / social | Announcements to broader open-source community | TBD |
| Matrix / IRC | Real-time contributor discussion (Input Leap used Libera IRC) | TBD |

### When We Announce

The project is not announced publicly until a Phase 0 deliverable exists — a working, CI-green fork with `CODE_OF_CONDUCT.md` and `CONTRIBUTING.md` in place. Announcing intent before substance invites noise and disappointment. The first public communication is a release, not a promise.

### Tone & Principles

- No vaporware — only announce what is shipped or imminently shipping
- Acknowledge upstream (Input Leap, Barrier, Synergy) respectfully — SeamGate exists because of their work
- Performance claims are backed by published benchmark data
- Security issues are disclosed responsibly: coordinated disclosure first, public advisory after a reasonable remediation window

---

## 15. Performance Baseline & Metrics

### Rationale

Input latency is the primary user-perceptible quality metric for KVM software. Every refactoring phase carries regression risk. Without a measured baseline, "no regression" is an assertion, not a fact.

SeamGate measures input latency before the first code change and publishes a delta report at the close of every phase.

### What We Measure

| Metric | Description | Tool |
|---|---|---|
| **Keystroke latency** | Time from key press on server to key event on client (end-to-end) | Custom harness using `uinput` injection + `evdev` capture timestamps |
| **Mouse warp latency** | Time from cursor edge-crossing to cursor position update on client | Custom harness |
| **Clipboard round-trip** | Time from server clipboard set to client clipboard available | Custom harness |
| **Reconnect time** | Time from network interruption to session re-establishment | Integration test measurement |

### Measurement Protocol

- **Pre-Phase-0 baseline:** collected on the unmodified Input Leap 3.x binary, same hardware as the primary development machine (Threadripper 3960X, Fedora 43, Wayland/KWin), before the fork is made
- **Per-phase:** collected at phase close on the phase deliverable binary, same hardware and conditions
- **Platforms:** Linux Wayland (primary); Linux X11, macOS, Windows (secondary — best effort per phase)
- **Sample size:** minimum 1,000 samples per metric; report median, p95, p99, and max
- **CI integration:** a lightweight latency smoke test (100 samples, median only) runs in CI on the Linux loopback path as a regression gate — full benchmark suite is run manually at phase close

### Publication

Performance reports are published as:
- A CI artifact for every phase-close build
- A summary table in the phase release notes
- A running comparison chart on the project website (once documentation infrastructure is established)

Regressions of >10% on median latency are a phase-close blocker. Regressions of >25% on p99 are a phase-close blocker. All other regressions are tracked as issues and triaged into the next phase.

---

*This charter is a living document. Update version and date on each substantive revision.*
