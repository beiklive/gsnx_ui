cmake_minimum_required(VERSION 3.20)

project(gui_dev
    VERSION 0.1.0
    DESCRIPTION "Unified ImGui frontend components for the GBAStation emulator family"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------- platform ----
# 不靠 CMAKE_SYSTEM_NAME 猜平台：Switch 由 cmake/toolchains/Switch.cmake 显式设置
# GUI_DEV_PLATFORM，桌面构建按宿主系统兜底。
if(NOT GUI_DEV_PLATFORM)
    if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
        set(GUI_DEV_PLATFORM "switch")
    elseif(APPLE)
        set(GUI_DEV_PLATFORM "mac")
    elseif(WIN32)
        set(GUI_DEV_PLATFORM "windows")
    elseif(UNIX)
        set(GUI_DEV_PLATFORM "linux")
    else()
        set(GUI_DEV_PLATFORM "unknown")
    endif()
endif()

# 可选后端：sdl2（默认，mac/switch 共用）| glfw（仅桌面）
set(GUI_DEV_BACKEND "sdl2" CACHE STRING "window/render backend: sdl2 | glfw")
set_property(CACHE GUI_DEV_BACKEND PROPERTY STRINGS sdl2 glfw)

add_compile_definitions(
    GUI_DEV_PLATFORM_${GUI_DEV_PLATFORM}
    GUI_DEV_VERSION="${PROJECT_VERSION}")

message(STATUS "gui_dev: platform=${GUI_DEV_PLATFORM} backend=${GUI_DEV_BACKEND}")

# ------------------------------------------------------------------- imgui ----
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
if(NOT EXISTS ${IMGUI_DIR}/imgui.cpp)
    message(FATAL_ERROR
        "third_party/imgui is empty. Run: git submodule update --init --recursive")
endif()

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_DIR}/misc/cpp)

# imgui 默认 shell 处理用 fork/execvp/waitpid，Switch(libnx/newlib) 没有这些符号，
# 且嵌入式环境不需要 shell out。
if(GUI_DEV_PLATFORM STREQUAL "switch")
    target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS)
endif()

