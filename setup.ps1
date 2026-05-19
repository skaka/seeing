###############################################################################
# Seeing NLE — Environment Setup Script (Windows)
# Run this AFTER installing Qt6 and CMake.
# Usage: .\setup.ps1 [-QtPath "C:\Qt\6.7.0\msvc2019_64"]
###############################################################################

param(
    [string]$QtPath = ""
)

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Seeing NLE — Build Setup (Windows)" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# ── Check Prerequisites ──────────────────────────────────────────────────────

$allGood = $true

# Check CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "[X] CMake not found!" -ForegroundColor Red
    Write-Host "    Download from: https://cmake.org/download/" -ForegroundColor Yellow
    Write-Host "    Or: winget install Kitware.CMake" -ForegroundColor Yellow
    Write-Host ""
    $allGood = $false
} else {
    $cmakeVer = & cmake --version | Select-Object -First 1
    Write-Host "[OK] $cmakeVer" -ForegroundColor Green
}

# Check C++ compiler (MSVC or MinGW g++)
$cl = Get-Command cl -ErrorAction SilentlyContinue
$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $cl -and -not $gpp) {
    Write-Host "[X] No C++ compiler found (cl.exe or g++)!" -ForegroundColor Red
    Write-Host "    Install Visual Studio 2022 with 'Desktop development with C++'" -ForegroundColor Yellow
    Write-Host "    Or: winget install Microsoft.VisualStudio.2022.BuildTools" -ForegroundColor Yellow
    Write-Host ""
    $allGood = $false
} else {
    if ($cl) { Write-Host "[OK] MSVC compiler found" -ForegroundColor Green }
    if ($gpp) { Write-Host "[OK] G++ compiler found" -ForegroundColor Green }
}

# Find Qt6
$qtSearchPaths = @(
    $QtPath,
    "C:\Qt",
    "$env:USERPROFILE\Qt",
    "C:\Program Files\Qt",
    "C:\Program Files (x86)\Qt"
) | Where-Object { $_ -ne "" -and (Test-Path $_ -ErrorAction SilentlyContinue) }

$qt6Found = $false
$qt6Path = ""

foreach ($basePath in $qtSearchPaths) {
    $qt6Dirs = Get-ChildItem -Path $basePath -Recurse -Filter "Qt6Config.cmake" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($qt6Dirs) {
        $qt6Path = Split-Path $qt6Dirs.FullName -Parent
        $qt6Found = $true
        break
    }
}

if ($qt6Found) {
    Write-Host "[OK] Qt6 found at: $qt6Path" -ForegroundColor Green
} else {
    Write-Host "[X] Qt6 not found!" -ForegroundColor Red
    Write-Host "    Download from: https://www.qt.io/download-qt-installer" -ForegroundColor Yellow
    Write-Host "    Install Qt 6.x with the 'Desktop' kit" -ForegroundColor Yellow
    Write-Host ""
    $allGood = $false
}

Write-Host ""

# ── Build ─────────────────────────────────────────────────────────────────────

if ($allGood) {
    Write-Host "All prerequisites met! Building..." -ForegroundColor Green
    Write-Host ""

    $projectRoot = $PSScriptRoot
    $buildDir = Join-Path $projectRoot "build"

    # Configure
    Write-Host ">>> Configuring with CMake..." -ForegroundColor Cyan
    cmake -B $buildDir -S $projectRoot -DCMAKE_PREFIX_PATH="$qt6Path"

    if ($LASTEXITCODE -eq 0) {
        # Build
        $nproc = (Get-CimInstance Win32_Processor).NumberOfLogicalProcessors
        Write-Host ""
        Write-Host ">>> Building with $nproc threads..." -ForegroundColor Cyan
        cmake --build $buildDir --config Release -j $nproc

        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "============================================" -ForegroundColor Green
            Write-Host "  BUILD SUCCESSFUL!" -ForegroundColor Green
            Write-Host "  Run: .\build\Release\Seeing.exe" -ForegroundColor Green
            Write-Host "============================================" -ForegroundColor Green
        } else {
            Write-Host "Build failed!" -ForegroundColor Red
        }
    } else {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
    }
} else {
    Write-Host "--------------------------------------------" -ForegroundColor Yellow
    Write-Host "  Fix the issues above, then re-run:" -ForegroundColor Yellow
    Write-Host "  .\setup.ps1" -ForegroundColor Yellow
    Write-Host "--------------------------------------------" -ForegroundColor Yellow
}

Write-Host ""
