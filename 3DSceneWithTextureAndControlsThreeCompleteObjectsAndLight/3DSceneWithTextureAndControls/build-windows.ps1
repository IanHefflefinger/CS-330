# Builds (and optionally runs) the 3D scene on Windows.
# Usage (PowerShell): powershell -ExecutionPolicy Bypass -File .\build-windows.ps1 [--run]
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

function Check($what) {
    if ($LASTEXITCODE -ne 0) { throw "$what failed (exit code $LASTEXITCODE)" }
}

foreach ($tool in "git", "cmake") {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "'$tool' was not found. Install it and re-open PowerShell. Also install Visual Studio (or Build Tools) with the 'Desktop development with C++' workload."
    }
}

# vcpkg provides GLFW, GLEW and GLM
$vcpkgRoot = Join-Path $env:USERPROFILE "vcpkg"
if (-not (Test-Path $vcpkgRoot)) {
    git clone https://github.com/microsoft/vcpkg $vcpkgRoot; Check "git clone vcpkg"
}
$vcpkg = Join-Path $vcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkg)) {
    & (Join-Path $vcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics; Check "vcpkg bootstrap"
}
& $vcpkg install glfw3 glew glm --triplet x64-windows; Check "vcpkg install"

if (-not (Test-Path "..\resources\textures")) {
    Write-Warning "..\resources\textures not found. The scene needs its texture PNGs there."
}

$toolchain = Join-Path $vcpkgRoot "scripts\buildsystems\vcpkg.cmake"
cmake -S . -B build "-DCMAKE_TOOLCHAIN_FILE=$toolchain" -DVCPKG_TARGET_TRIPLET=x64-windows; Check "cmake configure"
cmake --build build --config Release; Check "cmake build"

Write-Host "Build complete. Run it from this folder with: .\scene.exe"
if ($args -contains "--run") { & .\scene.exe }