# --------------------------------------------------------------- backends ----
# 后端实现只在这里选：换后端不影响 src/ui 与 src/core。
if(GUI_DEV_BACKEND STREQUAL "sdl2")
    find_package(PkgConfig REQUIRED)
    # 宿主上 devkitPro 的 pkg-config 常排在 PATH 前面（它只认 switch portlibs），
    # mac 侧必须显式指到 homebrew 的 pkg-config，否则找不到 sdl2.pc。
    if(GUI_DEV_PLATFORM STREQUAL "mac" AND NOT DEFINED PKG_CONFIG_EXECUTABLE)
        find_program(GUI_DEV_HOST_PKG_CONFIG NAMES pkg-config
            HINTS /opt/homebrew/bin /usr/local/bin)
        if(GUI_DEV_HOST_PKG_CONFIG)
            set(PKG_CONFIG_EXECUTABLE ${GUI_DEV_HOST_PKG_CONFIG} CACHE FILEPATH "" FORCE)
        endif()
    endif()
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    # 图片解码用 libpng：mac(homebrew) 与 Switch(portlibs) 都自带，
    # 因此不需要 SDL_image（mac 上没有）也不需要 vendored stb_image。
    pkg_check_modules(LIBPNG REQUIRED IMPORTED_TARGET libpng)

    add_library(gui_dev_backend STATIC
        src/platform/backends/sdl2/Sdl2Backend.cpp
        src/platform/backends/sdl2/Factory.cpp
        src/platform/backends/sdl2/AssetPaths.cpp
        src/platform/backends/sdl2/PngLoader.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp)

    # 字体来源按平台各实现一份（Switch: pl 共享字体 + romfs；桌面: assets/font/*.ttf）
    if(GUI_DEV_PLATFORM STREQUAL "switch")
        target_sources(gui_dev_backend PRIVATE src/platform/backends/sdl2/SwitchPlatform.cpp)
    else()
        target_sources(gui_dev_backend PRIVATE src/platform/backends/sdl2/DesktopPlatform.cpp)
    endif()
    target_compile_definitions(gui_dev_backend PRIVATE
        GUI_DEV_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

    target_include_directories(gui_dev_backend PUBLIC src)
    target_link_libraries(gui_dev_backend PUBLIC imgui PkgConfig::SDL2 PkgConfig::LIBPNG)
elseif(GUI_DEV_BACKEND STREQUAL "glfw")
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    add_library(gui_dev_backend STATIC
        src/platform/backends/glfw/GlfwBackend.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(gui_dev_backend PUBLIC src)
    target_link_libraries(gui_dev_backend PUBLIC imgui glfw OpenGL::GL)
else()
    message(FATAL_ERROR "unsupported GUI_DEV_BACKEND=${GUI_DEV_BACKEND}")
endif()

# --------------------------------------------------------------- gui_dev -----
# 统一前端组件库：src/ui 只依赖 imgui，不包含任何平台头文件；
# 平台相关的一切都从 src/platform/Backend.h 这层接口进来。
add_library(gui_dev STATIC
    src/core/App.cpp
    src/ui/UiContext.cpp
    src/ui/Theme.cpp
    src/ui/Icons.cpp
    src/ui/Texture.cpp
    src/ui/Scene.cpp
    src/ui/Components.cpp
    # 暂停菜单 UI 层（与渲染后端、模拟核心解耦）
    src/gamemenu/MenuAnimation.cpp
    src/gamemenu/GameMenuTheme.cpp
    src/gamemenu/GameMenuDraw.cpp
    src/gamemenu/GameMenuElement.cpp
    src/gamemenu/GameMenuView.cpp
    src/gamemenu/views/MainMenuView.cpp
    src/gamemenu/views/StateSlotView.cpp
    src/gamemenu/views/SettingsView.cpp
    src/gamemenu/views/DialogView.cpp)
target_include_directories(gui_dev PUBLIC src)
target_link_libraries(gui_dev PUBLIC gui_dev_backend)

add_executable(gui_dev_demo
    src/main.cpp
    src/demo/DemoApp.cpp)
target_link_libraries(gui_dev_demo PRIVATE gui_dev)

# 暂停菜单 Demo：假游戏画面 + Persona 风格动态菜单（纯 UI 展示）
add_executable(gui_dev_pause_demo
    src/demo/pause/main.cpp
    src/demo/pause/PauseDemo.cpp)
target_link_libraries(gui_dev_pause_demo PRIVATE gui_dev)

# 自定义控件教学 Demo：8 个循序渐进的例子
add_executable(gui_dev_widget_demo
    src/demo/widgets/main.cpp
    src/demo/widgets/WidgetDemo.cpp)
target_link_libraries(gui_dev_widget_demo PRIVATE gui_dev)

if(GUI_DEV_PLATFORM STREQUAL "switch")
    # NRO 打包：nx_create_nro 会生成 NACP 并调用 elf2nro。
    # 图标可选：把 256x256 png 放到 assets/icon.png 即自动打包。
    set(NACP_NAME "GUI_DEV")
    set(NACP_AUTHOR "GBAStation")
    set(NACP_VERSION "${PROJECT_VERSION}")

    # romfs 只放 Switch 端真正需要的运行时资源：
    #   img/  界面图片
    #   font/MaterialIcons-Regular.ttf  谷歌图标字体（必须随 NRO 发布）
    # 不放 font/switch_font.ttf(10.9MB) 与 switch_icons.ttf：Switch 走 pl 共享字体。
    set(GUI_DEV_ROMFS_DIR "${CMAKE_BINARY_DIR}/romfs")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/img" DESTINATION "${GUI_DEV_ROMFS_DIR}")
    file(MAKE_DIRECTORY "${GUI_DEV_ROMFS_DIR}/font")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/font/MaterialIcons-Regular.ttf"
         DESTINATION "${GUI_DEV_ROMFS_DIR}/font")

    foreach(gui_dev_nro_target gui_dev_demo gui_dev_pause_demo gui_dev_widget_demo)
        set(GUI_DEV_NRO_ARGS OUTPUT "${CMAKE_BINARY_DIR}/dist/${gui_dev_nro_target}.nro"
            ROMFS "${GUI_DEV_ROMFS_DIR}")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png")
            list(APPEND GUI_DEV_NRO_ARGS ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png")
        endif()
        nx_create_nro(${gui_dev_nro_target} ${GUI_DEV_NRO_ARGS})
    endforeach()

    # 图片额外拷一份到 NRO 旁边，方便用 sdmc:/switch/GUI_DEV/assets/ 覆盖 romfs 里的版本。
    add_custom_command(TARGET gui_dev_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/img"
            "${CMAKE_BINARY_DIR}/dist/assets/img")
endif()
