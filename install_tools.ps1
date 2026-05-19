# install_tools.ps1
# Download and install portable CMake, MinGW (w64devkit), and Qt6 locally.
# REQUIRES NO ADMIN PRIVILEGES.

$ErrorActionPreference = "Stop"

$toolsDir = Join-Path $PSScriptRoot "tools"
if (-not (Test-Path $toolsDir)) {
    New-Item -ItemType Directory -Path $toolsDir | Out-Null
}

Write-Host "===========================================" -ForegroundColor Cyan
Write-Host "  Installing Portable Development Tools...  " -ForegroundColor Cyan
Write-Host "===========================================" -ForegroundColor Cyan
Write-Host "Destination: $toolsDir" -ForegroundColor Yellow
Write-Host ""

# ── 1. Download & Extract CMake ──────────────────────────────────────────────
$cmakeDir = Join-Path $toolsDir "cmake"
if (-not (Test-Path $cmakeDir)) {
    Write-Host "[1/3] Downloading Portable CMake..." -ForegroundColor Cyan
    $cmakeUrl = "https://github.com/Kitware/CMake/releases/download/v3.29.3/cmake-3.29.3-windows-x86_64.zip"
    $cmakeZip = Join-Path $toolsDir "cmake.zip"
    
    Invoke-WebRequest -Uri $cmakeUrl -OutFile $cmakeZip -UseBasicParsing
    Write-Host "Extracting CMake..." -ForegroundColor Yellow
    Expand-Archive -Path $cmakeZip -DestinationPath $toolsDir
    
    # Rename to 'cmake'
    $extractedFolder = Get-ChildItem -Path $toolsDir -Directory -Filter "cmake-3.29.3-*" | Select-Object -First 1
    Rename-Item -Path $extractedFolder.FullName -NewName "cmake"
    Remove-Item $cmakeZip
    Write-Host "[OK] CMake installed successfully." -ForegroundColor Green
} else {
    Write-Host "[OK] CMake already installed." -ForegroundColor Green
}

# ── 2. Download & Extract w64devkit (GCC / MinGW) ────────────────────────────
$mingwDir = Join-Path $toolsDir "w64devkit"
if (-not (Test-Path $mingwDir)) {
    Write-Host "[2/3] Downloading w64devkit (GCC/MinGW + Make)..." -ForegroundColor Cyan
    $mingwUrl = "https://github.com/skeeto/w64devkit/releases/download/v2.0.0/w64devkit-2.0.0.zip"
    $mingwZip = Join-Path $toolsDir "mingw.zip"
    
    Invoke-WebRequest -Uri $mingwUrl -OutFile $mingwZip -UseBasicParsing
    Write-Host "Extracting MinGW..." -ForegroundColor Yellow
    Expand-Archive -Path $mingwZip -DestinationPath $toolsDir
    Remove-Item $mingwZip
    Write-Host "[OK] MinGW installed successfully." -ForegroundColor Green
} else {
    Write-Host "[OK] MinGW already installed." -ForegroundColor Green
}

# ── 3. Install Qt6 via aqtinstall ────────────────────────────────────────────
$qtDir = Join-Path $toolsDir "Qt"
if (-not (Test-Path $qtDir)) {
    Write-Host "[3/3] Installing Qt 6.8.3 via aqtinstall..." -ForegroundColor Cyan
    
    # We run aqt as a python module
    # Installing desktop Qt 6.8.3 for windows, architecture win64_mingw
    # Output to tools\Qt
    python -m aqt install-qt windows desktop 6.8.3 win64_mingw --output $qtDir
    
    Write-Host "[OK] Qt 6.8.3 installed successfully." -ForegroundColor Green
} else {
    Write-Host "[OK] Qt 6.8.3 already installed." -ForegroundColor Green
}

Write-Host ""
Write-Host "===========================================" -ForegroundColor Green
Write-Host "  All tools installed locally!             " -ForegroundColor Green
Write-Host "===========================================" -ForegroundColor Green
Write-Host ""
