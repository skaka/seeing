# Seeing — AI-Native Non-Linear Video Editor

> **VS Code-inspired, JSON-driven, AI-powered NLE built with C++20 & Qt6.**
> **Fully cross-platform: Windows · Linux · macOS (x86_64 & ARM64/Apple Silicon)**

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        MainWindow                               │
│  ┌──────────┬──────────────────────────┬──────────────────────┐ │
│  │  Media   │      Preview Panel       │    AI Copilot        │ │
│  │  Panel   │                          │    Chat Panel        │ │
│  │          ├──────────────────────────┤                      │ │
│  │  (Left)  │    Timeline Panel        │    (Right)           │ │
│  │          │  QGraphicsScene/View     │                      │ │
│  └──────────┴──────────────────────────┴──────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘

           ┌─────────┐     ┌────────────────┐     ┌─────────┐
           │  VIEW   │◄────│    MODEL       │────►│  VIEW   │
           │ (Panels)│     │ (ProjectModel) │     │ (Panels)│
           └────┬────┘     └───────▲────────┘     └─────────┘
                │                  │
                │     ┌────────────┴───────────┐
                └────►│     CONTROLLER         │
                      │  (EditorController)    │
                      │         │              │
                      │    ┌────▼────┐         │
                      │    │AI Engine│         │
                      │    │Interface│         │
                      │    └─────────┘         │
                      └────────────────────────┘
```

## Cross-Platform Support

| Platform | Architecture | Compiler | Status |
|----------|-------------|----------|--------|
| Windows 10/11 | x86_64, ARM64 | MSVC 2019+, MinGW | ✅ |
| macOS 11+ | x86_64, Apple Silicon (M1/M2/M3) | AppleClang | ✅ |
| Linux (Ubuntu/Fedora/Arch) | x86_64, ARM64 | GCC 11+, Clang 14+ | ✅ |

### Design Principles for Portability
- **Zero OS-specific headers** — No `<windows.h>`, no POSIX-only code
- **Cross-platform fonts** — Fallback chains: Inter → SF Pro Display → Segoe UI → Noto Sans
- **CMake-only build system** — No platform-specific build scripts in the pipeline
- **Qt6 abstraction** — All system calls go through Qt's cross-platform API

## Prerequisites

- **CMake** ≥ 3.20
- **Qt6** (Widgets module)
- **C++20 compiler**

### Install by Platform

<details>
<summary><b>Windows</b></summary>

```powershell
# Option 1: winget
winget install Kitware.CMake
# Install Visual Studio 2022 with "Desktop development with C++" workload
# Download Qt6 from https://www.qt.io/download-qt-installer

# Option 2: Use the setup script
.\setup.ps1
```
</details>

<details>
<summary><b>macOS</b></summary>

```bash
# Homebrew
xcode-select --install        # Apple Clang
brew install cmake qt@6

# Or use the setup script
chmod +x setup.sh && ./setup.sh
```
</details>

<details>
<summary><b>Linux (Ubuntu/Debian)</b></summary>

```bash
sudo apt update
sudo apt install cmake build-essential qt6-base-dev

# Or use the setup script
chmod +x setup.sh && ./setup.sh
```
</details>

<details>
<summary><b>Linux (Fedora)</b></summary>

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel

chmod +x setup.sh && ./setup.sh
```
</details>

## Build & Run

```bash
# 1. Configure (set your Qt path)
cmake -B build -S . -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/<kit>"

# 2. Build (auto-detects thread count)
cmake --build build --config Release

# 3. Run
# Windows:  .\build\Release\Seeing.exe
# macOS:    open build/Seeing.app
# Linux:    ./build/Seeing
```

### macOS Universal Binary (x86_64 + Apple Silicon)
```bash
cmake -B build -S . \
    -DCMAKE_PREFIX_PATH="$HOME/Qt/6.7.0/macos" \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build --config Release
```

### Quick Start with Qt Creator
1. Open `CMakeLists.txt` in Qt Creator
2. Select your Qt6 kit
3. Build & Run (`Ctrl+R` / `Cmd+R`)

## MVP Test Commands

Type these in the AI Copilot chat panel:

| Command | Effect |
|---------|--------|
| `add demo` | Adds a 10s demo clip on Track 1 |
| `add clip Intro on track 0 from 0s to 5s` | Adds a named clip |
| `cut the first 3 seconds` | Trims 3s from the earliest clip |
| `trim clip Demo to 7s` | Sets clip duration to 7s |
| `move clip Demo to 5s` | Moves clip to 5s position |
| `remove clip Demo` | Removes the clip |
| `undo` | Undo last action |
| `redo` | Redo last action |
| `help` | Show all commands |

## Project Structure

```
seeing/
├── CMakeLists.txt                  # Cross-platform build (Win/Linux/macOS, x64/ARM)
├── README.md
├── setup.ps1                       # Windows auto-setup script
├── setup.sh                        # Linux/macOS auto-setup script
├── platform/
│   ├── macos/Info.plist.in         # macOS .app bundle template
│   └── linux/com.seeing.nle.desktop # Linux .desktop entry
├── resources/
│   └── resources.qrc
└── src/
    ├── main.cpp                    # Entry point, dark theme, MVC wiring
    ├── model/
    │   ├── project_model.h         # Master JSON state (clips, assets, undo)
    │   └── project_model.cpp
    ├── view/
    │   ├── main_window.h/cpp       # 4-panel VS Code layout
    │   ├── media_panel.h/cpp       # Left: asset explorer
    │   ├── preview_panel.h/cpp     # Center-top: video preview placeholder
    │   ├── timeline_panel.h/cpp    # Center-bottom: QGraphicsScene timeline
    │   ├── timeline_clip_item.h/cpp# Custom-painted clip rectangles
    │   └── chat_panel.h/cpp        # Right: AI copilot chat
    ├── controller/
    │   ├── editor_controller.h     # Routes AI → Model mutations
    │   └── editor_controller.cpp
    └── ai/
        ├── ai_engine_interface.h   # Abstract base for all AI backends
        ├── dummy_ai_engine.h/cpp   # MVP mock (keyword parser)
        ├── ai_engine_factory.h     # Factory pattern for engine creation
        └── ai_engine_factory.cpp
```

## Adding a New AI Engine

1. Create a class inheriting `AiEngineInterface`
2. Implement `processPrompt()` returning a JSON mutation object
3. Register it in `AiEngineFactory`
4. Done — the Controller automatically routes to it

## License

MIT
