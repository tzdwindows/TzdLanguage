# ==============================================================================
# TzdTools Installer One-Click Build Script
# ==============================================================================
param(
    [string]$Version = "0.2.4"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistDir = Join-Path $RepoRoot "dist"
$StagingDir = Join-Path $DistDir "TzdTools"
$CpuStagingDir = Join-Path $DistDir "TzdTools_CPU"
$InstallerDir = Join-Path $RepoRoot "installer"
$ReleaseDir = Join-Path $RepoRoot "x64\Release"
$SevenZip = "C:\Program Files\7-Zip\7z.exe"
$MakeNSIS = "C:\Program Files (x86)\NSIS\makensis.exe"
$CudaBin = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin"
$MsvcRedist = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.OpenMP"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host " Building TzdTools Installer & Distribution Package v$Version" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan

# 1. Verify Prerequisites
if (-not (Test-Path $SevenZip)) {
    throw "7-Zip not found at $SevenZip"
}
if (-not (Test-Path $MakeNSIS)) {
    throw "NSIS compiler not found at $MakeNSIS"
}
if (-not (Test-Path (Join-Path $ReleaseDir "TzdTools.exe"))) {
    throw "TzdTools.exe not found in $ReleaseDir. Please build the Release configuration first."
}

# 2. Prepare Clean Staging Directory
Write-Host "[1/5] Preparing staging directory..." -ForegroundColor Yellow
if (Test-Path $StagingDir) {
    Remove-Item -Recurse -Force $StagingDir
}
New-Item -ItemType Directory -Path $StagingDir -Force | Out-Null

# Copy binary & runtime header
Copy-Item (Join-Path $ReleaseDir "TzdTools.exe") $StagingDir -Force
Copy-Item (Join-Path $RepoRoot "TzdNativeRuntime.hpp") $StagingDir -Force

# Copy stdlib
$StdlibDir = Join-Path $RepoRoot "stdlib"
if (Test-Path $StdlibDir) {
    Copy-Item $StdlibDir (Join-Path $StagingDir "stdlib") -Recurse -Force
}

# Copy all runtime DLLs from Release directory
Get-ChildItem -Path $ReleaseDir -Filter "*.dll" | ForEach-Object {
    Copy-Item $_.FullName $StagingDir -Force
}

# Ensure nvrtc-builtins64_126.dll is copied
$NvrtcBuiltins = Join-Path $CudaBin "nvrtc-builtins64_126.dll"
if (Test-Path $NvrtcBuiltins) {
    Copy-Item $NvrtcBuiltins (Join-Path $StagingDir "nvrtc-builtins64_126.dll") -Force
    Write-Host "  -> Included nvrtc-builtins64_126.dll" -ForegroundColor Green
}

# Ensure vcomp140.dll (OpenMP) is copied
$Vcomp = Join-Path $MsvcRedist "vcomp140.dll"
if (Test-Path $Vcomp) {
    Copy-Item $Vcomp (Join-Path $StagingDir "vcomp140.dll") -Force
    Write-Host "  -> Included vcomp140.dll" -ForegroundColor Green
}

# Create tzd CLI launcher
$TzdCmd = Join-Path $StagingDir "tzd.cmd"
"@echo off`r`nrem TzdLang CLI Launcher`r`n`"%~dp0TzdTools.exe`" %*`r`n" | Set-Content -Path $TzdCmd -Encoding ASCII

# Copy setup and uninstall helper scripts
Copy-Item (Join-Path $InstallerDir "setup_env.cmd") $StagingDir -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $InstallerDir "uninstall.cmd") $StagingDir -Force -ErrorAction SilentlyContinue

# 3. Compress Payload with 7-Zip LZMA2
Write-Host "[2/5] Compressing GPU payload with 7-Zip (LZMA2)..." -ForegroundColor Yellow
$PayloadFile = Join-Path $InstallerDir "payload.7z"
if (Test-Path $PayloadFile) {
    Remove-Item -Force $PayloadFile
}
& $SevenZip a -mx=5 $PayloadFile "$StagingDir\*" | Out-Null
$PayloadSize = (Get-Item $PayloadFile).Length / 1MB
Write-Host "  -> GPU Payload compressed: $([Math]::Round($PayloadSize, 1)) MB" -ForegroundColor Green

# 4. Compile GPU NSIS Setup Executable
Write-Host "[3/5] Compiling NSIS Windows Installer (GPU Edition)..." -ForegroundColor Yellow
$NsiScript = Join-Path $InstallerDir "TzdTools_Installer.nsi"
& $MakeNSIS $NsiScript
$SetupExe = Join-Path $DistDir "TzdTools_Setup_v$Version.exe"
if (Test-Path $SetupExe) {
    $SetupSize = (Get-Item $SetupExe).Length / 1MB
    Write-Host "  -> GPU Installer generated: $SetupExe ($([Math]::Round($SetupSize, 1)) MB)" -ForegroundColor Green
} else {
    throw "NSIS compilation failed to produce $SetupExe"
}

# 5. Build CPU Edition Installer if CPU staging directory exists
if (Test-Path $CpuStagingDir) {
    Write-Host "[4/5] Preparing and compressing CPU Edition payload..." -ForegroundColor Yellow
    Copy-Item (Join-Path $ReleaseDir "TzdTools.exe") $CpuStagingDir -Force
    Copy-Item (Join-Path $RepoRoot "TzdNativeRuntime.hpp") $CpuStagingDir -Force
    if (Test-Path $StdlibDir) {
        Copy-Item $StdlibDir (Join-Path $CpuStagingDir "stdlib") -Recurse -Force
    }

    $PayloadCpu = Join-Path $InstallerDir "payload_cpu.7z"
    if (Test-Path $PayloadCpu) {
        Remove-Item -Force $PayloadCpu
    }
    & $SevenZip a -mx=5 $PayloadCpu "$CpuStagingDir\*" | Out-Null
    $CpuPayloadSize = (Get-Item $PayloadCpu).Length / 1MB
    Write-Host "  -> CPU Payload compressed: $([Math]::Round($CpuPayloadSize, 1)) MB" -ForegroundColor Green

    Write-Host "[5/5] Compiling NSIS Windows Installer (CPU Edition)..." -ForegroundColor Yellow
    $NsiCpuScript = Join-Path $InstallerDir "TzdTools_Installer_CPU.nsi"
    if (Test-Path $NsiCpuScript) {
        & $MakeNSIS $NsiCpuScript
        $SetupCpuExe = Join-Path $DistDir "TzdTools_Setup_v${Version}_CPU.exe"
        if (Test-Path $SetupCpuExe) {
            $SetupCpuSize = (Get-Item $SetupCpuExe).Length / 1MB
            Write-Host "  -> CPU Installer generated: $SetupCpuExe ($([Math]::Round($SetupCpuSize, 1)) MB)" -ForegroundColor Green
        }
    }
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host " Build Complete! Artifacts available in ${DistDir}:" -ForegroundColor Cyan
Write-Host "   $SetupExe" -ForegroundColor Green
if (Test-Path (Join-Path $DistDir "TzdTools_Setup_v${Version}_CPU.exe")) {
    Write-Host "   $(Join-Path $DistDir "TzdTools_Setup_v${Version}_CPU.exe")" -ForegroundColor Green
}
Write-Host "============================================================" -ForegroundColor Cyan
