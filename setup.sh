#!/usr/bin/env bash
###############################################################################
# Seeing NLE — Cross-Platform Build Script (Linux / macOS)
###############################################################################

set -euo pipefail

echo ""
echo "============================================"
echo "  Seeing NLE — Build Setup"
echo "============================================"
echo ""

# ── Check Prerequisites ──────────────────────────────────────────────────────

HAS_CMAKE=false
HAS_COMPILER=false
HAS_QT6=false

# Check CMake
if command -v cmake &> /dev/null; then
    echo "[OK] $(cmake --version | head -1)"
    HAS_CMAKE=true
else
    echo "[X] CMake not found!"
    echo "    macOS:  brew install cmake"
    echo "    Ubuntu: sudo apt install cmake"
    echo "    Fedora: sudo dnf install cmake"
    echo ""
fi

# Check C++ compiler
if command -v g++ &> /dev/null; then
    echo "[OK] $(g++ --version | head -1)"
    HAS_COMPILER=true
elif command -v clang++ &> /dev/null; then
    echo "[OK] $(clang++ --version | head -1)"
    HAS_COMPILER=true
else
    echo "[X] No C++ compiler found (g++ or clang++)!"
    echo "    macOS:  xcode-select --install"
    echo "    Ubuntu: sudo apt install build-essential"
    echo "    Fedora: sudo dnf install gcc-c++"
    echo ""
fi

# Find Qt6
QT6_PATH=""
SEARCH_DIRS=(
    "${QTDIR:-}"
    "${Qt6_DIR:-}"
    "$HOME/Qt"
    "/opt/Qt"
    "/usr/lib/cmake/Qt6"
    "/usr/local/lib/cmake/Qt6"
)

for dir in "${SEARCH_DIRS[@]}"; do
    if [ -n "$dir" ] && [ -d "$dir" ]; then
        FOUND=$(find "$dir" -name "Qt6Config.cmake" -type f 2>/dev/null | head -1)
        if [ -n "$FOUND" ]; then
            QT6_PATH=$(dirname "$FOUND")
            break
        fi
    fi
done

if [ -n "$QT6_PATH" ]; then
    echo "[OK] Qt6 found at: $QT6_PATH"
    HAS_QT6=true
else
    echo "[X] Qt6 not found!"
    echo "    Download from: https://www.qt.io/download-qt-installer"
    echo "    Or:  brew install qt@6   (macOS)"
    echo "    Or:  sudo apt install qt6-base-dev   (Ubuntu 22.04+)"
    echo ""
fi

echo ""

# ── Build ─────────────────────────────────────────────────────────────────────

if $HAS_CMAKE && $HAS_COMPILER && $HAS_QT6; then
    echo "All prerequisites met! Building..."
    echo ""

    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    BUILD_DIR="$SCRIPT_DIR/build"

    echo ">>> Configuring with CMake..."
    cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_PREFIX_PATH="$QT6_PATH"

    if [ $? -eq 0 ]; then
        NPROC=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
        echo ""
        echo ">>> Building with $NPROC threads..."
        cmake --build "$BUILD_DIR" --config Release -j "$NPROC"

        if [ $? -eq 0 ]; then
            echo ""
            echo "============================================"
            echo "  BUILD SUCCESSFUL!"
            if [[ "$(uname)" == "Darwin" ]]; then
                echo "  Run: open $BUILD_DIR/Seeing.app"
            else
                echo "  Run: $BUILD_DIR/Seeing"
            fi
            echo "============================================"
        else
            echo "Build failed!" >&2
            exit 1
        fi
    else
        echo "CMake configuration failed!" >&2
        exit 1
    fi
else
    echo "--------------------------------------------"
    echo "  Fix the issues above, then re-run:"
    echo "  ./setup.sh"
    echo "--------------------------------------------"
fi

echo ""
