@echo off
setlocal EnableExtensions

set "NATIVE_ONLY=0"
set "NDK_ARG="

:parse_args
if "%~1"=="" goto setup
if /i "%~1"=="--help" goto help
if /i "%~1"=="-h" goto help
if /i "%~1"=="--native-only" goto native_only
if /i "%~1"=="-NativeOnly" goto native_only
if /i "%~1"=="--ndk" goto ndk_arg
echo Unknown argument: %~1 1>&2
goto help_error

:native_only
set "NATIVE_ONLY=1"
shift
goto parse_args

:ndk_arg
if "%~2"=="" (
    echo --ndk requires an NDK directory. 1>&2
    goto help_error
)
set "NDK_ARG=%~f2"
shift
shift
goto parse_args

:setup
set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
if not defined ANDROID_NDK_HOME if defined ANDROID_NDK_ROOT set "ANDROID_NDK_HOME=%ANDROID_NDK_ROOT%"
if not defined NDK_ARG if defined ANDROID_NDK_HOME set "NDK_ARG=%ANDROID_NDK_HOME%"
if not defined NDK_ARG if defined ANDROID_HOME if exist "%ANDROID_HOME%\ndk\" (
    for /f "delims=" %%D in ('dir /b /ad "%ANDROID_HOME%\ndk" 2^>nul ^| sort /r') do if not defined NDK_ARG set "NDK_ARG=%ANDROID_HOME%\ndk\%%D"
)
if not defined NDK_ARG (
    echo Android NDK not found. Set ANDROID_NDK_HOME or pass --ndk ^<path^>. 1>&2
    exit /b 1
)
set "ANDROID_NDK_HOME=%NDK_ARG%"
if not exist "%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake" (
    echo Invalid NDK path: "%ANDROID_NDK_HOME%" 1>&2
    exit /b 1
)
where cmake >nul 2>nul || (echo CMake not found in PATH. 1>&2 & exit /b 1)
if "%NATIVE_ONLY%"=="1" goto tools_ready
where gradle >nul 2>nul || (echo Gradle not found in PATH. Install Gradle 8.7 or use --native-only. 1>&2 & exit /b 1)
:tools_ready

pushd "%ROOT%"
call cmake --preset android
if errorlevel 1 goto failed
call cmake --build --preset android --target main --parallel
if errorlevel 1 goto failed
if "%NATIVE_ONLY%"=="1" goto native_done
set "SDL_DIR=%ROOT%\build\android\_deps\sdl2-src"
if not exist "%SDL_DIR%\src\main\android\SDL_android_main.c" (
    echo SDL2 source checkout not found under "%SDL_DIR%". 1>&2
    goto failed
)
call gradle --project-dir "%ROOT%\android" --no-daemon :app:assembleRelease "-Psdl2SourceDir=%SDL_DIR%"
if errorlevel 1 goto failed
echo APK complete: android\app\build\outputs\apk\release\app-release.apk
goto success

:native_done
echo Native build complete: build\android\libmain.so

:success
popd
exit /b 0

:failed
set "RESULT=%ERRORLEVEL%"
if "%RESULT%"=="0" set "RESULT=1"
popd
exit /b %RESULT%

:help
echo Usage: scripts\build_android.bat [--ndk ^<NDK directory^>] [--native-only]
echo Requires Android SDK/NDK (NDK 27.0.12077973 recommended), CMake, JDK 17 and Gradle 8.7.
echo Default builds the arm64-v8a Release APK; --native-only builds libmain.so only.
exit /b 0

:help_error
echo Usage: scripts\build_android.bat [--ndk ^<NDK directory^>] [--native-only]
exit /b 2
