# Seeing AI — Intelligent Non-Linear Video Editor (NLE)

> **VS Code-inspired, JSON-driven, AI-powered NLE built with C++20 & Qt6.**
> **Fully containerized & cross-platform: Windows · Linux · macOS**

Seeing AI is an advanced, modern non-linear video editor that integrates a **Dual-AI Architecture** directly into the editing workflow. It enables both fully local offline processing (to protect privacy and run without GPU/internet gates) and high-fidelity cloud-based AI workflows for media indexing and automated editing.

---

## ── Dual-AI Architecture

Seeing AI operates on two distinct, independent AI layers, configurable separately via the settings window (`Ctrl + ,`):

### 1. Media Indexing VLM (Visual Language Model)
Responsible for reading and detailing video, image, and audio files upon import:
* **Local Offline (Qwen2-VL-2B)**: Performs CPU/GPU inference entirely inside the container workspace. It checks for local weight existence at `/app/model_weights` and strictly blocks any automatic Hugging Face downloads to avoid unexpected bandwidth usage.
* **Cloud API (OpenAI GPT-4o / Gemini 2.5 Flash)**: Extracts keyframes dynamically from video streams inside the container, encodes them to base64, and sends them to cloud endpoints for high-speed captioning.
* **Interactive Explorer Tooltips**: Generated captions and descriptions are automatically bound to assets and displayed as rich hover tooltips in the Asset Explorer.

### 2. NLE Montage Manager & Chat Copilot
Responsible for executing timeline manipulations and automated video editing from the chat panel:
* **Automated Montage (Sequence Action)**: Translates high-level prompts into a series of timed track edits (`sequence` actions) that are applied recursively to compile imported assets into finished timelines automatically.
* **Interactive Editing Commands**: Supports manual command overrides like adding, trimming, moving, or removing clips, along with native undo/redo.
* **Flexible Backends**: Supports OpenAI (GPT Cloud), Gemini, Ollama (Local LLM like Llama3/Qwen), or a Mock Engine (Dummy AI).

---

## ── System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        MainWindow                               │
│  ┌──────────┬──────────────────────────┬──────────────────────┐ │
│  │  Media   │      Preview Panel       │    AI Copilot        │ │
│  │  Panel   │                          │    Chat Panel        │ │
│  │          ├──────────────────────────┤                      │ │
│  │  (Left)  │    Timeline Panel        │    (Right)           │ │
│  │          │  QGraphicsScene/View     │                      │ │
│  │  Explorer│  Multitrack Timeline     │  Dual AI Assistant   │ │
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

---

## ── Running with Docker / Podman

Seeing AI is fully dockerized to manage C++ dependencies, Qt libraries, X11/Wayland display forwarding, and Python model servers in a rootless, unified workspace:

```bash
# 1. Build the container image
./build.sh

# 2. Run the application
./run.sh
```

---

## ── Local Model Weights Verification

Under `AI` -> `Configure AI Copilot...` (`Ctrl + ,`):
* Select your preferred **VLM Indexer AI** and **NLE Montage Manager**.
* Under the VLM combo box, a real-time status label checks and reports whether local Qwen2-VL weights are successfully mapped at `/app/model_weights` (`🟢 detected` or `🔴 not found`), indicating if you can run local offline captioning or should configure cloud keys.

---

## ── AI Montage Chat Commands

Type these in the chat panel to instruct the co-pilot:

| Category | Command | Effect |
|:---|:---|:---|
| **Auto Montage** | `Make a dynamic montage using all imported assets` | **Sequentially adds and aligns all media on the timeline** |
| **Add Clip** | `add clip Intro on track 0 from 0s to 5s` | Adds a clip at the designated coordinates |
| **Trim Clip** | `cut the first 3 seconds` | Trims duration off the beginning of the earliest clip |
| **Move Clip** | `move clip Intro to 10s` | Shifts clip position to the 10-second mark |
| **History** | `undo` / `redo` | Reverts or repeats timeline modifications |

---

## ── Directory Layout

```
seeing/
├── CMakeLists.txt                  # C++20/Qt6 build system configuration
├── Dockerfile                      # Standardized containerized runtime build
├── build.sh                        # Automation compilation script
├── run.sh                          # GUI display-forwarding run script
├── src/
│   ├── main.cpp                    # Application entry point & Dark theme stylesheet
│   ├── model/
│   │   ├── project_model.h/cpp     # In-memory JSON state representation of NLE timeline
│   ├── view/
│   │   ├── main_window.h/cpp       # Quadrant layout configuration
│   │   ├── media_panel.h/cpp       # Explorer panel with AI description tooltips
│   │   ├── settings_dialog.h/cpp   # Dual-AI configuration & health check dialog
│   │   ├── timeline_panel.h/cpp    # Interactive multitrack timeline graphics
│   │   └── chat_panel.h/cpp        # Co-pilot conversational panel
│   ├── controller/
│   │   └── editor_controller.h/cpp # Routes and triggers AI mutations & sequence execution
│   └── ai/
│       ├── http_ai_engine.h/cpp    # Conversational LLM API client wrapper
│       ├── marlin_server.py        # Gateway routing local Qwen2-VL weights or cloud APIs
│       └── ai_engine_factory.h/cpp # LLM instantiation factory
```

## License

This project is licensed under the MIT License.
