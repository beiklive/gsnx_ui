#include "component_view/popup/Popup.h"

#include <utility>

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

// 弹窗里的按钮统一用 TextButton（文字居中，适合确认/取消这类提示文字）
TextButton* MakeButton(Widget& parent, std::string text, std::string icon) {
    TextButton* button = parent.Emplace<TextButton>(std::move(text));
    button->setFontSize(Theme::kFontHeader);
    if (!icon.empty()) {
        button->setIcon(std::move(icon));
    }
    return button;
}

// Header 左侧那个方形图标 Box：透明底、描边与图标同色、图标比 Box 小一圈并居中。
// 只做弹窗 Header 用，所以放在这里（Popup.cpp）而不对外暴露。
class PopupHeaderBadge : public Box {
public:
    explicit PopupHeaderBadge(std::string widget_name) : Box(std::move(widget_name)) {}

    std::string glyph;                 // Material 字形
    ImVec4 icon_color = Theme::kAccent;

protected:
    void OnDrawOverlay(ImDrawList* dl, const Rect& content) override {
        if (glyph.empty() || dl == nullptr) {
            return;
        }
        ImFont* font = Draw::CurrentFont();
        const float size = Minf(content.Width(), content.Height()) * 0.62f; // 比 Box 小一点
        const ImVec2 measured = Draw::MeasureText(font, size, glyph.c_str(), 0.0f);
        float y = content.Center().y - measured.y * 0.5f;
        // 图标按"看得见的墨迹"居中：行盒下方带 descender，按行盒居中会显得偏上
        float ink_top = 0.0f;
        float ink_bottom = 0.0f;
        if (Draw::GlyphInkExtent(font, size, glyph.c_str(), ink_top, ink_bottom)) {
            y = content.Center().y - (ink_top + ink_bottom) * 0.5f;
        }
        Draw::Text(dl, font, size, ImVec2(content.Center().x - measured.x * 0.5f, y),
                   Theme::Alpha(Theme::U32(icon_color), EffectiveOpacity()), glyph.c_str(), 0.0f);
    }
};

// 透明度要下发给整棵子树：Widget::opacity 只作用于自己的绘制，子节点各画各的
void ApplyOpacityTree(Widget& widget, float opacity) {
    widget.opacity = opacity;
    for (auto& child : widget.children) {
        ApplyOpacityTree(*child, opacity);
    }
}

} // namespace

ImVec4 PopupAccentColor(PopupKind kind) {
    switch (kind) {
    case PopupKind::Info:
        return Theme::kPopupInfo;
    case PopupKind::Success:
        return Theme::kPopupSuccess;
    case PopupKind::Warning:
        return Theme::kPopupWarning;
    case PopupKind::Error:
        return Theme::kPopupError;
    case PopupKind::Confirm:
        return Theme::kPopupConfirm;
    case PopupKind::Selection:
        return Theme::kPopupSelection;
    case PopupKind::Progress:
        return Theme::kPopupProgress;
    case PopupKind::Custom:
    default:
        return Theme::kPopupNeutral;
    }
}

ImU32 PopupAccentU32(PopupKind kind) {
    return Theme::U32(PopupAccentColor(kind));
}

// Header 默认图标：只在这里按语义取一次，调用方可以用 setHeaderIcon() 覆盖
const char* PopupKindIcon(PopupKind kind) {
    switch (kind) {
    case PopupKind::Info:
        return Icons::Glyph(Icons::Material::Info);
    case PopupKind::Success:
        return Icons::Glyph(Icons::Material::CheckCircle);
    case PopupKind::Warning:
        return Icons::Glyph(Icons::Material::ErrorOutline);
    case PopupKind::Error:
        return Icons::Glyph(Icons::Material::ErrorOutline);
    case PopupKind::Confirm:
        return Icons::Glyph(Icons::Material::HelpOutline);
    case PopupKind::Selection:
        return Icons::Glyph(Icons::Material::SelectAll);
    case PopupKind::Progress:
        return Icons::Glyph(Icons::Material::Update);
    case PopupKind::Custom:
    default:
        return Icons::Glyph(Icons::Material::Edit);
    }
}

// ================================================================= 构造 =====

Popup::Popup(std::string name, PopupKind kind) : name_(std::move(name)), kind_(kind) {
    SyncVisualStyleFromGlobal();
    BuildChrome();
    ApplyKindColors();
}

