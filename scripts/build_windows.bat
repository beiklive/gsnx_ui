@echo off
setlocal

if /i "%~1"=="--help" goto help
if /i "%~1"=="-h" goto help

set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "CONFIG=Release"
set "TARGET=gui_dev_demo"

:parse_args
if "%~1"=="" goto build
if /i "%~1"=="--config" goto parse_config
if /i "%~1"=="--target" goto parse_target
echo Unknown argument: %~1 1>&2
goto help_error

:parse_config
if "%~2"=="" (
    echo --config requires Debug or Release. 1>&2
    goto help_error
)
set "CONFIG=%~2"
shift
shift
goto parse_args

:parse_target
if "%~2"=="" (
    echo --target requires a CMake target name. 1>&2
    goto help_error
)
set "TARGET=%~2"
shift
shift
goto parse_args

:build
where cmake >nul 2>nul || (echo CMake not found in PATH. 1>&2 & exit /b 1)
pushd "%ROOT%"
call cmake --preset windows
if errorlevel 1 goto failed
call cmake --build --preset windows --config "%CONFIG%" --target "%TARGET%" --parallel
if errorlevel 1 goto failed
echo Build complete: build\windows\%CONFIG%\%TARGET%.exe
popd
exit /b 0

:failed
set "RESULT=%ERRORLEVEL%"
popd
exit /b %RESULT%

:help
echo Usage: scripts\build_windows.bat [--config Release^|Debug] [--target gui_dev_demo]
echo Requires CMake and Visual Studio 2022 C++ workload. SDL2/libpng are fetched by CMake.
echo Output: build\windows\^<config^>\gui_dev_demo.exe
exit /b 0

:help_error
echo Usage: scripts\build_windows.bat [--config Release^|Debug] [--target gui_dev_demo]
exit /b 2
