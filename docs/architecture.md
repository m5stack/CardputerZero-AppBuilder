# CardputerZero AppBuilder — Desktop IDE Architecture

## Overview

Tauri 2 desktop app (Rust backend + TypeScript/React frontend) for building, debugging, and running M5CardputerZero applications. Targets Windows, macOS, Linux.

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Framework | Tauri 2 (Rust + system WebView) |
| Frontend | React + TypeScript + Vite |
| UI | Tailwind CSS |
| Backend | Rust (tauri commands) |
| Build system | Invokes SCons via subprocess |
| Emulator | QEMU aarch64 user-mode (runs ARM64 ELF on host) |

## Core Features

### 1. Project Management
- Open/create projects based on M5Stack_Linux_Libs SDK scaffold
- Detect `app-builder.json` in project directories
- Git clone/pull from remote repositories

### 2. One-Click Build
- Cross-compile to ARM64 via `aarch64-linux-gnu-` toolchain
- Invoke `scons -j$(nproc)` with `CardputerZero=y` + `CONFIG_REPO_AUTOMATION=y`
- Stream build output to a terminal panel in real-time
- Parse errors and show inline diagnostics
- Package `.deb` via `pack_deb.py`

### 3. One-Click Debug
- SSH to device (`pi@<ip>`) and deploy `.deb` or binary
- Attach GDB via `gdb-multiarch` / `aarch64-linux-gnu-gdb`
- Stream stdout/stderr from device to IDE terminal

### 4. One-Click Emulator
- Launch QEMU user-mode: `qemu-aarch64 -L <sysroot> ./dist/<binary>`
- SDL2 window shows the 320x170 LVGL UI on host
- Install `.deb` into a lightweight aarch64 rootfs for full-stack testing

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                    Tauri Window                          │
│  ┌───────────────────────────────────────────────────┐  │
│  │              React Frontend (WebView)              │  │
│  │                                                    │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐          │  │
│  │  │  Build   │ │  Debug   │ │ Emulator │          │  │
│  │  │  Panel   │ │  Panel   │ │  Panel   │          │  │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘          │  │
│  │       │             │             │                │  │
│  │  ┌────┴─────────────┴─────────────┴────┐          │  │
│  │  │        Terminal Output Panel         │          │  │
│  │  └────────────────────────────────────────┘          │  │
│  └───────────────────────────────────────────────────┘  │
│         │ invoke()          │ events                     │
│  ┌──────┴──────────────────┴──────────────────────┐     │
│  │              Tauri Rust Backend                  │     │
│  │                                                  │     │
│  │  ┌────────────┐ ┌────────────┐ ┌─────────────┐ │     │
│  │  │ BuildMgr   │ │ DeviceMgr  │ │ EmulatorMgr │ │     │
│  │  │            │ │            │ │             │ │     │
│  │  │ scons      │ │ SSH/SCP    │ │ qemu-aarch64│ │     │
│  │  │ dpkg-deb   │ │ GDB remote │ │ rootfs mgmt │ │     │
│  │  └────────────┘ └────────────┘ └─────────────┘ │     │
│  └─────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────┘
```

## Implementation Checklist

### Phase 1: Scaffold (MVP shell)
- [x] Tauri 2 project init with React + TypeScript + Vite
- [x] Basic window with sidebar navigation (Build / Debug / Emulator)
- [x] Rust command: `greet` smoke test

### Phase 2: Build
- [ ] "Open Project" dialog — pick a directory with `app-builder.json`
- [ ] Rust `BuildMgr`: spawn `scons` subprocess, stream stdout via Tauri events
- [ ] Frontend terminal panel (xterm.js) for build output
- [ ] Build status indicator (idle / building / success / error)
- [ ] "Package DEB" button after successful build

### Phase 3: Device & Debug
- [ ] Device connection settings (IP, user, password)
- [ ] Rust `DeviceMgr`: SSH deploy (SCP binary or `dpkg -i`)
- [ ] Remote run: SSH exec and stream output
- [ ] GDB attach (stretch goal)

### Phase 4: Emulator
- [ ] Detect/install QEMU user-mode
- [ ] Rust `EmulatorMgr`: launch `qemu-aarch64` with sysroot
- [ ] SDL2 window forwarding for LVGL UI
- [ ] "Install to emulator" from built `.deb`

### Phase 5: Polish
- [ ] Auto-detect toolchain (aarch64-linux-gnu-gcc, scons, qemu)
- [ ] Toolchain installer wizard
- [ ] Settings persistence
- [ ] Windows / Linux packaging & testing

## Directory Structure

```
CardputerZero-AppBuilder/
├── src-tauri/              # Rust backend
│   ├── src/
│   │   ├── main.rs         # Tauri entry point
│   │   ├── build_mgr.rs    # Build subprocess management
│   │   ├── device_mgr.rs   # SSH/SCP/GDB
│   │   └── emulator_mgr.rs # QEMU management
│   ├── Cargo.toml
│   └── tauri.conf.json
├── src/                    # React frontend
│   ├── App.tsx
│   ├── components/
│   │   ├── Sidebar.tsx
│   │   ├── BuildPanel.tsx
│   │   ├── DebugPanel.tsx
│   │   ├── EmulatorPanel.tsx
│   │   └── TerminalPanel.tsx
│   ├── main.tsx
│   └── styles/
├── .github/workflows/      # CI (existing)
├── scripts/                # Packaging scripts (existing)
├── docs/                   # This file
├── package.json
├── vite.config.ts
└── tsconfig.json
```
