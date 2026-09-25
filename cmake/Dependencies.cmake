# 依赖解析：SDL2 + libpng（+ zlib）。
#
# 目标：同一份 CMakeLists 在 mac / Switch / Windows / Android / iOS 上都能配起来，
# 所以这里把「怎么拿到依赖」抽出来，出口统一成三个 imported target：
#     gui_dev::SDL2   gui_dev::PNG
#
# 两种模式（-DGUI_DEV_DEPS_MODE=...）：
#   package（默认）：优先 pkg-config（mac 的 homebrew / Switch 的 devkitPro 都自带 .pc），
#                    没有 pkg-config 就 find_package(CONFIG)（Windows 的 vcpkg / 手工装的 SDL2）。
#   fetch          ：FetchContent 直接拉源码编译（Windows / Android / iOS 上最省事，
#                    不需要先装 vcpkg 或系统包；需要网络，首次配置会慢一点）。
#
# 另外对 Android 暴露 GUI_DEV_SDL2_SOURCE_DIR：SDL2 的 Android 入口（SDL_android_main.c）
# 要从源码树里编进 libmain.so，所以那个平台必须能拿到 SDL2 源码（fetch 模式即可）。

include_guard(GLOBAL)

set(GUI_DEV_DEPS_MODE "package" CACHE STRING "dependencies: package | fetch")
set_property(CACHE GUI_DEV_DEPS_MODE PROPERTY STRINGS package fetch)

if(NOT GUI_DEV_DEPS_MODE STREQUAL "package" AND NOT GUI_DEV_DEPS_MODE STREQUAL "fetch")
    message(FATAL_ERROR "GUI_DEV_DEPS_MODE 只能是 package 或 fetch（当前：${GUI_DEV_DEPS_MODE}）")
endif()

# 把任意依赖包成 gui_dev::Xxx。
# 不能直接 add_library(gui_dev::SDL2 ALIAS SDL2::SDL2)：SDL2 自己导出的 SDL2::SDL2
# 本身就是个 ALIAS，而 CMake 不允许 alias-to-alias；所以先建普通 INTERFACE 再 alias。
function(gui_dev_export_target impl_name exported_name source_target)
    add_library(${impl_name} INTERFACE)
    target_link_libraries(${impl_name} INTERFACE ${source_target})
    add_library(${exported_name} ALIAS ${impl_name})
endfunction()

# ------------------------------------------------------------------ package ---
if(GUI_DEV_DEPS_MODE STREQUAL "package")

    find_package(PkgConfig QUIET)

    # ---- SDL2 ----
    if(TARGET PkgConfig::GUI_DEV_SDL2_PC)
        gui_dev_export_target(gui_dev_sdl2_impl gui_dev::SDL2 PkgConfig::GUI_DEV_SDL2_PC)
    elseif(PKG_CONFIG_FOUND)
        pkg_check_modules(GUI_DEV_SDL2_PC QUIET IMPORTED_TARGET sdl2)
    endif()

    if(NOT TARGET gui_dev::SDL2 AND TARGET PkgConfig::GUI_DEV_SDL2_PC)
        gui_dev_export_target(gui_dev_sdl2_impl gui_dev::SDL2 PkgConfig::GUI_DEV_SDL2_PC)
    endif()

    if(NOT TARGET gui_dev::SDL2)
        find_package(SDL2 CONFIG QUIET)
        if(TARGET SDL2::SDL2)
            gui_dev_export_target(gui_dev_sdl2_impl gui_dev::SDL2 SDL2::SDL2)
        elseif(TARGET SDL2::SDL2-static)
            gui_dev_export_target(gui_dev_sdl2_impl gui_dev::SDL2 SDL2::SDL2-static)
        else()
            message(FATAL_ERROR
                "找不到 SDL2。四种办法选一个：\n"
                "  * macOS:   brew install sdl2\n"
                "  * Linux:   装 libsdl2-dev（或发行版对应包）\n"
                "  * Windows: vcpkg install sdl2:x64-windows 并加 -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake\n"
                "  * 任意平台: -DGUI_DEV_DEPS_MODE=fetch （直接从源码编译 SDL2/libpng）")
        endif()
    endif()
    set(GUI_DEV_SDL2_SOURCE_DIR "" CACHE INTERNAL "")

    # ---- libpng（纹理解码）----
    if(TARGET PkgConfig::GUI_DEV_PNG_PC)
        gui_dev_export_target(gui_dev_png_impl gui_dev::PNG PkgConfig::GUI_DEV_PNG_PC)
    elseif(PKG_CONFIG_FOUND)
        pkg_check_modules(GUI_DEV_PNG_PC QUIET IMPORTED_TARGET libpng libpng16)
    endif()

    if(NOT TARGET gui_dev::PNG AND TARGET PkgConfig::GUI_DEV_PNG_PC)
        gui_dev_export_target(gui_dev_png_impl gui_dev::PNG PkgConfig::GUI_DEV_PNG_PC)
    endif()

    if(NOT TARGET gui_dev::PNG)
        find_package(PNG QUIET)
        if(TARGET PNG::PNG)
            gui_dev_export_target(gui_dev_png_impl gui_dev::PNG PNG::PNG)
        else()
            message(FATAL_ERROR
                "找不到 libpng（纹理解码用）。装一个，或 -DGUI_DEV_DEPS_MODE=fetch 让 CMake 自己编。")
        endif()
    endif()

    message(STATUS "gui_dev: 依赖模式 package（系统包 / vcpkg）")