// 弹窗不另立视觉体系：圆角 / 边框 / 阴影 / 内边距 / 类型条高度 / 遮罩都来自全局 Style。
void Popup::SyncVisualStyleFromGlobal() {
    const Global::ComponentStyle& global = Global::component_style;
    style_.padding = global.popup_padding;
    style_.header_size = global.popup_header_size;
    style_.header_gap = global.popup_header_gap;
    style_.gap = global.popup_gap;
    style_.min_width = global.popup_min_width;
    style_.max_width_ratio = global.popup_max_width_ratio;
    style_.max_height_ratio = global.popup_max_height_ratio;
    style_.backdrop_alpha = global.popup_backdrop_alpha;
    style_.button_min_width = global.popup_button_min_width;
    style_.corner_radius = global.corner_radius;
    if (window_ != nullptr) {
        window_->applyComponentStyle(); // 边框 / 圆角 / 阴影 = Box / Button 同一套
        window_->padding = EdgeInsets::All(style_.padding);
        window_->corner_radius = style_.corner_radius;
    }
    if (header_badge_ != nullptr) {
        // 方形图标 Box：透明底 + 语义色描边（描边色 = 图标色），圆角跟全局（Box/Button 同一套）
        const float radius = Minf(style_.header_size * 0.3f, style_.corner_radius + 1.0f);
        header_badge_->corner_radius = radius;
        header_badge_->background = 0; // 透明
        header_badge_->background_follows_theme = false;
        header_badge_->border.width = 1.5f;
        header_badge_->border.color = PopupAccentU32(kind_);
        header_badge_->shadow.enabled = false;
        header_badge_->interactive = false;
    }
    if (backdrop_ != nullptr) {
        backdrop_->background = Theme::U32(ImVec4(0.03f, 0.03f, 0.04f, style_.backdrop_alpha));
    }
}

Popup::~Popup() = default;

void Popup::BuildChrome() {
    // ---- 根：覆盖整个画布，透明、不吃装饰（和 Page::Root() 一个套路） ----
    root_owner_ = std::make_unique<Box>("popup_root");
    root_ = root_owner_.get();
    root_->background = 0;
    root_->background_follows_theme = false;
    root_->border.width = 0.0f;
    root_->shadow.enabled = false;
    root_->corner_radius = 0.0f;
    root_->padding = EdgeInsets{};
    root_->interactive = true; // 模态弹窗：命中不到具体控件时由根兜住，背景页面拿不到 hover

    // ---- 遮罩：z_order = -1（画在最下面、命中测试最后一个），点击可关闭（可配置） ----
    backdrop_ = root_->Emplace<Box>("popup_backdrop");
    backdrop_->z_order = -1;
    backdrop_->size = Global::canvas_size;
    backdrop_->background = Theme::U32(ImVec4(0.03f, 0.03f, 0.04f, style_.backdrop_alpha));
    backdrop_->background_follows_theme = false;
    backdrop_->border.width = 0.0f;
    backdrop_->shadow.enabled = false;
    backdrop_->corner_radius = 0.0f;
    connect(backdrop_, &Widget::clicked, this, [this] {
        if (IsOpen() && dismiss_on_backdrop_) {
            RequestDismiss();
        }
    });

    // ---- 窗口：居中、带阴影与类型条 ----
    window_ = root_->Emplace<Box>("popup_window");
    window_->anchor = ImVec2(0.5f, 0.5f);
    window_->pivot = ImVec2(0.5f, 0.5f);
    window_->position = ImVec2(0.0f, 0.0f);
    window_->applyComponentStyle(); // 与 Button / Box 完全同一套边框 + 圆角 + 阴影
    window_->padding = EdgeInsets::All(style_.padding);
    window_->corner_radius = style_.corner_radius;

    // Header：方形图标 Box（透明底 + 语义色描边 + 居中图标）+ 文字，整体在弹窗内边距之内。
    header_badge_ = window_->Emplace<PopupHeaderBadge>("popup_header_badge");
    header_badge_->position = ImVec2(0.0f, 0.0f);
    header_badge_->size = ImVec2(style_.header_size, style_.header_size);

    // 标题（Header 的文字部分；例如 提示 / 警告 / 通知 / 情绪 / 请选择 / 功能名）
    title_ = window_->Emplace<Label>("popup_title");
    title_->setFontSize(style_.title_size);
    title_->vertical_align = VerticalAlign::Middle;
    title_->visible = false;

    // 内容容器：纵向排列，宽度里塞什么就排什么（Checkbox / RadioGroup / Slider / 可滚动容器…）
    content_host_ = window_->Emplace<Box>("popup_content");
    content_host_->layout = LayoutMode::Vertical;
    content_host_->gap = ImVec2(0.0f, style_.gap * 0.6f);
    content_host_->align_x = Align::Stretch;
    content_host_->background = 0;
    content_host_->background_follows_theme = false;
    content_host_->border.width = 0.0f;
    content_host_->shadow.enabled = false;
    content_host_->padding = EdgeInsets{};

    // 按钮组：默认横排居中（纵向排列由 setButtonLayout 切换）
    buttons_box_ = window_->Emplace<Box>("popup_buttons");
    buttons_box_->layout = LayoutMode::Horizontal;
    buttons_box_->gap = ImVec2(style_.button_gap, 0.0f);
    buttons_box_->align_x = Align::Center;
    buttons_box_->background = 0;
    buttons_box_->background_follows_theme = false;
    buttons_box_->border.width = 0.0f;
    buttons_box_->shadow.enabled = false;
    buttons_box_->padding = EdgeInsets{};
}

