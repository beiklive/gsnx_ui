#include "ui/UiContext.h"

#include <algorithm>
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
constexpr float kBaseFontSize = 22.0f;

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

// 某个内容类型提供的码位（图标码位只在 ui/Icons.h 里定义）。
std::vector<std::uint32_t> OwnedCodepoints(FontContent content) {
    std::vector<std::uint32_t> codes;
    switch (content) {
    case FontContent::ButtonIcons:
        codes.reserve(Icons::kButtonCount);
        for (std::size_t i = 0; i < Icons::kButtonCount; ++i) {
            codes.push_back(Icons::Code(static_cast<Icons::Button>(i)));
        }
        break;
    case FontContent::MaterialIcons:
        codes.reserve(Icons::kMaterialCount);
        for (std::size_t i = 0; i < Icons::kMaterialCount; ++i) {
            codes.push_back(Icons::Code(static_cast<Icons::Material>(i)));
        }
        break;
    case FontContent::Text:
        break;
    }
    std::sort(codes.begin(), codes.end());
    codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    return codes;
}

// 码位列表 -> imgui 要的 [first,last] 区间数组（0 结尾）。
std::vector<ImWchar> CompressRanges(const std::vector<std::uint32_t>& codes) {
    std::vector<ImWchar> ranges;
    for (std::uint32_t code : codes) {
        if (!ranges.empty() && ranges.size() % 2 == 0 &&
            code == static_cast<std::uint32_t>(ranges.back()) + 1u) {
            ranges.back() = static_cast<ImWchar>(code); // 与上一区间连续，向后扩
        } else {
            ranges.push_back(static_cast<ImWchar>(code));
            ranges.push_back(static_cast<ImWchar>(code));
        }
    }
    ranges.push_back(0);
    return ranges;
}

// 加载一个字体源。merge=true 时合并进已有字体（附加字形）。
bool AddFont(ImGuiIO& io, const FontSource& source, bool merge, const ImWchar* exclude_ranges) {
    ImFontConfig cfg;
    cfg.GlyphExcludeRanges = exclude_ranges;
    if (merge) {
        cfg.MergeMode = true;
    } else {
        // 内存字体归平台所有（Switch 共享内存），imgui 不许释放。
        cfg.FontDataOwnedByAtlas = false;
    }
    if (source.data != nullptr && source.size > 0) {
        return io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(source.data),
                                              static_cast<int>(source.size), source.size_pixels,
                                              &cfg) != nullptr;
    }
    if (!source.path.empty()) {
        return io.Fonts->AddFontFromFileTTF(source.path.c_str(), source.size_pixels, &cfg) != nullptr;
    }
    return false;
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
    // 图集；但字体源本身要重建（分辨率变化时走到这里）。
    io.Fonts->ClearFonts();

    std::vector<FontSource> sources;
    CollectPlatformFontSources(sources);

    // ---- 排除表：每个字体只提供自己那份码位 --------------------------------
    // 合并字体时同一码位由「第一个能提供它的源」胜出，而 NintendoExt 覆盖了
    // 1022 个私用区码位（大量空白字形），会把 Material 图标整片挡掉。
    // 因此让每个源排除「其它源拥有的、且不属于自己」的码位。
    // 注意：exclusion_ 的缓冲区必须活到字体销毁（imgui 只存指针）。
    exclusion_.clear();
    exclusion_.resize(sources.size());
    {
        std::vector<std::vector<std::uint32_t>> owned(sources.size());
        for (std::size_t i = 0; i < sources.size(); ++i) {
            owned[i] = OwnedCodepoints(sources[i].content);
        }
        for (std::size_t i = 0; i < sources.size(); ++i) {
            std::vector<std::uint32_t> blocked;
            for (std::size_t j = 0; j < sources.size(); ++j) {
                if (j == i) {
                    continue;
                }
                for (std::uint32_t code : owned[j]) {
                    if (std::find(owned[i].begin(), owned[i].end(), code) == owned[i].end()) {
                        blocked.push_back(code);
                    }
                }
            }
            std::sort(blocked.begin(), blocked.end());
            blocked.erase(std::unique(blocked.begin(), blocked.end()), blocked.end());
            exclusion_[i] = CompressRanges(blocked);
        }
    }

    // ---- 主字体：第一个加载成功的 Primary 源 ----
    bool primary_loaded = false;
    for (std::size_t i = 0; i < sources.size(); ++i) {
        if (sources[i].role != FontRole::Primary || primary_loaded) {
            continue;
        }
        primary_loaded = AddFont(io, sources[i], false, exclusion_[i].data());
    }
    if (!primary_loaded) {
        // 平台字体都没有时的兜底：系统 CJK 字体，最后才是 imgui 内置字体。
        const char* system_font = FindSystemFont();
        if (system_font == nullptr || io.Fonts->AddFontFromFileTTF(system_font, kBaseFontSize) == nullptr) {
            io.Fonts->AddFontDefault();
        }
    }

    // ---- 附加字形：按声明顺序合并（已有的码位不会被后来的源覆盖）-------------
    for (std::size_t i = 0; i < sources.size(); ++i) {
        const FontSource& s = sources[i];
        if (s.role != FontRole::Merge) {
            continue;
        }
        if (!AddFont(io, s, true, exclusion_[i].data())) {
            std::fprintf(stderr, "[gui_dev] 字体合并失败：%s\n",
                         s.path.empty() ? "(内存字体)" : s.path.c_str());
        }
    }

    ReportGlyphCoverage();
}

void UiContext::ReportGlyphCoverage() const {
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts->Fonts.empty()) {
        std::fprintf(stderr, "[gui_dev] 字体图集为空，图标不可用\n");
        return;
    }

    // 直接问字体图集要字形：缺字形就是屏幕上会看到方块的那个情况。
    ImFont* primary = io.Fonts->Fonts[0];
    ImFontBaked* baked = primary->GetFontBaked(kBaseFontSize);
    if (baked == nullptr) {
        std::fprintf(stderr, "[gui_dev] 字体烘焙失败，字形自检跳过\n");
        return;
    }

    std::string missing_buttons;
    for (std::size_t i = 0; i < Icons::kButtonCount; ++i) {
        const Icons::Button button = static_cast<Icons::Button>(i);
        if (baked->FindGlyphNoFallback(static_cast<ImWchar>(Icons::Code(button))) == nullptr) {
            if (!missing_buttons.empty()) {
                missing_buttons += ", ";
            }
            missing_buttons += Icons::Label(button);
        }
    }

    std::string missing_material;
    for (std::size_t i = 0; i < Icons::kMaterialCount; ++i) {
        const Icons::Material icon = static_cast<Icons::Material>(i);
        if (baked->FindGlyphNoFallback(static_cast<ImWchar>(Icons::Code(icon))) == nullptr) {
            if (!missing_material.empty()) {
                missing_material += ", ";
            }
            missing_material += Icons::Label(icon);
        }
    }

    std::fprintf(stderr, "[gui_dev] 字形自检：按键图标 %s（共 %zu），Material 图标 %s（共 %zu）\n",
                 missing_buttons.empty() ? "全部就绪" : missing_buttons.c_str(), Icons::kButtonCount,
                 missing_material.empty() ? "全部就绪" : missing_material.c_str(),
                 Icons::kMaterialCount);
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
