// 桌面（macOS 等）：按键图标来自仓库内的 assets/font/switch_icons.ttf，
// 与 Switch 的 HOS NintendoExt 使用同一套私用区码位，因此 UI 代码无需分支。
#include <cstdio>
#include <string>
#include <utility>

#include "platform/AssetPaths.h"
#include "platform/Fonts.h"

namespace gui_dev {

bool PlatformFontsInit() { return true; }

void PlatformFontsShutdown() {}

void CollectPlatformFontSources(std::vector<FontSource>& out) {
    const std::string icons_path = ResolveAssetPath("font/switch_icons.ttf");
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
