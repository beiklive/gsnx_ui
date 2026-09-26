#include "component_view/components/MarkdownView.h"

#include <utility>

#include "component_view/Global.h"
#include "third_party/imgui_markdown/imgui_markdown.h"

namespace gui_dev::cv {
namespace {

// 传给 imgui_markdown 的上下文：控件自己 + 当前可用宽度（图片按它限宽）
struct MarkdownContext {
    MarkdownView* view = nullptr;
    float width = 0.0f;
};

float HeadingSize(int level, float base, float scale) {
    float size = base;
    if (level <= 1) {
        size = Theme::kFontTitle + 4.0f;
    } else if (level == 2) {
        size = Theme::kFontHeader;
    } else {
        size = Theme::kFontBody;
    }
    return Maxf(size * scale, 12.0f);
}

// 格式回调：颜色/字号全部来自 Theme，不在这里写死色值
void MarkdownFormatCallback(const ImGui::MarkdownFormatInfo& info, bool start) {
    const MarkdownContext* context = static_cast<const MarkdownContext*>(info.config->userData);
    const float scale = context != nullptr && context->view != nullptr ? context->view->text_scale : 1.0f;
    const MarkdownView* view = context != nullptr ? context->view : nullptr;

    switch (info.type) {
    case ImGui::MarkdownFormatType::HEADING:
        if (start) {
            ImGui::NewLine();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::U32(Theme::kTextBright));
            ImGui::PushFont(ImGui::GetFont(), HeadingSize(info.level, Theme::kFontBody, scale));
        } else {
            ImGui::PopFont();
            ImGui::PopStyleColor();
            const bool underline = view == nullptr || view->heading_separator;
            if (underline) {
                ImGui::Separator();
            }
        }
        break;
    case ImGui::MarkdownFormatType::LINK:
        if (start) {
            const ImVec4 color = view != nullptr ? view->link_color : Theme::kBlue;
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::U32(color));
        } else {
            ImGui::PopStyleColor();
        }
        break;
    case ImGui::MarkdownFormatType::EMPHASIS:
        if (start) {
            // level 1 = 斜体，level 2 = 粗体。当前字体只有一个字重：
            // 斜体按语义保留（可用 emphasis_color 单独染色），粗体用高亮色区分。
            const float size = info.level == 1 ? Theme::kFontBody * scale
                                               : Theme::kFontBody * scale * 1.02f;
            ImGui::PushFont(ImGui::GetFont(), size);
            if (info.level != 1) {
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::U32(Theme::kTextBright));
            } else if (view != nullptr && view->emphasis_color.w > 0.0f) {
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::U32(view->emphasis_color));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
            }
        } else {
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }
        break;
    case ImGui::MarkdownFormatType::UNORDERED_LIST:
    case ImGui::MarkdownFormatType::NORMAL_TEXT:
    default:
        break;
    }
}

// 图片回调：交给宿主的 resolver 找纹理；找不到就退化成链接文本
ImGui::MarkdownImageData MarkdownImageCallback(ImGui::MarkdownLinkCallbackData data) {
    ImGui::MarkdownImageData image;
    const MarkdownContext* context = static_cast<const MarkdownContext*>(data.userData);
    if (context == nullptr || context->view == nullptr || !context->view->image_resolver) {
        return image;
    }
    const std::string path(data.link, static_cast<std::size_t>(data.linkLength));
    ImTextureRef texture{};
    float width = 0.0f;
    float height = 0.0f;
    if (!context->view->image_resolver(path, texture, width, height) ||
        texture.GetTexID() == ImTextureID_Invalid || width <= 0.0f || height <= 0.0f) {
        return image;
    }
    float scale = 1.0f;
    if (context->width > 1.0f) {
        scale = Minf(scale, context->width / width); // 不超出可用宽度
    }
    const float max_height = context->view->max_image_height;
    if (max_height > 0.0f) {
        scale = Minf(scale, max_height / height);
    }
    image.isValid = true;
    image.user_texture_id = texture.GetTexID();
    image.size = ImVec2(width * scale, height * scale);
    return image;
}

void MarkdownLinkCallback(ImGui::MarkdownLinkCallbackData) {
    // 主机上没有浏览器：链接只做视觉呈现（蓝色 + 下划线），不触发打开动作
}

} // namespace

MarkdownView::MarkdownView() : Widget("markdown_view") {
    capture_vertical = true; // 上下键用来滚自己所在的滚动容器
}

MarkdownView& MarkdownView::setText(std::string value) {
    text = std::move(value);
    rendered_height_ = 1.0f; // 内容变了：下一帧重新量
    return *this;
}

MarkdownView& MarkdownView::setImageResolver(ImageResolver resolver) {
    image_resolver = std::move(resolver);
    return *this;
}

