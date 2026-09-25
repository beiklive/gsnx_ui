#include "platform/AssetPaths.h"

#include <cstdio>
#include <string>
#include <vector>

#include <SDL.h>

#ifndef GUI_DEV_ASSET_DIR
#define GUI_DEV_ASSET_DIR "."
#endif

namespace gui_dev {
namespace {

bool FileExists(const std::string& path) {
    if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
        std::fclose(f);
        return true;
    }
    return false;
}

} // namespace

// 按平台找资源（字体、图片）。返回空字符串表示没找到，调用方负责降级。
std::string ResolveAssetPath(const char* relative_path) {
    if (relative_path == nullptr || relative_path[0] == '\0') {
        return {};
    }

#if defined(GUI_DEV_PLATFORM_switch)
    // Switch：NRO 没有打包 romfs（assets 里有 10MB+ 字体，不适合塞进 NRO），
    // 因此优先找 sdmc 上的 resources 目录，其次才是 romfs。
    const char* prefixes[] = {
        "sdmc:/switch/GUI_DEV/assets/",
        "romfs:/assets/",
        "romfs:/",
    };
    for (const char* prefix : prefixes) {
        std::string path = std::string(prefix) + relative_path;
        if (FileExists(path)) {
            return path;
        }
    }
    return {};
#elif defined(GUI_DEV_PLATFORM_android)
    // Android：资源都打包在 APK 里，没有文件系统路径可给（libpng 的 fopen 也读不到）。
    // 字体走 AndroidPlatform.cpp 里的 SDL_RWops 读进内存；纹理暂时不支持，见
    // docs/platform-builds.md 的「已知限制」。
    (void)relative_path;
    return {};
#else
    // 桌面 / 移动端：先看仓库里的 assets（开发时最方便），再看可执行文件旁边。
    std::vector<std::string> prefixes = {
        "assets/",
        "../assets/",
        "../../assets/",
        GUI_DEV_ASSET_DIR "/",
    };
    // SDL_GetBasePath()：Windows 是 exe 所在目录（发布时把 assets 拷过去即可），
    // iOS 是 .app 里的 bundle 目录（CMake 会把 assets 拷进 Contents/Resources）。
    if (char* base = SDL_GetBasePath()) {
        prefixes.push_back(std::string(base) + "assets/");
        prefixes.push_back(std::string(base) + "Resources/assets/");
        SDL_free(base);
    }
    for (const std::string& prefix : prefixes) {
        std::string path = prefix + relative_path;
        if (FileExists(path)) {
            return path;
        }
    }
    return {};
#endif
}

} // namespace gui_dev