void Popup::ApplyKindColors() {
    ApplyKindHeader();
}

void Popup::ApplyKindHeader() {
    if (header_badge_ == nullptr) {
        return;
    }
    const ImVec4 accent = PopupAccentColor(kind_);
    header_badge_->background = 0;
    header_badge_->background_follows_theme = false;
    header_badge_->border.width = 1.5f;
    header_badge_->border.color = PopupAccentU32(kind_);
    if (auto* badge = dynamic_cast<PopupHeaderBadge*>(header_badge_)) {
        badge->glyph = header_icon_glyph_.empty() ? PopupKindIcon(kind_) : header_icon_glyph_;
        badge->icon_color = accent; // 描边与图标同色
    }
}

// ============================================================= 配置接口 =====

Popup& Popup::setKind(PopupKind value) {
    kind_ = value;
    ApplyKindColors();
    return *this;
}

Popup& Popup::setHeaderIcon(std::string glyph) {
    header_icon_glyph_ = std::move(glyph);
    ApplyKindHeader();
    return *this;
}

Popup& Popup::setTitle(std::string value) {
    if (title_ != nullptr) {
        title_->setText(std::move(value));
        title_->visible = !title_->text.empty();
    }
    return *this;
}

Popup& Popup::setModal(bool value) {
    modal_ = value;
    if (root_ != nullptr) {
        root_->interactive = modal_;
    }
    return *this;
}

Popup& Popup::setBackdrop(bool enabled, float alpha) {
    style_.backdrop = enabled;
    if (alpha >= 0.0f) {
        style_.backdrop_alpha = alpha;
    }
    if (backdrop_ != nullptr) {
        backdrop_->visible = enabled && modal_;
        backdrop_->background = Theme::U32(ImVec4(0.03f, 0.03f, 0.04f, style_.backdrop_alpha));
    }
    return *this;
}

Popup& Popup::setDismissOnBackdrop(bool enabled) {
    dismiss_on_backdrop_ = enabled;
    return *this;
}

Popup& Popup::setDismissOnCancel(bool enabled) {
    dismiss_on_cancel_ = enabled;
    return *this;
}

Popup& Popup::setAutoCloseOnButton(bool enabled) {
    auto_close_on_button_ = enabled;
    return *this;
}

Popup& Popup::setScope(PopupScope value) {
    scope_ = value;
    return *this;
}

Popup& Popup::setSize(float width, float height) {
    style_.width = width;
    style_.height = height;
    return *this;
}

Popup& Popup::setMinSize(float width, float height) {
    style_.min_width = width;
    style_.min_height = height;
    return *this;
}

Popup& Popup::setMaxSize(float width, float height) {
    style_.max_width = width;
    // max_height 用比例表达（>1 视为像素）
    style_.max_height_ratio = height > 1.0f ? height : style_.max_height_ratio;
    return *this;
}

Popup& Popup::setButtonLayout(PopupButtonLayout value) {
    button_layout_ = value;
    if (buttons_box_ != nullptr) {
        buttons_box_->layout = value == PopupButtonLayout::Vertical ? LayoutMode::Vertical : LayoutMode::Horizontal;
        buttons_box_->gap = value == PopupButtonLayout::Vertical ? ImVec2(0.0f, style_.button_row_gap)
                                                                 : ImVec2(style_.button_gap, 0.0f);
        buttons_box_->align_x = value == PopupButtonLayout::Vertical ? Align::Stretch : Align::Center;
    }
    return *this;
}

