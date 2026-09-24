#include "ui/UiContext.h"

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#include "platform/Fonts.h"
#include "ui/Icons.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace gui_dev {
namespace {

// 文本与图标的基准字号（逻辑像素）。1.92 的光栅化器按需生成字形，
// 放大需求交给 style.FontScaleMain / PushFont，不用在这里改。
constexpr float kBaseFontSize = 18.0f;

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

    // 1.92 起字形按需光栅化（后端声明 RendererHasTextures），所以不需要手工 Build()
    // 图集，也不需要 glyph ranges —— 用到哪个字形就加载哪个。
    // 但字体源本身要重建：分辨率变化时会走到这里。
    io.Fonts->ClearFonts();

    std::vector<FontSource> sources;
    CollectPlatformFontSources(sources);

    // ---- 主字体：平台提供的源 > 系统 CJK 字体 > imgui 内置字体 ----
    bool primary_loaded = false;
    for (const FontSource& s : sources) {
        if (s.role != FontRole::Primary || primary_loaded) {
            continue;
        }
        if (s.data != nullptr && s.size > 0) {
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false; // 内存字体归平台所有（Switch 共享内存）
            primary_loaded = io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(s.data),
                                                           static_cast<int>(s.size),
                                                           s.size_pixels, &cfg) != nullptr;
        } else if (!s.path.empty()) {
            primary_loaded = io.Fonts->AddFontFromFileTTF(s.path.c_str(), s.size_pixels) != nullptr;
        }
    }
    if (!primary_loaded) {
        const char* system_font = FindSystemFont();
        if (system_font == nullptr || io.Fonts->AddFontFromFileTTF(system_font, kBaseFontSize) == nullptr) {
            // 兜底：内置字体（无 CJK 字形，仅用于没有字体的环境）。
            io.Fonts->AddFontDefault();
        }
    }

    // ---- 图标字体：合并进主字体，提供任天堂按键图标（私用区） ----
    for (const FontSource& s : sources) {
        if (s.role != FontRole::Icons) {
            continue;
        }
        ImFontConfig cfg;
        cfg.MergeMode = true;

        bool ok = false;
        if (s.data != nullptr && s.size > 0) {
            cfg.FontDataOwnedByAtlas = false;
            ok = io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(s.data), static_cast<int>(s.size),
                                                s.size_pixels, &cfg) != nullptr;
        } else if (!s.path.empty()) {
            ok = io.Fonts->AddFontFromFileTTF(s.path.c_str(), s.size_pixels, &cfg) != nullptr;
        }
        if (!ok) {
            std::fprintf(stderr, "[gui_dev] 图标字体加载失败：%s\n",
                         s.path.empty() ? "(内存字体)" : s.path.c_str());
        }
    }

    ReportIconCoverage();
}

void UiContext::ReportIconCoverage() const {
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts->Fonts.empty()) {
        std::fprintf(stderr, "[gui_dev] 字体图集为空，按键图标不可用\n");
        return;
    }

    // 直接问字体图集要字形：缺字形就是屏幕上会看到方块的那个情况。
    // Switch 上 HOS 字体版本与预期不符时，这里会一次性列出所有缺口。
    ImFont* primary = io.Fonts->Fonts[0];
    ImFontBaked* baked = primary->GetFontBaked(kBaseFontSize);
    if (baked == nullptr) {
        std::fprintf(stderr, "[gui_dev] 字体烘焙失败，按键图标自检跳过\n");
        return;
    }

    std::string missing;
    for (std::size_t i = 0; i < Icons::kButtonCount; ++i) {
        const Icons::Button button = static_cast<Icons::Button>(i);
        if (baked->FindGlyphNoFallback(static_cast<ImWchar>(Icons::Code(button))) == nullptr) {
            if (!missing.empty()) {
                missing += ", ";
            }
            missing += Icons::Label(button);
        }
    }

    if (missing.empty()) {
        std::fprintf(stderr, "[gui_dev] 按键图标 %zu/%zu 全部就绪\n", Icons::kButtonCount,
                     Icons::kButtonCount);
    } else {
        std::fprintf(stderr, "[gui_dev] 按键图标缺字形：%s（共 %zu 个）\n", missing.c_str(),
                     Icons::kButtonCount);
    }
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
