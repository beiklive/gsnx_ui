# iOS 工具链：CMake 3.14+ 原生支持 CMAKE_SYSTEM_NAME=iOS，这里补上常用默认值。
#
# 用法（preset 里已经接好）：
#   cmake --preset ios          # 真机（arm64, iphoneos）
#   cmake --preset ios-sim      # 模拟器（arm64 + x86_64, iphonesimulator）
#
# 依赖：SDL2 走源码编译（GUI_DEV_DEPS_MODE=fetch），链接 SDL2main 提供 UIApplicationMain 入口。
set(GUI_DEV_IOS_SIMULATOR OFF CACHE BOOL "iOS 模拟器构建（不签名）")
set(GUI_DEV_IOS_DEPLOYMENT_TARGET "13.0" CACHE STRING "最低 iOS 版本")

set(CMAKE_SYSTEM_NAME iOS)

# 先确认真的有 iOS SDK —— 只有 Command Line Tools 时 xcrun 找不到 iphoneos，
# 这里提前给出可执行的提示，而不是让 CMake 在后面报一堆编译器错误。
if(GUI_DEV_IOS_SIMULATOR)
    set(GUI_DEV_IOS_SDK_NAME "iphonesimulator")
else()
    set(GUI_DEV_IOS_SDK_NAME "iphoneos")
endif()
execute_process(
    COMMAND xcrun --sdk ${GUI_DEV_IOS_SDK_NAME} --show-sdk-path
    OUTPUT_VARIABLE GUI_DEV_IOS_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET RESULT_VARIABLE GUI_DEV_IOS_SDK_RESULT)
if(NOT GUI_DEV_IOS_SDK_RESULT EQUAL 0 OR NOT EXISTS "${GUI_DEV_IOS_SDK_PATH}")
    message(FATAL_ERROR
        "找不到 iOS SDK（${GUI_DEV_IOS_SDK_NAME}）。Command Line Tools 不带 iOS SDK，需要完整 Xcode：\n"
        "  1) 装 Xcode（App Store，或 brew install --cask xcodes && xcodes install --latest）\n"
        "  2) sudo xcode-select -s /Applications/Xcode.app/Contents/Developer\n"
        "  3) xcodebuild -runFirstLaunch && sudo xcodebuild -license accept\n"
        "如果 Xcode 装在别处，也可以不改 xcode-select，直接：\n"
        "  DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer cmake --preset ios\n"
        "（打包成 .ipa 用 scripts/build_ios_ipa.sh）")
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "${GUI_DEV_IOS_DEPLOYMENT_TARGET}" CACHE STRING "" FORCE)

if(GUI_DEV_IOS_SIMULATOR)
    set(CMAKE_OSX_SYSROOT "iphonesimulator" CACHE STRING "" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
else()
    set(CMAKE_OSX_SYSROOT "iphoneos" CACHE STRING "" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
endif()

# 默认不做代码签名（本机跑模拟器 / CI 冒烟用）；要装真机时在命令行覆盖成自己的 team。
if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
endif()
