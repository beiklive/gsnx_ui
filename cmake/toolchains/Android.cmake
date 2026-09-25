# Android 工具链包装：真正的工具链在 NDK 里，这里只做「设好默认 ABI/API 再 include」。
#
# 用法（preset 里已经接好）：
#   export ANDROID_NDK_HOME=~/Library/Android/sdk/ndk/27.0.12077973
#   cmake --preset android
#
# 依赖：SDL2 必须是源码（要编 SDL_android_main.c 进 libmain.so），
#       所以 android preset 默认 GUI_DEV_DEPS_MODE=fetch。
set(GUI_DEV_ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI: arm64-v8a | armeabi-v7a | x86_64")
set(GUI_DEV_ANDROID_API "26" CACHE STRING "Android API level")

if(NOT DEFINED ANDROID_NDK_HOME OR ANDROID_NDK_HOME STREQUAL "")
    if(DEFINED ENV{ANDROID_NDK_HOME})
        set(ANDROID_NDK_HOME "$ENV{ANDROID_NDK_HOME}")
    elseif(DEFINED ENV{ANDROID_NDK_ROOT})
        set(ANDROID_NDK_HOME "$ENV{ANDROID_NDK_ROOT}")
    endif()
endif()

if(NOT ANDROID_NDK_HOME OR NOT EXISTS "${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake")
    message(FATAL_ERROR
        "找不到 Android NDK。装一个（Android Studio 的 SDK Manager，或 https://developer.android.com/ndk/downloads），"
        "然后设 ANDROID_NDK_HOME 指向它，例如：\n"
        "  export ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk/27.0.12077973")
endif()

set(ANDROID_ABI "${GUI_DEV_ANDROID_ABI}" CACHE STRING "" FORCE)
set(ANDROID_PLATFORM "android-${GUI_DEV_ANDROID_API}" CACHE STRING "" FORCE)
set(ANDROID_STL "c++_static" CACHE STRING "" FORCE)

include("${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake")
