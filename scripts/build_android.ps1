#!/usr/bin/env pwsh
[CmdletBinding()]
param(
    [string]$Ndk = $env:ANDROID_NDK_HOME,
    [switch]$NativeOnly,
    [switch]$Help
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

if ($Help) {
    @'
用法：pwsh -File scripts/build_android.ps1 [-Ndk <NDK目录>] [-NativeOnly]
依赖：Android SDK/NDK（NDK 27.0.12077973 推荐）、CMake、JDK 17、Gradle 8.7。
默认构建 arm64-v8a Release APK；-NativeOnly 只构建 build/android/libmain.so。
'@ | Write-Output
    exit 0
}

if (-not $Ndk -and $env:ANDROID_HOME) {
    $ndkRoot = Join-Path $env:ANDROID_HOME 'ndk'
    if (Test-Path $ndkRoot) {
        $candidate = Get-ChildItem $ndkRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
        if ($candidate) { $Ndk = $candidate.FullName }
    }
}
if (-not $Ndk -and $env:ANDROID_NDK_ROOT) { $Ndk = $env:ANDROID_NDK_ROOT }
if (-not $Ndk -or -not (Test-Path (Join-Path $Ndk 'build/cmake/android.toolchain.cmake'))) {
    throw 'Android NDK 未找到。传入 -Ndk <路径> 或设置 ANDROID_NDK_HOME。'
}
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw '找不到 cmake。' }
$gradle = Get-Command gradle -ErrorAction SilentlyContinue
if (-not $NativeOnly -and -not $gradle) { throw '找不到 gradle；请安装 Gradle 8.7 并加入 PATH，或加 -NativeOnly。' }

$env:ANDROID_NDK_HOME = (Resolve-Path $Ndk).Path
Push-Location $Root
try {
    cmake --preset android
    if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败，退出码 $LASTEXITCODE" }
    cmake --build --preset android --target main --parallel
    if ($LASTEXITCODE -ne 0) { throw "Android native 构建失败，退出码 $LASTEXITCODE" }
    if (-not $NativeOnly) {
        $sdlDir = Join-Path $Root 'build/android/_deps/sdl2-src'
        if (-not (Test-Path $sdlDir)) { throw "找不到 SDL2 源码目录：$sdlDir" }
        & $gradle.Source --project-dir (Join-Path $Root 'android') --no-daemon :app:assembleRelease "-Psdl2SourceDir=$sdlDir"
        if ($LASTEXITCODE -ne 0) { throw "APK 打包失败，退出码 $LASTEXITCODE" }
        Write-Host "完成：android/app/build/outputs/apk/release/app-release.apk"
    } else {
        Write-Host '完成：build/android/libmain.so'
    }
}
finally {
    Pop-Location
}