Popup& Popup::setDefaultFocus(int index) {
    default_focus_ = index;
    return *this;
}

Popup& Popup::setAnimated(bool value) {
    style_.animated = value;
    return *this;
}

Popup& Popup::setStyle(const PopupStyle& value) {
    style_ = value;
    if (window_ != nullptr) {
        window_->padding = EdgeInsets::All(style_.padding);
        window_->corner_radius = style_.corner_radius;
    }
    return *this;
}

// =============================================================== 内容 =====

void Popup::RebuildContent() {
    if (content_host_ == nullptr) {
        return;
    }
    content_host_->Clear();
    message_ = nullptr;
    progress_bar_ = nullptr;
    image_ = nullptr;
    rich_text_ = nullptr;
    markdown_scroll_ = nullptr;
    image_viewer_ = nullptr;
    if (content_builder_) {
        content_builder_(*content_host_);
    }
}

Box& Popup::content() {
    return *content_host_;
}

Popup& Popup::setText(std::string text) {
    content_builder_ = [text = std::move(text)](Widget& host) {
        Label* label = host.Emplace<Label>(text);
        label->setWrap(true, 6.0f);
        label->font_size = Theme::kFontBody;
        label->setColor(Theme::kTextPrimary);
    };
    RebuildContent();
    return *this;
}

Popup& Popup::setImageViewer(std::string image_path) {
    content_builder_ = [this, image_path = std::move(image_path)](Widget& host) {
        ImageViewer* viewer = host.Emplace<ImageViewer>(image_path);
        viewer->size.y = 0.0f; // 高度由 Popup::Layout 按可用区给
        viewer->SetCloseCallback([this] { Close(); });
        image_viewer_ = viewer;
    };
    RebuildContent();
    return *this;
}

Popup& Popup::setMarkdown(std::string markdown, RichText::ImageResolver resolver, float view_height) {
    // 滚动容器仍然是普通 Box + Overflow::Scroll（不新造 ScrollView 类型）
    const float image_max = style_.image_max_height;
    content_builder_ = [markdown = std::move(markdown), resolver = std::move(resolver), view_height,
                        image_max](Widget& host) {
        Box* view = host.Emplace<Box>("markdown_scroll");
        view->overflow = Overflow::Scroll;
        view->scroll_bar_auto_hide = true;
        view->scroll_overscroll = true;
        view->background = 0;
        view->background_follows_theme = false;
        view->border.width = 0.0f;
        view->shadow.enabled = false;
        view->padding = EdgeInsets{};
        view->size.y = view_height;
        RichText* text = view->Emplace<RichText>();
        text->SetMarkdown(markdown);
        text->SetImageResolver(resolver);
        // 焦点给正文：手柄上下键可以滚这一段（RichText 自己不做滚动容器，只滚外部容器）
        text->focusable = true;
        text->focus_frame = true;
        text->focus_frame_offset = 3.0f;
        text->SetScrollKeys(true); // 弹窗里读长文：上下键滚这段正文
        text->SetMaxImageSize(0.0f, image_max);
    };
    RebuildContent();
    markdown_scroll_ = nullptr;
    rich_text_ = nullptr;
    if (content_host_ != nullptr && !content_host_->children.empty()) {
        if (Widget* view = content_host_->children.front().get()) {
            markdown_scroll_ = dynamic_cast<Box*>(view);
            if (!view->children.empty()) {
                rich_text_ = dynamic_cast<RichText*>(view->children.front().get());
            }
        }
    }
    return *this;
}

Popup& Popup::setImage(ImTextureRef texture, float width, float height) {
    content_builder_ = [texture, width, height](Widget& host) {
        Image* image = host.Emplace<Image>();
        image->setTexture(texture, width, height);
        image->setRadius(Global::component_style.corner_radius);
        image->max_size = ImVec2(0.0f, 320.0f); // 限高：弹窗不会因为图片反复变高变矮
    };
    RebuildContent();
    if (content_host_ != nullptr && !content_host_->children.empty()) {
        image_ = dynamic_cast<Image*>(content_host_->children.front().get());
    }
    return *this;
}

