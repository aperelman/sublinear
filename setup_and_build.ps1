# setup_and_build.ps1
# Run this script as Administrator to ensure tools can be installed.

$currentBranch = git rev-parse --abbrev-ref HEAD

Write-Host "Detected branch: $currentBranch" -ForegroundColor Cyan

# --- SECTION 1: CORE TOOLS ---
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    Write-Host "Installing Python..." -ForegroundColor Yellow
    winget install -e --id Python.Python.3.11 --accept-package-agreements
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "Installing CMake..." -ForegroundColor Yellow
    winget install -e --id Kitware.CMake --accept-package-agreements
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
}

# --- SECTION 2: PYTHON DEPENDENCIES ---
Write-Host "Configuring Python environment..." -ForegroundColor Cyan
python -m pip install --upgrade pip
if (Test-Path "requirements.txt") {
    python -m pip install -r requirements.txt
} else {
    python -m pip install numpy matplotlib aqtinstall
}

# --- SECTION 3: C++ / QT SPECIFIC (Only runs on qt branch) ---
if ($currentBranch -eq "qt") {
    Write-Host "Branch is 'qt'. Setting up MSVC Build Tools..." -ForegroundColor Cyan

    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        winget install --id Microsoft.VisualStudio.2022.BuildTools --override "--quiet --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended" --accept-package-agreements
    }

    $qtVersion = "6.5.0"
    $qtPath = "$env:USERPROFILE\Qt"
    if (-not (Test-Path "$qtPath\$qtVersion\msvc2019_64")) {
        Write-Host "Downloading Qt Framework..." -ForegroundColor Yellow
        python -m aqt install-qt windows desktop $qtVersion win64_msvc2019_64 --outputdir $qtPath
    }

    # Build process
    if (-not (Test-Path "build")) { New-Item -ItemType Directory "build" }
    Set-Location build
    cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="$qtPath\$qtVersion\msvc2019_64" ..
    cmake --build . --config Release

    Write-Host "Build complete. Executable is in build\Release\" -ForegroundColor Green
} else {
    Write-Host "Python environment is ready. Run your scripts using 'python <filename>.py'" -ForegroundColor Green
}