Widget* MarkdownView::ScrollHostOrNull() const {
    Widget* host = const_cast<MarkdownView*>(this)->ScrollHost();
    return (host == const_cast<MarkdownView*>(this)) ? nullptr : host;
}

FocusVisual MarkdownView::BuildFocusVisual() const {
    FocusVisual visual = Widget::BuildFocusVisual();
    if (!visual.enabled) {
        return visual;
    }
    if (Widget* host = const_cast<MarkdownView*>(this)->ScrollHostOrNull()) {
        if (host->overflow == Overflow::Scroll) {
            const float scale = Maxf(DrawScale(), 0.001f);
            const float offset = Maxf(focus_frame_offset, 2.0f) * scale;
            visual.rect = host->DrawRect().Expanded(offset * 0.5f);
            visual.radius = Global::component_style.corner_radius * scale + offset;
        }
    }
    return visual;
}

ImVec2 MarkdownView::MeasureContent(const ImVec2& available) {
    // 高度用上一帧实际渲染出来的高度（imgui_markdown 是即时模式，先渲染才知道多高）。
    // 放在滚动容器里时，Popup 每帧会把这个高度显式写进 size.y（否则会被夹到视口高度）。
    return ImVec2(Maxf(available.x, 1.0f), Maxf(rendered_height_, 1.0f));
}

void MarkdownView::OnDrawContent(ImDrawList* dl, const Rect& content) {
    (void)dl;
    if (text.empty()) {
        rendered_height_ = 1.0f;
        return;
    }

    // 可见区：默认是自己的矩形；在滚动容器里就夹到容器可见区（逐帧裁剪）
    Rect viewport = content;
    if (Widget* host = ScrollHostOrNull()) {
        if (host->overflow == Overflow::Scroll) {
            const Rect host_rect = host->DrawRect();
            viewport.min.y = Maxf(viewport.min.y, host_rect.min.y);
            viewport.max.y = Minf(viewport.max.y, host_rect.max.y);
            viewport.min.x = Maxf(viewport.min.x, host_rect.min.x);
            viewport.max.x = Minf(viewport.max.x, host_rect.max.x);
        }
    }
    if (viewport.Height() <= 1.0f || viewport.Width() <= 1.0f) {
        return;
    }

    MarkdownContext context;
    context.view = this;
    context.width = content.Width();

    ImGui::MarkdownConfig config;
    config.linkCallback = MarkdownLinkCallback;
    config.tooltipCallback = nullptr;
    config.imageCallback = MarkdownImageCallback;
    config.userData = &context;
    config.formatCallback = MarkdownFormatCallback;
    config.linkIcon = "";
    for (int i = 0; i < ImGui::MarkdownConfig::NUMHEADINGS; ++i) {
        config.headingFormats[i].font = ImGui::GetFont();
        config.headingFormats[i].separator = heading_separator;
        config.headingFormats[i].fontSize = HeadingSize(i + 1, Theme::kFontBody, text_scale);
    }

    // 注意：必须自己开一个无装饰窗口。BeginChild 在"没有当前窗口"时会落到 ImGui 的
    // 隐式 Debug 窗口（屏幕上会多出一个带标题栏的框），所以这里显式 Begin/End。
    ImGui::PushID(this);
    const ImVec2 saved_spacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(saved_spacing.x, Maxf(line_gap, 0.0f)));
    ImGui::SetNextWindowPos(viewport.min);
    ImGui::SetNextWindowSize(viewport.Size());
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("##markdown", nullptr, flags)) {
        // content 已经含了容器的滚动偏移：直接按它排版，窗口自己负责裁剪
        ImGui::SetCursorScreenPos(content.min);
        ImGui::Markdown(text.c_str(), text.size(), config);
        rendered_height_ = Maxf(ImGui::GetCursorScreenPos().y - content.min.y, 1.0f);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopID();
}

bool MarkdownView::OnPadAction(InputAction action) {
    if (!scroll_keys) {
        return false;
    }
    int direction = 0;
    float lines = 1.0f;
    if (action == InputAction::Up) {
        direction = -1;
    } else if (action == InputAction::Down) {
        direction = 1;
    } else if (action == InputAction::PageLeft || action == InputAction::TriggerLeft) {
        direction = -1;
        lines = 4.0f;
    } else if (action == InputAction::PageRight || action == InputAction::TriggerRight) {
        direction = 1;
        lines = 4.0f;
    }
    if (direction == 0) {
        return false;
    }
    Widget* host = ScrollHostOrNull();
    if (host == nullptr || host->scroll_max.y <= 0.5f) {
        return false;
    }
    const float step = (Theme::kFontBody + line_gap) * lines;
    host->scroll_target.y = Clampf(host->scroll_target.y + static_cast<float>(direction) * step, 0.0f,
                                   host->scroll_max.y);
    return true;
}

} // namespace gui_dev::cv