Popup& Popup::setProgressContent(std::string message, bool indeterminate) {
    content_builder_ = [message = std::move(message), indeterminate](Widget& host) {
        ProgressBar* bar = host.Emplace<ProgressBar>();
        bar->setIndeterminate(indeterminate);
        bar->show_percent = !indeterminate;
        bar->setBarHeight(12.0f);
        Label* text = host.Emplace<Label>(message);
        text->setWrap(true, 6.0f);
        text->font_size = Theme::kFontSmall;
        text->setColor(Theme::kTextMuted);
        text->visible = !text->text.empty();
    };
    RebuildContent();
    if (content_host_ != nullptr) {
        for (auto& child : content_host_->children) {
            if (auto* bar = dynamic_cast<ProgressBar*>(child.get())) {
                progress_bar_ = bar;
            } else if (auto* label = dynamic_cast<Label*>(child.get())) {
                message_ = label;
            }
        }
    }
    return *this;
}

Popup& Popup::setContentBuilder(std::function<void(Widget& content)> builder) {
    content_builder_ = std::move(builder);
    RebuildContent();
    return *this;
}

// =============================================================== 进度 =====

Popup& Popup::setProgress(float value) {
    if (progress_bar_ != nullptr) {
        progress_bar_->setValue(value);
        progress_bar_->setIndeterminate(false);
    }
    return *this;
}

Popup& Popup::setIndeterminate(bool enabled) {
    if (progress_bar_ != nullptr) {
        progress_bar_->setIndeterminate(enabled);
        progress_bar_->show_percent = !enabled;
    }
    return *this;
}

Popup& Popup::setMessage(std::string message) {
    if (message_ != nullptr) {
        message_->setText(std::move(message));
        message_->visible = !message_->text.empty();
    } else if (content_host_ != nullptr) {
        Label* label = content_host_->Emplace<Label>(std::move(message));
        label->setWrap(true, 6.0f);
        label->font_size = Theme::kFontSmall;
        label->setColor(Theme::kTextMuted);
        message_ = label;
    }
    return *this;
}

Popup& Popup::fail(std::string message) {
    setKind(PopupKind::Error);
    setIndeterminate(false);
    if (progress_bar_ != nullptr) {
        progress_bar_->fill_color = Theme::kError;
    }
    setMessage(std::move(message));
    // 失败后不能靠任务自动关闭：给一个明确的关闭方式
    if (buttons_.empty()) {
        ButtonSpec spec;
        spec.text = "关闭";
        spec.icon = std::string();
        addButton(std::move(spec));
        setDefaultFocus(0);
    }
    return *this;
}

// =============================================================== 按钮 =====

Popup& Popup::addButton(ButtonSpec spec) {
    const int index = static_cast<int>(buttons_.size());
    TextButton* button = MakeButton(*buttons_box_, spec.text, spec.icon);
    button->SetName("popup_button_" + std::to_string(index));
    button->resize(style_.button_min_width, style_.button_height);
    if (spec.primary) {
        button->setBorder(1.0f, PopupAccentColor(kind_));
        button->setTextColors(PopupAccentColor(kind_), Theme::kTextMuted);
    } else if (spec.color.w > 0.0f) {
        button->setTextColors(spec.color, Theme::kTextMuted);
    }
    connect(button, &Widget::clicked, this, [this, index] {
        emit buttonClicked(index);
        ButtonEntry& entry = buttons_[static_cast<std::size_t>(index)];
        if (entry.spec.on_click) {
            entry.spec.on_click();
        }
        if (entry.spec.close_on_click && auto_close_on_button_) {
            Close();
        }
    });
    ButtonEntry entry;
    entry.spec = std::move(spec);
    entry.button = button;
    buttons_.push_back(std::move(entry));
    return *this;
}

Popup& Popup::addButton(std::string text, std::function<void()> on_click) {
    ButtonSpec spec;
    spec.text = std::move(text);
    spec.on_click = std::move(on_click);
    return addButton(std::move(spec));
}

Button* Popup::buttonAt(int index) const {
    if (index < 0 || index >= static_cast<int>(buttons_.size())) {
        return nullptr;
    }
    return buttons_[static_cast<std::size_t>(index)].button;
}

// =========================================================== 生命周期 =====

void Popup::Open() {
    if (state_ == PopupState::Opening || state_ == PopupState::Visible) {
        return;
    }
    emit opening();
    if (!style_.animated || style_.open_duration <= 0.0f) {
        state_ = PopupState::Visible;
        progress_ = 1.0f;
        emit opened();
        return;
    }
    state_ = PopupState::Opening;
    progress_ = 0.0f;
}

