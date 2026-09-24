// 桌面（macOS 等）：按键图标来自仓库内的 assets/font/switch_icons.ttf，
// 与 Switch 的 HOS NintendoExt 使用同一套私用区码位，因此 UI 代码无需分支。
#include <cstdio>
#include <string>
#include <utility>

#include "platform/Fonts.h"

#ifndef GUI_DEV_ASSET_DIR
#define GUI_DEV_ASSET_DIR "."
#endif

namespace gui_dev {
namespace {

// 依次尝试：相对当前工作目录、相对可执行文件上级、以及 CMake 写死的源码目录。
// 开发期（build/mac/gui_dev_demo）三种情况都能命中。
std::string FindAssetFont(const char* file_name) {
    const char* prefixes[] = {
        "assets/font/",
        "../assets/font/",
        "../../assets/font/",
        GUI_DEV_ASSET_DIR "/font/",
    };
    for (const char* prefix : prefixes) {
        std::string path = std::string(prefix) + file_name;
        if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
            std::fclose(f);
            return path;
        }
    }
    return {};
}

} // namespace

bool PlatformFontsInit() { return true; }

void PlatformFontsShutdown() {}

void CollectPlatformFontSources(std::vector<FontSource>& out) {
    const std::string icons_path = FindAssetFont("switch_icons.ttf");
    if (icons_path.empty()) {
        std::fprintf(stderr,
                     "[gui_dev] 找不到 assets/font/switch_icons.ttf，按键图标将无法显示\n");
        return;
    }

    FontSource icons;
    icons.path = icons_path;
    icons.size_pixels = 18.0f;
    icons.role = FontRole::Icons;
    out.push_back(std::move(icons));
}

} // namespace gui_dev
