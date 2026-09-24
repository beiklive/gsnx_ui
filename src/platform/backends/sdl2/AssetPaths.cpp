#include "platform/AssetPaths.h"

#include <cstdio>
#include <string>

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
#else
    // 桌面开发：可执行文件在 build/<preset>/ 下运行时，前三个都能命中。
    const char* prefixes[] = {
        "assets/",
        "../assets/",
        "../../assets/",
        GUI_DEV_ASSET_DIR "/",
    };
#endif

    for (const char* prefix : prefixes) {
        std::string path = std::string(prefix) + relative_path;
        if (FileExists(path)) {
            return path;
        }
    }
    return {};
}

} // namespace gui_dev