# -------------------------------------------------------------------- fetch ---
else()
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    # CMake 4 不再兼容 cmake_minimum_required(<3.5) 的老项目；SDL2 2.32.x 已经改了，
    # 这里留个兜底，免得换成老 tag 直接配置失败。
    if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    endif()

    # Android 上 SDL2 必须是动态库（gradle 把 libSDL2.so 打进 APK）；
    # 桌面 / iOS 用静态库省事，不用管运行时路径。
    if(GUI_DEV_PLATFORM STREQUAL "android")
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    else()
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
    endif()
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

    # ---- zlib：优先用 SDK 自带的（macOS / iOS SDK / Android NDK 都有 libz），没有再拉源码 ----
    find_package(ZLIB QUIET)
    if(NOT ZLIB_FOUND)
        FetchContent_Declare(zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG v1.3.1
            GIT_SHALLOW TRUE)
        set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(zlib)
    endif()

    # ---- libpng ----
    if(NOT TARGET gui_dev::PNG)
        set(PNG_SHARED OFF CACHE BOOL "" FORCE)
        set(PNG_STATIC ON CACHE BOOL "" FORCE)
        set(PNG_TESTS OFF CACHE BOOL "" FORCE)
        set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
        set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE)
        # 不安装也不导出：否则 install(EXPORT PNGTargets) 会抱怨 zlib 不在导出集里
        set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
        set(SKIP_INSTALL_EXPORT ON CACHE BOOL "" FORCE)
        FetchContent_Declare(libpng
            GIT_REPOSITORY https://github.com/pnggroup/libpng.git
            GIT_TAG v1.6.58
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(libpng)
        if(TARGET png_static)
            gui_dev_export_target(gui_dev_png_impl gui_dev::PNG png_static)
        elseif(TARGET PNG::PNG)
            gui_dev_export_target(gui_dev_png_impl gui_dev::PNG PNG::PNG)
        elseif(TARGET png)
            gui_dev_export_target(gui_dev_png_impl gui_dev::PNG png)
        else()
            message(FATAL_ERROR "libpng 拉下来了但没找到可链接的 target")
        endif()
    endif()

    # ---- SDL2 ----
    if(NOT TARGET gui_dev::SDL2)
        FetchContent_Declare(SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.32.10
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(SDL2)
        # Android 入口要用源码树里的 SDL_android_main.c
        set(GUI_DEV_SDL2_SOURCE_DIR "${sdl2_SOURCE_DIR}" CACHE INTERNAL "")
        if(TARGET SDL2::SDL2)
            gui_dev_export_target(gui_dev_sdl2_impl gui_dev::SDL2 SDL2::SDL2)
        elseif(TARGET SDL2-static)
            gui_dev_export_target(gui_dev_sdl2_impl gui_dev::SDL2 SDL2-static)
        elseif(TARGET SDL2)
            gui_dev_export_target(gui_dev_sdl2_impl gui_dev::SDL2 SDL2)
        else()
            message(FATAL_ERROR "SDL2 拉下来了但没找到可链接的 target")
        endif()
    endif()

    message(STATUS "gui_dev: 依赖模式 fetch（SDL2 / libpng 从源码编译）")
endif()

# iOS 需要 SDL2main 提供 UIApplicationMain 入口；有就链，没有就算了。
if(TARGET SDL2::SDL2main)
    set(GUI_DEV_SDL2_MAIN SDL2::SDL2main)
elseif(TARGET SDL2main)
    set(GUI_DEV_SDL2_MAIN SDL2main)
else()
    set(GUI_DEV_SDL2_MAIN "")
endif()