void Popup::Close() {
    if (state_ == PopupState::Closing || state_ == PopupState::Closed) {
        return;
    }
    emit closing();
    if (!style_.animated || style_.close_duration <= 0.0f) {
        state_ = PopupState::Closed;
        progress_ = 0.0f;
        emit closed();
        return;
    }
    state_ = PopupState::Closing;
}

void Popup::CloseNow() {
    if (state_ == PopupState::Closed) {
        return;
    }
    state_ = PopupState::Closed;
    progress_ = 0.0f;
    emit closed();
}

void Popup::RequestDismiss() {
    dismiss_requested_ = true;
    emit dismissRequested();
    Close();
}

// =============================================================== 布局 =====

float Popup::ResolveWidth(float canvas_width) const {
    if (style_.width > 1.0f) {
        return style_.width;
    }
    const float limit = Minf(canvas_width * style_.max_width_ratio, style_.max_width);
    const float base = style_.width > 0.0f ? canvas_width * style_.width : limit;
    return Clampf(base, style_.min_width, Maxf(limit, style_.min_width));
}

float Popup::ResolveHeightLimit(float canvas_height) const {
    return Maxf(canvas_height * style_.max_height_ratio, 80.0f);
}

void Popup::PositionChildren() {
    // 第一行是 Header：方形图标 Box + 文字（标题），后面依次是内容 / 按钮
    const float header_height =
        Maxf(style_.header_size, (title_ != nullptr && title_->visible) ? title_->measured_size.y : 0.0f);
    if (header_badge_ != nullptr) {
        header_badge_->position = ImVec2(0.0f, (header_height - style_.header_size) * 0.5f);
        header_badge_->size = ImVec2(style_.header_size, style_.header_size);
    }
    float y = header_height + style_.gap;
    const float title_height = (title_ != nullptr && title_->visible) ? title_->measured_size.y : 0.0f;
    if (title_height > 0.0f) {
        title_->position = ImVec2(style_.header_size + style_.header_gap, (header_height - title_height) * 0.5f);
        title_->size.y = title_height;
    } else if (title_ != nullptr) {
        title_->position = ImVec2(style_.header_size + style_.header_gap, 0.0f);
    }

    const float content_height = content_host_->measured_size.y;
    content_host_->position = ImVec2(0.0f, y);
    y += content_height + (buttons_.empty() ? 0.0f : style_.gap);

    if (buttons_box_ != nullptr) {
        buttons_box_->position = ImVec2(0.0f, y);
    }
}

