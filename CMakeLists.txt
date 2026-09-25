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
    elseif(ANDROID)
        set(GUI_DEV_PLATFORM "android")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(GUI_DEV_PLATFORM "ios")
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
# 后端实现只在这里选：换后端不影响 framework/ui 与 framework/core。
if(GUI_DEV_BACKEND STREQUAL "sdl2")
    # 依赖怎么拿（系统包 / vcpkg / 源码编译）都在这里，出口是 gui_dev::SDL2 / gui_dev::PNG
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake)

    add_library(gui_dev_backend STATIC
        framework/platform/backends/sdl2/Sdl2Backend.cpp
        framework/platform/backends/sdl2/Factory.cpp
        framework/platform/backends/sdl2/AssetPaths.cpp
        framework/platform/backends/sdl2/PngLoader.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp)

    # 平台服务按平台各实现一份：
    #   Switch: pl 共享字体 + romfs；Android: APK assets；其余: assets/font/*.ttf
    if(GUI_DEV_PLATFORM STREQUAL "switch")
        target_sources(gui_dev_backend PRIVATE framework/platform/backends/sdl2/SwitchPlatform.cpp)
    elseif(GUI_DEV_PLATFORM STREQUAL "android")
        target_sources(gui_dev_backend PRIVATE framework/platform/backends/sdl2/AndroidPlatform.cpp)
    else()
        target_sources(gui_dev_backend PRIVATE framework/platform/backends/sdl2/DesktopPlatform.cpp)
    endif()
    target_compile_definitions(gui_dev_backend PRIVATE
        GUI_DEV_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

    target_include_directories(gui_dev_backend PUBLIC framework)
    target_link_libraries(gui_dev_backend PUBLIC imgui gui_dev::SDL2 gui_dev::PNG)
elseif(GUI_DEV_BACKEND STREQUAL "glfw")
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    add_library(gui_dev_backend STATIC
        framework/platform/backends/glfw/GlfwBackend.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(gui_dev_backend PUBLIC framework)
    target_link_libraries(gui_dev_backend PUBLIC imgui glfw OpenGL::GL)
else()
    message(FATAL_ERROR "unsupported GUI_DEV_BACKEND=${GUI_DEV_BACKEND}")
endif()

# --------------------------------------------------------------- gui_dev -----
# 统一前端组件库：framework/ui 只依赖 imgui，不包含任何平台头文件；
# 平台相关的一切都从 framework/platform/Backend.h 这层接口进来。
add_library(gui_dev STATIC
    framework/core/App.cpp
    framework/ui/UiContext.cpp
    framework/ui/Theme.cpp
    framework/ui/Icons.cpp
    framework/ui/Texture.cpp
    framework/ui/Scene.cpp
    framework/ui/Components.cpp
    # 暂停菜单 UI 层（与渲染后端、模拟核心解耦）
    framework/gamemenu/MenuAnimation.cpp
    framework/gamemenu/GameMenuTheme.cpp
    framework/gamemenu/GameMenuDraw.cpp
    framework/gamemenu/GameMenuElement.cpp
    framework/gamemenu/GameMenuView.cpp
    framework/gamemenu/views/MainMenuView.cpp
    framework/gamemenu/views/StateSlotView.cpp
    framework/gamemenu/views/SettingsView.cpp
    framework/gamemenu/views/DialogView.cpp)
target_include_directories(gui_dev PUBLIC framework ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gui_dev PUBLIC gui_dev_backend)

# ------------------------------------------------- component_view 组件库 -----
# 你自己写的组件与页面：只依赖 gui_dev（UiContext / TextureRef / Backend），
# 不依赖任何示例，mac 与 switch 共用同一份代码。
add_library(gui_dev_components STATIC)
# component_view/ 下新增 .cpp 不需要改这里：GLOB + CONFIGURE_DEPENDS 会自动纳入
file(GLOB_RECURSE GUI_DEV_COMPONENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/component_view/*.cpp")
target_sources(gui_dev_components PRIVATE ${GUI_DEV_COMPONENT_SOURCES})
target_include_directories(gui_dev_components PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gui_dev_components PUBLIC gui_dev)

# 信号槽语义测试（纯头文件，不依赖 SDL / ImGui）
add_executable(gui_dev_signal_test tests/qt_signal_test.cpp)
target_include_directories(gui_dev_signal_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
enable_testing()
add_test(NAME qt_signal_style COMMAND gui_dev_signal_test)

# 每个演示一个 target。移动端不方便一个工程出多个可执行文件，所以这里按平台换形态：
#   Android : 不建这些，只把 GUI_DEV_ANDROID_APP 编成 SHARED 库 "main"，交给 gradle 打包
#   iOS     : 编成 .app bundle（assets 拷进 Contents/Resources）
#   其他    : 普通可执行文件
function(gui_dev_add_demo target)
    if(GUI_DEV_PLATFORM STREQUAL "android")
        return()
    endif()
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE gui_dev_components)
    if(GUI_DEV_PLATFORM STREQUAL "ios")
        set_target_properties(${target} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
            MACOSX_BUNDLE_BUNDLE_NAME "${target}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.beiklive.gui_dev.${target}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/assets")
    endif()
endfunction()

# 组件演示：入口就是仓库根目录的 demo.cpp
gui_dev_add_demo(gui_dev_demo demo.cpp)

# 最小接入示例：证明 component_view 能整体搬进别的项目（不依赖 demo.cpp / 业务代码）
gui_dev_add_demo(gui_dev_min_demo examples/min_demo/main.cpp)

# 下面这些示例只对桌面/掌机有意义（Android 上只打包一个 app，iOS 上不必出 5 个 bundle）
if(NOT GUI_DEV_PLATFORM STREQUAL "android")

# 框架示例：不做暂停菜单之外的业务，只用来对照 framework/ui 的用法
gui_dev_add_demo(gui_dev_flow_demo
    examples/flow_box/main.cpp
    examples/flow_box/DemoApp.cpp)

# ImGui 能力导览：Tab 分页展示 ImGui 原生控件与接口（页面不滚动）
gui_dev_add_demo(gui_dev_imgui_tour
    examples/imgui_tour/main.cpp
    examples/imgui_tour/ImGuiTourApp.cpp
    examples/imgui_tour/TourTabsBasic.cpp
    examples/imgui_tour/TourTabsAdvanced.cpp
    examples/imgui_tour/TourTabsFontSystem.cpp)

# 暂停菜单 Demo：假游戏画面 + Persona 风格动态菜单（纯 UI 展示）
gui_dev_add_demo(gui_dev_pause_demo
    examples/pause_menu/main.cpp
    examples/pause_menu/PauseDemo.cpp)

# 自定义控件教学 Demo：8 个循序渐进的例子
gui_dev_add_demo(gui_dev_widget_demo
    examples/widget_lessons/main.cpp
    examples/widget_lessons/WidgetDemo.cpp)

endif() # 非 Android

# ------------------------------------------------------------ 移动端入口 ------
if(GUI_DEV_PLATFORM STREQUAL "android")
    # Android：入口必须编成 libmain.so（gradle 会加载 "main"），并且要带上 SDL 的
    # SDL_android_main.c（真正的 JNI 入口在里面）。
    set(GUI_DEV_ANDROID_APP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/demo.cpp"
        CACHE FILEPATH "Android 上要打包的演示入口")
    if(NOT GUI_DEV_SDL2_SOURCE_DIR)
        message(FATAL_ERROR
            "Android 需要 SDL2 源码（要编 SDL_android_main.c 进 libmain.so）："
            "用 -DGUI_DEV_DEPS_MODE=fetch，或手动 -DGUI_DEV_SDL2_SOURCE_DIR=<SDL2 源码目录>。")
    endif()
    add_library(main SHARED
        ${GUI_DEV_ANDROID_APP_SOURCE}
        ${GUI_DEV_SDL2_SOURCE_DIR}/src/main/android/SDL_android_main.c)
    target_link_libraries(main PRIVATE gui_dev_components)
    if(GUI_DEV_SDL2_MAIN)
        target_link_libraries(main PRIVATE ${GUI_DEV_SDL2_MAIN})
    endif()
    set_target_properties(main PROPERTIES OUTPUT_NAME "main")
    # 资源不往源码树里拷：android/app/build.gradle 直接把仓库的 assets/ 当 assets 源目录，
    # APK 里的 assets/font/*.ttf 由 AndroidPlatform.cpp 用 SDL_RWops 读。
endif()

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

    foreach(gui_dev_nro_target gui_dev_demo gui_dev_imgui_tour gui_dev_flow_demo gui_dev_pause_demo gui_dev_widget_demo)
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
