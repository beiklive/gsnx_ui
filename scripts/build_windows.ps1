#!/usr/bin/env pwsh
[CmdletBinding()]
param(
    [string]$Config = 'Release',
    [string]$Target = 'gui_dev_demo',
    [switch]$Help
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if ($Help) {
    @'
用法：pwsh -File scripts/build_windows.ps1 [-Config Release|Debug] [-Target gui_dev_demo]
依赖：CMake、Visual Studio 2022 C++ workload；SDL2/libpng 由 preset 从源码获取。
产物：build/windows/<配置>/gui_dev_demo.exe
'@ | Write-Output
    exit 0
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw '找不到 cmake；请安装 CMake 并确保它在 PATH 中。'
}

Push-Location $Root
try {
    cmake --preset windows
    if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败，退出码 $LASTEXITCODE" }
    cmake --build --preset windows --config $Config --target $Target --parallel
    if ($LASTEXITCODE -ne 0) { throw "构建失败，退出码 $LASTEXITCODE" }
    Write-Host "完成：build/windows/$Config/$Target.exe"
}
finally {
    Pop-Location
}