void Popup::Layout() {
    if (root_ == nullptr || state_ == PopupState::Closed) {
        return;
    }
    const Rect canvas = Global::CanvasRect();

    root_->position = ImVec2(0.0f, 0.0f);
    root_->size = canvas.Size();
    backdrop_->position = ImVec2(0.0f, 0.0f);
    backdrop_->size = canvas.Size();
    backdrop_->visible = style_.backdrop && modal_;
    root_->interactive = modal_;

    const float width = ResolveWidth(canvas.Width());
    width_ = width;
    const float chrome = (style_.padding + window_->border.width) * 2.0f;
    const float inner_width = Maxf(width - chrome, 40.0f);

    // 关键：测量阶段的窗口高度先钉在「高度上限」，不要用自适应高度去测自适应内容。
    // 自适应高度本身依赖子节点测量结果，而 Image 这类控件又会按 available 反向撑满 →
    // 高度会来回震荡（表现为弹窗里的图片和底部按钮上下抖）。
    const float height_limit = ResolveHeightLimit(canvas.Height());
    window_->size.x = width;
    window_->size.y = style_.height > 1.0f ? Minf(style_.height, height_limit) : height_limit;

    if (title_ != nullptr && title_->visible) {
        title_->size.x = Maxf(inner_width - style_.header_size - style_.header_gap, 40.0f);
    }
    // 图片浏览器：铺满弹窗内容区（Header / 按钮组高度用上一帧的值，稳定后逐帧一致）
    if (image_viewer_ != nullptr) {
        const float free_for_viewer = Maxf(height_limit - chrome - last_header_block_ - last_buttons_height_ -
                                                (buttons_.empty() ? 0.0f : style_.gap),
                                            160.0f);
        image_viewer_->size = ImVec2(inner_width, free_for_viewer);
    }
    // Markdown 正文自带"内容高度"（上一帧排好的）；滚动容器高度 = 固定视口 或 内容高度
    if (markdown_scroll_ != nullptr && rich_text_ != nullptr) {
        rich_text_->size.x = inner_width;
        const float content_height = Maxf(rich_text_->ContentHeight(), 40.0f);
        markdown_scroll_->size.x = inner_width;
        markdown_scroll_->size.y = style_.markdown_height > 1.0f ? style_.markdown_height : content_height;
    }
    content_host_->size.x = inner_width;
    if (!scrollable_) {
        content_host_->size.y = 0.0f;
    }
    content_host_->overflow = scrollable_ ? Overflow::Scroll : Overflow::Visible;
    buttons_box_->size.x = inner_width;

    // 按钮等宽：第一次布局量出各自需要的宽度，取最大值（不低于最小宽度）
    if (!buttons_.empty()) {
        if (button_layout_ == PopupButtonLayout::Vertical) {
            for (ButtonEntry& entry : buttons_) {
                entry.button->size = ImVec2(inner_width, style_.button_height);
            }
        } else {
            root_->LayoutTree(canvas.min, canvas.Size());
            float max_width = style_.button_min_width;
            for (ButtonEntry& entry : buttons_) {
                max_width = Maxf(max_width, entry.button->measured_size.x);
            }
            const float total = static_cast<float>(buttons_.size()) * max_width +
                                static_cast<float>(buttons_.size() - 1) * style_.button_gap;
            const float width_use = total > inner_width ? (inner_width - static_cast<float>(buttons_.size() - 1) *
                                                                              style_.button_gap) /
                                                              static_cast<float>(buttons_.size())
                                                        : max_width;
            for (ButtonEntry& entry : buttons_) {
                entry.button->size = ImVec2(Maxf(width_use, 60.0f), style_.button_height);
            }
        }
    }

    // ---- 第一次布局（窗口固定为高度上限）：量出 Header / 内容 / 按钮组 的高度 ----
    root_->LayoutTree(canvas.min, canvas.Size());
    const float title_height = (title_ != nullptr && title_->visible) ? title_->measured_size.y : 0.0f;
    const float content_height = content_host_->measured_size.y;
    const float buttons_height = buttons_.empty() ? 0.0f : buttons_box_->measured_size.y;
    const float header_height = Maxf(style_.header_size, title_height);
    const float gaps = (buttons_.empty() ? 0.0f : style_.gap);
    const float header_block = header_height + style_.gap; // Header 行 + 它下面那一行间距
    last_header_block_ = header_block;
    last_buttons_height_ = buttons_height;
    const float desired_height = chrome + header_block + content_height + gaps + buttons_height;

    // ---- 定高决策：内容装不下就让内容容器自己滚（ScrollView），窗口不再长高 ----
    // 用一点迟滞（±4px）：避免内容高度刚好压在上限上时，每帧在「滚动/不滚动」之间反复切换，
    // 那同样会让底部按钮上下跳。
    const bool wants_scroll = style_.height > 1.0f   ? desired_height > Minf(style_.height, height_limit)
                              : style_.height > 0.0f ? desired_height > Minf(canvas.Height() * style_.height, height_limit)
                                                     : desired_height > height_limit;
    if (wants_scroll == scrollable_) {
        // 保持上一帧判断
    } else if (wants_scroll) {
        scrollable_ = desired_height > height_limit + 4.0f;
    } else {
        scrollable_ = desired_height > height_limit - 4.0f;
    }

    if (style_.height > 1.0f) {
        window_->size.y = Minf(style_.height, height_limit);
    } else if (style_.height > 0.0f) {
        window_->size.y = Minf(canvas.Height() * style_.height, height_limit);
    } else {
        window_->size.y = scrollable_ ? height_limit : desired_height;
    }
    if (style_.min_height > 0.0f && window_->size.y < style_.min_height) {
        window_->size.y = style_.min_height;
    }

    if (scrollable_) {
        const float free_for_content =
            Maxf(window_->size.y - chrome - header_block - gaps - buttons_height, 60.0f);
        content_host_->size.y = free_for_content;
        content_host_->overflow = Overflow::Scroll;
    } else {
        content_host_->size.y = 0.0f;
        content_host_->overflow = Overflow::Visible;
    }

    PositionChildren();

    // ---- 第二次布局：用最终尺寸落位 ----
    root_->LayoutTree(canvas.min, canvas.Size());

}

// =============================================================== 每帧 =====

