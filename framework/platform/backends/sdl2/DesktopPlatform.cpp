// 桌面（macOS 等）：字体全部来自仓库 assets/font/。
//
// - 主字体 switch_font.ttf：从 HOS 共享字体转出，与 Switch 实机排版一致。
// - switch_icons.ttf：NintendoExt 转出的按键图标。
// - MaterialIcons-Regular.ttf：谷歌 Material 图标。
#include <cstdio>
#include <string>
#include <utility>

#include "platform/AssetPaths.h"
#include "platform/Fonts.h"
#include "platform/Platform.h"

namespace gui_dev {
namespace {

void AddFileFont(std::vector<FontSource>& out, const char* relative_path, FontRole role,
                 FontContent content, const char* what) {
    const std::string path = ResolveAssetPath(relative_path);
    if (path.empty()) {
        std::fprintf(stderr, "[gui_dev] 找不到 %s（%s）\n", relative_path, what);
        return;
    }
    FontSource source;
    source.path = path;
    source.size_pixels = 18.0f;
    source.role = role;
    source.content = content;
    out.push_back(std::move(source));
}

} // namespace

bool PlatformServicesInit() { return true; }

void PlatformServicesShutdown() {}

void CollectPlatformFontSources(std::vector<FontSource>& out) {
    AddFileFont(out, "font/switch_font.ttf", FontRole::Primary, FontContent::Text, "主文本字体");
    // content 决定这个源负责哪些码位：switch_icons 覆盖了 1022 个私用区码位
    // （大量空白字形），不声明清楚就会把 Material 图标整片遮蔽。
    AddFileFont(out, "font/switch_icons.ttf", FontRole::Merge, FontContent::ButtonIcons, "按键图标");
    AddFileFont(out, "font/MaterialIcons-Regular.ttf", FontRole::Merge, FontContent::MaterialIcons,
                "Material 图标");
}

} // namespace gui_dev
