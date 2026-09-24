// Switch：HOS 共享字体。按键图标来自 PlSharedFontType_NintendoExt
// （普通字形不在这个字体里，它只含任天堂专有符号）。
//
// 生命周期：plInitialize 必须在取字体之前，返回的内存地址由 pl 的共享内存支撑，
// 因此在 PlatformFontsShutdown（plExit）之前一直有效 —— imgui 侧以
// FontDataOwnedByAtlas=false 引用，不复制、不释放。
#include <switch.h>

#include <cstdio>
#include <utility>

#include "platform/Fonts.h"

namespace gui_dev {
namespace {

bool g_pl_initialized = false;

} // namespace

bool PlatformFontsInit() {
    const Result rc = plInitialize(PlServiceType_User);
    if (R_FAILED(rc)) {
        std::fprintf(stderr, "[gui_dev] plInitialize failed: 0x%08X (按键图标不可用)\n",
                     static_cast<unsigned>(rc));
        g_pl_initialized = false;
        return false;
    }
    g_pl_initialized = true;
    return true;
}

void PlatformFontsShutdown() {
    if (g_pl_initialized) {
        plExit();
        g_pl_initialized = false;
    }
}

void CollectPlatformFontSources(std::vector<FontSource>& out) {
    if (!g_pl_initialized) {
        return;
    }

    PlFontData font{};
    const Result rc = plGetSharedFontByType(&font, PlSharedFontType_NintendoExt);
    if (R_FAILED(rc)) {
        std::fprintf(stderr, "[gui_dev] plGetSharedFontByType(NintendoExt) failed: 0x%08X\n",
                     static_cast<unsigned>(rc));
        return;
    }
    if (font.address == nullptr || font.size == 0) {
        std::fprintf(stderr, "[gui_dev] NintendoExt shared font is empty\n");
        return;
    }

    FontSource icons;
    icons.data = font.address;
    icons.size = font.size;
    icons.size_pixels = 18.0f;
    icons.role = FontRole::Icons;
    out.push_back(std::move(icons));
}

} // namespace gui_dev
