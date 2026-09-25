// Android：字体从 APK 的 assets/ 里读进内存。
//
// Android 上 GUI_DEV_ASSET_DIR 那种「源码目录路径」在设备上不存在，资源都打包在 APK 里，
// 只能通过 SDL_RWFromFile（SDL 会去 APK 的 assets/ 找）拿。FontSource 本来就支持
// 「内存里的字体数据」（Switch 的共享字体也是走 data），所以这里把文件读进堆里、
// 让 buffer 活到进程结束，再把 data/size 填进 FontSource。
//
// 注意：纹理（LoadTexture）目前仍走文件路径，Android 上读不到 APK 里的 png —— 见
// docs/platform-builds.md 的「已知限制」。
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <SDL.h>

#include "platform/Fonts.h"
#include "platform/Platform.h"

namespace gui_dev {
namespace {

// 读进来的字体数据；用 deque 语义保证 data() 指针稳定（vector 扩容会搬家）
std::vector<std::unique_ptr<std::vector<unsigned char>>> g_font_buffers;

bool ReadAsset(const char* relative_path, std::vector<unsigned char>& out) {
    SDL_RWops* rw = SDL_RWFromFile(relative_path, "rb");
    if (rw == nullptr) {
        return false;
    }
    const Sint64 size = SDL_RWsize(rw);
    if (size <= 0) {
        SDL_RWclose(rw);
        return false;
    }
    out.resize(static_cast<std::size_t>(size));
    const std::size_t read = SDL_RWread(rw, out.data(), 1, out.size());
    SDL_RWclose(rw);
    if (read != out.size()) {
        out.clear();
        return false;
    }
    return true;
}

void AddAssetFont(std::vector<FontSource>& out, const char* relative_path, FontRole role,
                  FontContent content, const char* what) {
    auto buffer = std::make_unique<std::vector<unsigned char>>();
    if (!ReadAsset(relative_path, *buffer)) {
        std::fprintf(stderr, "[gui_dev] 找不到 %s（APK assets/%s）\n", what, relative_path);
        return;
    }
    FontSource source;
    source.data = buffer->data();
    source.size = buffer->size();
    source.size_pixels = 18.0f;
    source.role = role;
    source.content = content;
    out.push_back(std::move(source));
    g_font_buffers.push_back(std::move(buffer)); // 留住内存，data 指针才有效
    std::fprintf(stderr, "[gui_dev] 字体已加载 assets/%s（%zu 字节）\n", relative_path, g_font_buffers.back()->size());
}

} // namespace

bool PlatformServicesInit() { return true; }

void PlatformServicesShutdown() {
    // 字体图集销毁后才会走到这里，可以安全放手
    g_font_buffers.clear();
}

void CollectPlatformFontSources(std::vector<FontSource>& out) {
    AddAssetFont(out, "font/switch_font.ttf", FontRole::Primary, FontContent::Text, "主文本字体");
    AddAssetFont(out, "font/switch_icons.ttf", FontRole::Merge, FontContent::ButtonIcons, "按键图标");
    AddAssetFont(out, "font/MaterialIcons-Regular.ttf", FontRole::Merge, FontContent::MaterialIcons,
                 "Material 图标");
}

} // namespace gui_dev
