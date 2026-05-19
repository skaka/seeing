###############################################################################
# Seeing — AI-Native Non-Linear Video Editor
# Cross-Platform CMake Configuration
#
# Supported:
#   • Windows  (x86_64, ARM64)    — MSVC / MinGW
#   • Linux    (x86_64, ARM64)    — GCC / Clang
#   • macOS    (x86_64, ARM64/Apple Silicon) — AppleClang
#
# Usage:
#   cmake -B build -S . -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/<kit>"
#   cmake --build build --config Release
###############################################################################
cmake_minimum_required(VERSION 3.20)

project(Seeing
    VERSION     0.1.0
    DESCRIPTION "AI-Native Non-Linear Video Editor"
    HOMEPAGE_URL "https://github.com/seeing-nle"
    LANGUAGES   CXX
)

# ── C++20 Standard ───────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)       # Strict ISO C++20, no GNU extensions

# ── Qt Automations ───────────────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)                # Qt Meta-Object Compiler
set(CMAKE_AUTORCC ON)                # Qt Resource Compiler
set(CMAKE_AUTOUIC ON)                # Qt UI Compiler

# ── Build Type Defaults (single-config generators only) ──────────────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# ── Architecture Detection (informational logging) ───────────────────────────
message(STATUS "──────────────────────────────────────────────")
message(STATUS "Seeing NLE v${PROJECT_VERSION}")
message(STATUS "System:       ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor:    ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler:     ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "──────────────────────────────────────────────")

# ── Find Qt6 (cross-platform) ───────────────────────────────────────────────
# The user must point CMAKE_PREFIX_PATH to the Qt6 installation directory,
# e.g. "C:/Qt/6.7.0/msvc2019_64" or "/opt/Qt/6.7.0/gcc_64" or
# "$HOME/Qt/6.7.0/macos".
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# Log Qt6 version
message(STATUS "Qt6 version:  ${Qt6_VERSION}")
message(STATUS "Qt6 path:     ${Qt6_DIR}")
message(STATUS "──────────────────────────────────────────────")

# ── Source Files ─────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp

    # Model
    src/model/project_model.cpp
    src/model/project_model.h

    # View
    src/view/main_window.cpp
    src/view/main_window.h
    src/view/media_panel.cpp
    src/view/media_panel.h
    src/view/preview_panel.cpp
    src/view/preview_panel.h
    src/view/timeline_panel.cpp
    src/view/timeline_panel.h
    src/view/timeline_clip_item.cpp
    src/view/timeline_clip_item.h
    src/view/chat_panel.cpp
    src/view/chat_panel.h

    # Controller
    src/controller/editor_controller.cpp
    src/controller/editor_controller.h

    # AI Engine (pluggable interface)
    src/ai/ai_engine_interface.h
    src/ai/dummy_ai_engine.cpp
    src/ai/dummy_ai_engine.h
    src/ai/ai_engine_factory.cpp
    src/ai/ai_engine_factory.h

    # Resources
    resources/resources.qrc
)

# ── Executable ───────────────────────────────────────────────────────────────
qt_add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)

# ── Cross-Platform Compiler Warnings ─────────────────────────────────────────
# Strict warnings across all compilers, no platform-specific flags.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
    )
elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 /wd4100    # Suppress "unreferenced formal parameter"
        /utf-8         # Source and execution charset = UTF-8
    )
endif()

# ── Platform-Specific Target Properties ──────────────────────────────────────
# These are CMake-level properties — no platform-specific C++ code is used.

if(WIN32)
    # Suppress the console window on Windows GUI apps.
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    # Build a proper macOS .app bundle.
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME       "Seeing"
        MACOSX_BUNDLE_BUNDLE_VERSION    "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER    "com.seeing.nle"
        MACOSX_BUNDLE_INFO_PLIST        "${CMAKE_SOURCE_DIR}/platform/macos/Info.plist.in"
    )
    # Universal Binary support (x86_64 + arm64) if not cross-compiling.
    # Users can override with: cmake -DCMAKE_OSX_ARCHITECTURES="arm64"
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING
            "macOS architectures" FORCE)
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux: standard install locations.
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES platform/linux/com.seeing.nle.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )
endif()

# ── Qt Deployment (optional: auto-deploy Qt DLLs/dylibs/so) ─────────────────
# Qt6 provides qt_generate_deploy_app_script() for packaging.
# Uncomment below when preparing release builds:
#
# qt_generate_deploy_app_script(
#     TARGET ${PROJECT_NAME}
#     OUTPUT_SCRIPT deploy_script
#     NO_UNSUPPORTED_PLATFORM_ERROR
# )
# install(SCRIPT ${deploy_script})

# ── Export compile_commands.json for IDE support ─────────────────────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