void Popup::UpdateTree(float dt) {
    if (root_ == nullptr) {
        return;
    }
    // 关闭动画期间不再接受输入（背景也不该在这时候被操作）
    if (state_ == PopupState::Closing) {
        return;
    }
    root_->UpdateTree(dt);
}

void Popup::Update(float dt) {
    if (state_ == PopupState::Opening) {
        progress_ = Anim::MoveTowards(progress_, 1.0f, style_.open_duration, dt);
        if (progress_ >= 1.0f) {
            progress_ = 1.0f;
            state_ = PopupState::Visible;
            emit opened();
        }
    } else if (state_ == PopupState::Closing) {
        progress_ = Anim::MoveTowards(progress_, 0.0f, style_.close_duration, dt);
        if (progress_ <= 0.0f) {
            progress_ = 0.0f;
            state_ = PopupState::Closed;
            emit closed();
        }
    } else if (state_ == PopupState::Visible) {
        progress_ = 1.0f;
    }

    // 出场动画：缩放 + 上移 + 淡入（关闭时反向播放）
    const float eased = Anim::EaseOutCubic(progress_);
    const float scale = style_.open_scale_from + (1.0f - style_.open_scale_from) * eased;
    if (window_ != nullptr) {
        window_->visual_scale = ImVec2(scale, scale);
        window_->visual_translate = ImVec2(0.0f, style_.open_translate_y * (1.0f - eased));
    }
    if (backdrop_ != nullptr) {
        backdrop_->opacity = eased;
    }
    // 淡入淡出要下发到整棵子树（Widget::opacity 只作用于自己）；动画结束后不再覆盖，
    // 免得把控件自己设过的半透明（例如禁用项）改回 1.0。
    if (window_ != nullptr && eased < 0.999f) {
        ApplyOpacityTree(*window_, eased);
    } else if (window_ != nullptr) {
        window_->opacity = 1.0f;
    }
}

void Popup::Draw(ImDrawList* dl) {
    if (root_ == nullptr || state_ == PopupState::Closed) {
        return;
    }
    root_->DrawTree(dl);
}

Widget* Popup::HitTest(const ImVec2& point) {
    if (root_ == nullptr || state_ == PopupState::Closed || state_ == PopupState::Closing) {
        return nullptr;
    }
    return root_->HitTest(point);
}

bool Popup::EnsureVisible(Widget* target) {
    if (root_ == nullptr || target == nullptr) {
        return false;
    }
    return root_->EnsureVisible(target);
}

Widget& Popup::Root() {
    return *root_;
}

Widget* Popup::window() const {
    return window_;
}

Widget* Popup::contentWidget() const {
    return content_host_;
}

Widget* Popup::DefaultFocusTarget() const {
    if (default_focus_ >= 0 && default_focus_ < static_cast<int>(buttons_.size())) {
        if (Button* button = buttons_[static_cast<std::size_t>(default_focus_)].button) {
            return button;
        }
    }
    // 自定义页面里有控件时优先落在内容上（Tab/List/Slider…）
    if (content_host_ != nullptr) {
        if (Widget* inner = content_host_->FirstFocusable()) {
            return inner;
        }
    }
    if (!buttons_.empty()) {
        return buttons_.front().button;
    }
    return root_ != nullptr ? root_->FirstFocusable() : nullptr;
}

void Popup::RefreshTheme() {
    if (root_ == nullptr) {
        return;
    }
    root_->RefreshThemeTree();
    // 根/遮罩/内容容器都是“不画装饰”的容器，主题刷新会把 Box 默认装饰带回来，这里复位
    root_->background = 0;
    root_->background_follows_theme = false;
    root_->border.width = 0.0f;
    root_->shadow.enabled = false;
    backdrop_->background = Theme::U32(ImVec4(0.03f, 0.03f, 0.04f, style_.backdrop_alpha));
    backdrop_->background_follows_theme = false;
    backdrop_->border.width = 0.0f;
    backdrop_->shadow.enabled = false;
    content_host_->background = 0;
    content_host_->background_follows_theme = false;
    content_host_->border.width = 0.0f;
    content_host_->shadow.enabled = false;
    buttons_box_->background = 0;
    buttons_box_->background_follows_theme = false;
    buttons_box_->border.width = 0.0f;
    buttons_box_->shadow.enabled = false;
    ApplyKindColors();
    SyncVisualStyleFromGlobal();
}

} // namespace gui_dev::cv
