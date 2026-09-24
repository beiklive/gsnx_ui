// Switch：HOS 服务与打包资源。
//
// - pl:u 共享字体：Standard（日/美/欧）作主字体，ChineseSimplified 补充中文，
//   NintendoExt 提供任天堂按键图标。共享内存由 pl 持有，imgui 侧以
//   FontDataOwnedByAtlas=false 引用，不复制、不释放。
// - romfs：MaterialIcons-Regular.ttf 与 assets/img/*.png 打包进 NRO。
//   assets/font/switch_font.ttf(10.9MB) 与 switch_icons.ttf 在 Switch 上用不到
//   （走 pl 共享字体），因此不进 romfs，否则 NRO 会被撑大。
#include <switch.h>

#include <cstdio>
#include <string>
#include <utility>

#include "platform/AssetPaths.h"
#include "platform/Fonts.h"
#include "platform/Platform.h"

namespace gui_dev {
namespace {

bool g_pl_initialized = false;
bool g_romfs_initialized = false;

void AddSharedFont(std::vector<FontSource>& out, PlSharedFontType type, FontRole role,
                   FontContent content, const char* what) {
    PlFontData font{};
    const Result rc = plGetSharedFontByType(&font, type);
    if (R_FAILED(rc)) {
        std::fprintf(stderr, "[gui_dev] plGetSharedFontByType(%s) 失败: 0x%08X\n", what,
                     static_cast<unsigned>(rc));
        return;
    }
    if (font.address == nullptr || font.size == 0) {
        std::fprintf(stderr, "[gui_dev] 共享字体 %s 为空\n", what);
        return;
    }
    FontSource source;
    source.data = font.address;
    source.size = font.size;
    source.size_pixels = 18.0f;
    source.role = role;
    source.content = content;
    out.push_back(std::move(source));
}

} // namespace

bool PlatformServicesInit() {
    g_romfs_initialized = R_SUCCEEDED(romfsInit());
    if (!g_romfs_initialized) {
        std::fprintf(stderr, "[gui_dev] romfsInit 失败，打包资源（Material 图标/图片）不可用\n");
    }

    const Result rc = plInitialize(PlServiceType_User);
    if (R_FAILED(rc)) {
        std::fprintf(stderr, "[gui_dev] plInitialize 失败: 0x%08X（共享字体不可用）\n",
                     static_cast<unsigned>(rc));
        g_pl_initialized = false;
        return false;
    }
    g_pl_initialized = true;
    return true;
}

void PlatformServicesShutdown() {
    if (g_pl_initialized) {
        plExit();
        g_pl_initialized = false;
    }
    if (g_romfs_initialized) {
        romfsExit();
        g_romfs_initialized = false;
    }
}

void CollectPlatformFontSources(std::vector<FontSource>& out) {
    if (g_pl_initialized) {
        AddSharedFont(out, PlSharedFontType_Standard, FontRole::Primary, FontContent::Text,
                      "Standard");
        AddSharedFont(out, PlSharedFontType_ChineseSimplified, FontRole::Merge, FontContent::Text,
                      "ChineseSimplified");
        // content 决定这个源负责哪些码位：NintendoExt 覆盖了整整 1022 个私用区码位,
        // 不声明清楚就会把 Material 图标整片遮蔽。
        AddSharedFont(out, PlSharedFontType_NintendoExt, FontRole::Merge, FontContent::ButtonIcons,
                      "NintendoExt");
    } else {
        std::fprintf(stderr, "[gui_dev] pl 未初始化，主字体将退回 imgui 内置字体\n");
    }

    const std::string material = ResolveAssetPath("font/MaterialIcons-Regular.ttf");
    if (material.empty()) {
        std::fprintf(stderr, "[gui_dev] 找不到打包的 MaterialIcons-Regular.ttf\n");
    } else {
        FontSource source;
        source.path = material;
        source.size_pixels = 18.0f;
        source.role = FontRole::Merge;
        source.content = FontContent::MaterialIcons;
        out.push_back(std::move(source));
    }
}

} // namespace gui_dev
