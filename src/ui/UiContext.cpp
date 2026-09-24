#include "ui/UiContext.h"

#include <cstdarg>
#include <cstdio>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace gui_dev {
namespace {

// 统一字体策略：优先找一个带 CJK 字形的系统字体，找不到就退回 ImGui 内置字体。
// 纯 ASCII 界面不依赖任何外部文件，保证任何平台都能起来。
const char* FindSystemFont() {
#if defined(__APPLE__)
    static const char* kCandidates[] = {
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Medium.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
    };
#elif defined(__SWITCH__)
    static const char* kCandidates[] = {
        "sdmc:/switch/GUI_DEV/fonts/ui.ttf",
        "romfs:/fonts/ui.ttf",
    };
#else
    static const char* kCandidates[] = {
        "C:/Windows/Fonts/msyh.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    };
#endif
    for (const char* path : kCandidates) {
        if (std::FILE* f = std::fopen(path, "rb")) {
            std::fclose(f);
            return path;
        }
    }
    return nullptr;
}

} // namespace

UiContext::UiContext(Backend& backend) : backend_(backend) {}

UiContext::~UiContext() = default;

void UiContext::BeginFrame() {
    input_ = InputFrame{};
    backend_.PollEvents(input_);
    backend_.NewImGuiFrame();
}

void UiContext::EndFrame() {
    ImGui::Render();
    backend_.BeginRenderFrame();
    backend_.EndRenderFrame();
}

bool UiContext::RefreshIfDisplayChanged() {
    const std::uint32_t gen = backend_.DisplayGeneration();
    if (!fonts_built_ || gen != last_display_generation_) {
        RebuildFonts();
        last_display_generation_ = gen;
        fonts_built_ = true;
        return true;
    }
    return false;
}

void UiContext::RebuildFonts() {
    ImGuiIO& io = ImGui::GetIO();

    // 1.92 起字体按需光栅化（RendererHasTextures），不再需要手工 Build() 图集，
    // 也不需要指定 glyph ranges —— 用到哪个字形就加载哪个，所以这里只选字体源。
    const float base_size = 18.0f;

    if (const char* path = FindSystemFont()) {
        if (io.Fonts->AddFontFromFileTTF(path, base_size)) {
            return;
        }
    }

    // 兜底：内置字体（中文会缺字形，仅用于无字体环境调试）。
    io.Fonts->AddFontDefault();
}

void UiContext::Text(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}

void UiContext::TextDisabled(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::TextDisabledV(fmt, args);
    va_end(args);
}

} // namespace gui_dev
