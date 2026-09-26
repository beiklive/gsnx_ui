#include "component_view/components/ImageViewer.h"

#include <algorithm>
#include <cctype>
#include <utility>

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/components/Content.h"
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

constexpr float kInfoHeight = 24.0f;
constexpr float kStateIconSize = 40.0f;

std::string LowerExtension(const std::string& path) {
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return {};
    }
    std::string extension = path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension;
}

std::string BaseName(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

// 一次无边框文字：状态界面（Empty / Loading / Failed）共用
Label* MakeLabel(Widget& parent, const char* name, float font_size, const ImVec4& color) {
    Label* label = parent.Emplace<Label>();
    label->SetName(name);
    label->font_size = font_size;
    label->setColor(color);
    label->setAlign(TextAlign::Center);
    label->visible = false;
    return label;
}

TextButton* MakeToolButton(Widget& parent, const char* name, const char* text) {
    TextButton* button = parent.Emplace<TextButton>(text);
    button->SetName(name);
    button->setFontSize(Theme::kFontSmall);
    button->focus_margin = -1.0f;
    return button;
}

} // namespace

ImageViewer::ImageViewer() : Widget("image_viewer") {
    // 自己就是「图片区」的焦点停靠点：手柄方向键在图片超出可视区时用来平移，
    // 没超出时（capture 关闭）方向键照常把焦点送去 Toolbar 按钮。
    focusable = true;
    focus_frame = true;
    focus_frame_offset = 3.0f;
    BuildUi();
    ApplyLabels();
    SetState(State::Empty);
}

ImageViewer::ImageViewer(std::string image_path) : ImageViewer() {
    SetImagePath(std::move(image_path));
}

// ---------------------------------------------------------------- 建 UI ----

void ImageViewer::BuildUi() {
    // 图片画在 OnDrawContent 里；这里只建状态界面与工具条（全部是现有控件）
    canvas_ = Emplace<Box>("viewer_canvas");
    canvas_->background = 0;
    canvas_->background_follows_theme = false;
    canvas_->border.width = 0.0f;
    canvas_->shadow.enabled = false;
    canvas_->interactive = false; // 命中测试落在 ImageViewer 自己身上
    canvas_->focusable = false;

    info_ = MakeLabel(*this, "viewer_info", Theme::kFontSmall, Theme::kTextMuted);
    info_->setAlign(TextAlign::Left);

    status_ = MakeLabel(*this, "viewer_status", Theme::kFontHeader, Theme::kTextPrimary);
    status_detail_ = MakeLabel(*this, "viewer_status_detail", Theme::kFontSmall, Theme::kTextMuted);

    status_bar_ = Emplace<ProgressBar>();
    status_bar_->SetName("viewer_progress");
    status_bar_->setIndeterminate(true);
    status_bar_->show_percent = false;
    status_bar_->setBarHeight(8.0f);
    status_bar_->visible = false;
    status_bar_->focusable = false;

    error_actions_ = Emplace<Box>("viewer_error_actions");
    error_actions_->layout = LayoutMode::Horizontal;
    error_actions_->gap = ImVec2(Global::component_style.content_padding, 0.0f);
    error_actions_->align_x = Align::Center;
    error_actions_->background = 0;
    error_actions_->background_follows_theme = false;
    error_actions_->border.width = 0.0f;
    error_actions_->shadow.enabled = false;
    error_actions_->padding = EdgeInsets{};
    error_actions_->visible = false;
    retry_button_ = MakeToolButton(*error_actions_, "viewer_retry", "重试");
    error_close_button_ = MakeToolButton(*error_actions_, "viewer_error_close", "关闭");
    connect(retry_button_, &Widget::clicked, this, [this] { Reload(); });
    connect(error_close_button_, &Widget::clicked, this, [this] {
        if (close_callback_) {
            close_callback_();
        }
        emit closeRequested();
    });

    // 底部工具条：全部是普通 Button，视觉/交互/焦点都复用 Button
    toolbar_ = Emplace<Box>("viewer_toolbar");
    toolbar_->layout = LayoutMode::Horizontal;
    toolbar_->gap = ImVec2(Global::component_style.content_padding * 0.5f, 0.0f);
    toolbar_->align_x = Align::Start;
    toolbar_->background = 0;
    toolbar_->background_follows_theme = false;
    toolbar_->border.width = 0.0f;
    toolbar_->shadow.enabled = false;
    toolbar_->padding = EdgeInsets{};

    fit_button_ = MakeToolButton(*toolbar_, "viewer_fit", "适应屏幕");
    actual_button_ = MakeToolButton(*toolbar_, "viewer_actual", "100%");
    zoom_out_button_ = MakeToolButton(*toolbar_, "viewer_zoom_out", "－");
    zoom_in_button_ = MakeToolButton(*toolbar_, "viewer_zoom_in", "＋");
    reset_button_ = MakeToolButton(*toolbar_, "viewer_reset", "重置");
    close_button_ = MakeToolButton(*toolbar_, "viewer_close", "关闭");

    connect(fit_button_, &Widget::clicked, this, [this] { FitToWindow(); });
    connect(actual_button_, &Widget::clicked, this, [this] { ActualSize(); });
    connect(zoom_out_button_, &Widget::clicked, this, [this] { ZoomOut(); });
    connect(zoom_in_button_, &Widget::clicked, this, [this] { ZoomIn(); });
    connect(reset_button_, &Widget::clicked, this, [this] { ResetView(); });
    connect(close_button_, &Widget::clicked, this, [this] {
        if (close_callback_) {
            close_callback_();
        }
        emit closeRequested();
    });
}

void ImageViewer::ApplyLabels() {
    if (fit_button_ == nullptr) {
        return;
    }
    // 信息行按需求只显示：文件名 + 尺寸 + 缩放比例（不显示完整路径）
    char info[160];
    const int percent = static_cast<int>(scale_ * 100.0f + 0.5f);
    if (state_ == State::Loaded) {
        std::snprintf(info, sizeof(info), "%s    %d × %d    %d%%", file_name_.c_str(), image_.width, image_.height,
                      percent);
    } else if (!file_name_.empty()) {
        std::snprintf(info, sizeof(info), "%s", file_name_.c_str());
    } else {
        std::snprintf(info, sizeof(info), "未选择图片");
    }
    info_->SetName("viewer_info");
    info_->setText(info);
}

// ------------------------------------------------------------ 状态机 -----

void ImageViewer::SetState(State next, std::string error) {
    error_text_ = std::move(error);
    if (state_ == next && next != State::Failed) {
        return;
    }
    state_ = next;
    const bool loaded = state_ == State::Loaded;
    const bool failed = state_ == State::Failed;
    const bool loading = state_ == State::Loading;

    if (status_ != nullptr) {
        status_->visible = !loaded;
        status_->setIcon(failed ? std::string(Icons::Glyph(Icons::Material::ErrorOutline))
                                : loading ? std::string(Icons::Glyph(Icons::Material::Update))
                                          : std::string(Icons::Glyph(Icons::Material::ImagePlaceholder)));
        status_->setText(failed ? "图片加载失败"
                                : loading ? "正在加载图片…"
                                          : "没有选择图片");
    }
    if (status_detail_ != nullptr) {
        status_detail_->visible = failed;
        status_detail_->setText(error_text_);
    }
    if (status_bar_ != nullptr) {
        status_bar_->visible = loading && show_state_indicators_;
    }
    if (error_actions_ != nullptr) {
        error_actions_->visible = failed;
    }
    if (toolbar_ != nullptr) {
        // Empty / Loading / Failed 时只留「关闭」（Failed 另外有「重试」）
        const bool show_view_buttons = loaded;
        fit_button_->visible = show_view_buttons;
        actual_button_->visible = show_view_buttons;
        zoom_out_button_->visible = show_view_buttons;
        zoom_in_button_->visible = show_view_buttons;
        reset_button_->visible = show_view_buttons;
        toolbar_->visible = toolbar_visible_ && (toolbar_autohide_ ? loaded : true);
    }
    ApplyLabels();
    emit stateChanged(state_);
}

void ImageViewer::RequestLoad() {
    load_pending_ = true;
}

void ImageViewer::LoadIfPending() {
    if (!load_pending_) {
        return;
    }
    load_pending_ = false;
    Unload();

    if (path_.empty()) {
        SetState(State::Empty);
        return;
    }
    SetState(State::Loading);

    const std::string extension = LowerExtension(path_);
    bool supported = supported_extensions_.empty();
    for (const std::string& known : supported_extensions_) {
        if (known == extension) {
            supported = true;
            break;
        }
    }
    if (!supported) {
        SetState(State::Failed, "不支持的图片格式：" + (extension.empty() ? std::string("(无扩展名)") : extension));
        return;
    }
    if (!Global::image_source.load) {
        SetState(State::Failed, "宿主没有注册 Global::image_source，无法按路径加载图片");
        return;
    }

    Global::ImageHandle handle = Global::image_source.load(path_.c_str());
    if (!handle.Valid()) {
        SetState(State::Failed, handle.error.empty() ? std::string("无法解码该图片") : handle.error);
        return;
    }
    image_.texture = handle.texture;
    image_.width = handle.width;
    image_.height = handle.height;
    loaded_once_ = true;
    fit_mode_ = true;
    pan_ = ImVec2(0.0f, 0.0f);
    SetState(State::Loaded);
    RevealToolbar();
}

void ImageViewer::Unload() {
    if (image_.texture.GetTexID() == ImTextureID_Invalid) {
        return;
    }
    if (Global::image_source.release) {
        Global::ImageHandle handle;
        handle.texture = image_.texture;
        handle.width = image_.width;
        handle.height = image_.height;
        Global::image_source.release(handle);
    }
    image_ = ImageData{};
}

// ------------------------------------------------------------ 内容 / 视图 --

ImageViewer& ImageViewer::SetImagePath(std::string path) {
    const bool same = (path == path_);
    path_ = std::move(path);
    file_name_ = BaseName(path_);
    if (!same) {
        RequestLoad();
    }
    return *this;
}

ImageViewer& ImageViewer::Reload() {
    RequestLoad();
    return *this;
}

ImageViewer& ImageViewer::SetZoom(float scale) {
    // 以视口中心为缩放基准（按钮 / L R / 手柄都走这里）
    return SetZoomAt(scale, ImVec2(viewport_size_.x * 0.5f, viewport_size_.y * 0.5f));
}

ImageViewer& ImageViewer::SetZoomAt(float scale, const ImVec2& focal) {
    const float next = Clampf(scale, zoom_steps_.front() * 0.5f, zoom_steps_.back() * 2.0f);
    if (Absf(next - scale_) < 0.0001f) {
        return *this;
    }
    const float ratio = next / Maxf(scale_, 0.0001f);
    // 焦点（相对视口中心）保持不动：pan_ 是图片中心相对视口中心的偏移
    const ImVec2 offset(focal.x - viewport_size_.x * 0.5f, focal.y - viewport_size_.y * 0.5f);
    pan_.x = offset.x - (offset.x - pan_.x) * ratio;
    pan_.y = offset.y - (offset.y - pan_.y) * ratio;
    scale_ = next;
    fit_mode_ = Absf(scale_ - fit_scale_) < 0.005f;
    ClampPan();
    ApplyLabels();
    emit zoomChanged(scale_);
    return *this;
}

ImageViewer& ImageViewer::ZoomIn() {
    if (state_ != State::Loaded) {
        return *this;
    }
    for (float step : zoom_steps_) {
        if (step > scale_ + 0.001f) {
            return SetZoom(step);
        }
    }
    return SetZoom(zoom_steps_.back());
}

ImageViewer& ImageViewer::ZoomOut() {
    if (state_ != State::Loaded) {
        return *this;
    }
    for (std::size_t i = zoom_steps_.size(); i > 0; --i) {
        if (zoom_steps_[i - 1] < scale_ - 0.001f) {
            return SetZoom(zoom_steps_[i - 1]);
        }
    }
    return SetZoom(zoom_steps_.front());
}

ImageViewer& ImageViewer::FitToWindow() {
    fit_mode_ = true;
    pan_ = ImVec2(0.0f, 0.0f);
    scale_ = fit_scale_;
    ApplyLabels();
    emit zoomChanged(scale_);
    return *this;
}

ImageViewer& ImageViewer::ActualSize() {
    fit_mode_ = false;
    scale_ = 1.0f;
    ClampPan();
    ApplyLabels();
    emit zoomChanged(scale_);
    return *this;
}

ImageViewer& ImageViewer::ResetView() {
    fit_mode_ = false;
    scale_ = 1.0f;
    pan_ = ImVec2(0.0f, 0.0f);
    ApplyLabels();
    emit zoomChanged(scale_);
    return *this;
}

ImageViewer& ImageViewer::PanBy(const ImVec2& delta) {
    pan_.x += delta.x;
    pan_.y += delta.y;
    ClampPan();
    return *this;
}

ImageViewer& ImageViewer::SetZoomSteps(std::vector<float> steps) {
    if (!steps.empty()) {
        std::sort(steps.begin(), steps.end());
        zoom_steps_ = std::move(steps);
    }
    return *this;
}

ImageViewer& ImageViewer::SetCloseCallback(CloseCallback callback) {
    close_callback_ = std::move(callback);
    return *this;
}

ImageViewer& ImageViewer::SetToolbarVisible(bool visible) {
    toolbar_visible_ = visible;
    return *this;
}

ImageViewer& ImageViewer::SetToolbarAutoHide(bool enabled, float idle_seconds) {
    toolbar_autohide_ = enabled;
    toolbar_idle_ = Maxf(idle_seconds, 0.5f);
    toolbar_opacity_ = 1.0f;
    return *this;
}

ImageViewer& ImageViewer::SetShowInfo(bool visible) {
    show_info_ = visible;
    if (info_ != nullptr) {
        info_->visible = visible;
    }
    return *this;
}

ImageViewer& ImageViewer::SetStateIndicators(bool visible) {
    show_state_indicators_ = visible;
    return *this;
}

ImageViewer& ImageViewer::SetSupportedExtensions(std::vector<std::string> extensions) {
    supported_extensions_.clear();
    for (std::string& extension : extensions) {
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!extension.empty() && extension[0] != '.') {
            extension.insert(extension.begin(), '.');
        }
        supported_extensions_.push_back(std::move(extension));
    }
    return *this;
}

bool ImageViewer::ToolbarVisible() const {
    return toolbar_ != nullptr && toolbar_->visible && toolbar_->opacity > 0.05f;
}

void ImageViewer::RecomputeFit(const ImVec2& viewport) {
    viewport_size_ = ImVec2(Maxf(viewport.x, 1.0f), Maxf(viewport.y, 1.0f));
    if (image_.width <= 0 || image_.height <= 0) {
        fit_scale_ = 1.0f;
        return;
    }
    fit_scale_ = Minf(viewport_size_.x / static_cast<float>(image_.width),
                      viewport_size_.y / static_cast<float>(image_.height));
    if (fit_mode_) {
        scale_ = fit_scale_;
    }
    ClampPan();
    // 图片超出可视区的那根轴自己吃掉方向键（用来平移），否则方向键照常去 Toolbar 按钮
    const float display_w = static_cast<float>(image_.width) * scale_;
    const float display_h = static_cast<float>(image_.height) * scale_;
    capture_horizontal = (display_w - viewport_size_.x) > 1.0f;
    capture_vertical = (display_h - viewport_size_.y) > 1.0f;
    ApplyLabels(); // 缩放比例随时更新（Fit 后第一帧就要显示正确百分比）
}

void ImageViewer::ClampPan() {
    if (image_.width <= 0 || image_.height <= 0) {
        pan_ = ImVec2(0.0f, 0.0f);
        return;
    }
    const float display_w = static_cast<float>(image_.width) * scale_;
    const float display_h = static_cast<float>(image_.height) * scale_;
    const float max_x = Maxf((display_w - viewport_size_.x) * 0.5f, 0.0f);
    const float max_y = Maxf((display_h - viewport_size_.y) * 0.5f, 0.0f);
    pan_.x = Clampf(pan_.x, -max_x, max_x);
    pan_.y = Clampf(pan_.y, -max_y, max_y);
}

Rect ImageViewer::ImageRect(const Rect& viewport) const {
    if (image_.width <= 0 || image_.height <= 0) {
        return viewport;
    }
    const float width = static_cast<float>(image_.width) * scale_;
    const float height = static_cast<float>(image_.height) * scale_;
    const ImVec2 center(viewport.Center().x + pan_.x, viewport.Center().y + pan_.y);
    return Rect{ImVec2(center.x - width * 0.5f, center.y - height * 0.5f),
                ImVec2(center.x + width * 0.5f, center.y + height * 0.5f)};
}

void ImageViewer::RevealToolbar() {
    last_interaction_ = Global::time;
    toolbar_opacity_ = 1.0f;
}

// ------------------------------------------------------------ 每帧 -----

void ImageViewer::LayoutChildren(const ImVec2& inner) {
    const float info_height = show_info_ ? kInfoHeight + Theme::kGapSmall : 0.0f;
    const float toolbar_height = Theme::kControlHeight + Theme::kGapSmall;
    const float status_block = 96.0f;
    const float width = Maxf(inner.x, 1.0f);
    const float height = Maxf(inner.y, 1.0f);

    if (info_ != nullptr) {
        info_->visible = show_info_;
        info_->position = ImVec2(0.0f, 0.0f);
        info_->size = ImVec2(width, kInfoHeight);
    }

    // 工具条：一屏宽度内等分（响应式，不写死平台布局）
    if (toolbar_ != nullptr) {
        toolbar_->position = ImVec2(0.0f, height - Theme::kControlHeight);
        toolbar_->size = ImVec2(width, Theme::kControlHeight);
        const int count = 6;
        const float gap = toolbar_->gap.x;
        const float each = Clampf((width - gap * static_cast<float>(count - 1)) / static_cast<float>(count), 56.0f,
                                  160.0f);
        Button* buttons[count] = {fit_button_, actual_button_, zoom_out_button_,
                                  zoom_in_button_, reset_button_, close_button_};
        for (int i = 0; i < count; ++i) {
            if (buttons[i] != nullptr) {
                buttons[i]->resize(each, Theme::kControlHeight);
            }
        }
    }

    // 画布区（图片显示 / 状态界面）
    const float canvas_top = info_height;
    const float canvas_bottom = height - (toolbar_visible_ ? toolbar_height : 0.0f);
    const float canvas_height = Maxf(canvas_bottom - canvas_top, 1.0f);
    canvas_size_ = ImVec2(width, canvas_height);
    if (canvas_ != nullptr) {
        canvas_->position = ImVec2(0.0f, canvas_top);
        canvas_->size = canvas_size_;
    }

    const float block_top = canvas_top + Maxf((canvas_height - status_block) * 0.5f, 0.0f);
    if (status_ != nullptr) {
        status_->position = ImVec2(0.0f, block_top);
        status_->size = ImVec2(width, 34.0f);
    }
    if (status_detail_ != nullptr) {
        status_detail_->position = ImVec2(0.0f, block_top + 34.0f);
        status_detail_->size = ImVec2(width, 22.0f);
    }
    if (status_bar_ != nullptr) {
        status_bar_->position = ImVec2(width * 0.5f - 140.0f, block_top + 62.0f);
        status_bar_->size = ImVec2(280.0f, 0.0f);
        status_bar_->visible = state_ == State::Loading && show_state_indicators_;
    }
    if (error_actions_ != nullptr) {
        const float actions_width = Minf(300.0f, width);
        error_actions_->position = ImVec2(width * 0.5f - actions_width * 0.5f, block_top + 62.0f);
        error_actions_->size = ImVec2(actions_width, Theme::kControlHeight);
        error_actions_->visible = state_ == State::Failed;
        retry_button_->resize(130.0f, Theme::kControlHeight);
        error_close_button_->resize(130.0f, Theme::kControlHeight);
    }

    RecomputeFit(canvas_size_);
}

ImVec2 ImageViewer::MeasureContent(const ImVec2& available) {
    // 组件默认铺满可用区（弹窗内容 / 页面里都合适）；尺寸由调用方给或按可用区撑满。
    const float width = size.x > 0.0f ? size.x : Maxf(available.x, 1.0f);
    const float height = size.y > 0.0f ? size.y : Maxf(available.y, 1.0f);
    const float chrome_x = padding.Horizontal() + (border.width + border.inset) * 2.0f;
    const float chrome_y = padding.Vertical() + (border.width + border.inset) * 2.0f;
    // 子控件位置在这里算：Measure 在本帧 Place 之前，同一帧就生效（本控件不用 Vertical 布局，
    // 因为画布需要"占满剩余高度"，而现有布局没有 flex；位置全部是相对内容区的坐标）。
    LayoutChildren(ImVec2(Maxf(width - chrome_x, 1.0f), Maxf(height - chrome_y, 1.0f)));
    return ImVec2(width, height);
}

void ImageViewer::OnDrawContent(ImDrawList* dl, const Rect& content) {
    // 画布区：图片（透明 PNG 保持 alpha，不填白底）
    const float info_height = show_info_ ? kInfoHeight + Theme::kGapSmall : 0.0f;
    const float toolbar_height = toolbar_visible_ ? Theme::kControlHeight + Theme::kGapSmall : 0.0f;
    const Rect canvas{ImVec2(content.min.x, content.min.y + info_height),
                      ImVec2(content.max.x, Maxf(content.max.y - toolbar_height, content.min.y + info_height + 1.0f))};
    if (!canvas.Valid()) {
        return;
    }
    dl->PushClipRect(canvas.min, canvas.max, true);
    if (state_ == State::Loaded && image_.texture.GetTexID() != ImTextureID_Invalid) {
        const Rect target = ImageRect(canvas);
        dl->AddImage(image_.texture, target.min, target.max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                     Theme::Alpha(Theme::U32(Theme::kWhite), EffectiveOpacity()));
        // 图片贴边时给一圈很淡的描边，避免和背景糊在一起（颜色来自 Theme）
        if (target.min.x <= canvas.min.x + 0.5f || target.min.y <= canvas.min.y + 0.5f ||
            target.max.x >= canvas.max.x - 0.5f || target.max.y >= canvas.max.y - 0.5f) {
            Draw::RoundedRectOutline(dl, canvas.Inset(0.5f, 0.5f, 0.5f, 0.5f),
                                     Theme::Alpha(Theme::U32(Theme::kBorder), EffectiveOpacity() * 0.6f), 1.0f,
                                     Global::component_style.corner_radius, Global::component_style.corner_radius,
                                     Global::component_style.corner_radius, Global::component_style.corner_radius);
        }
    }
    dl->PopClipRect();
}

void ImageViewer::OnUpdate(float dt) {
    LoadIfPending();

    // 手柄：持有方向键连续平移（Pressed 只在边沿触发，长按要自己按 dt 推进）
    if (Global::focused == this && state_ == State::Loaded) {
        const float step = (viewport_size_.x + viewport_size_.y) * 0.5f * dt * 0.9f;
        ImVec2 delta(0.0f, 0.0f);
        if (Global::pad.Held(InputAction::Left)) {
            delta.x += step;
        }
        if (Global::pad.Held(InputAction::Right)) {
            delta.x -= step;
        }
        if (Global::pad.Held(InputAction::Up)) {
            delta.y += step;
        }
        if (Global::pad.Held(InputAction::Down)) {
            delta.y -= step;
        }
        if (delta.x != 0.0f || delta.y != 0.0f) {
            PanBy(delta);
            RevealToolbar();
        }
    }

    // 指针（触摸 / 鼠标共用）：在画布内按下开始拖动 → 平移；同时屏蔽页面级拖动滚动
    const bool inside = Global::mouse_available && content_rect.Contains(Global::mouse);
    if (inside && Global::mouse_pressed[0] && state_ == State::Loaded) {
        pointer_dragging_ = true;
        drag_origin_ = Global::mouse;
        Global::pointer_drag_host = nullptr; // 别让外层滚动容器同时滚
        Global::pointer_dragging = false;
    }
    if (pointer_dragging_) {
        if (Global::mouse_down[0]) {
            if (Global::mouse_delta.x != 0.0f || Global::mouse_delta.y != 0.0f) {
                PanBy(Global::mouse_delta);
                RevealToolbar();
            }
        } else {
            pointer_dragging_ = false;
        }
    }

    // 鼠标滚轮 / 触控板滚动：缩放（以指针位置为基准）
    if (inside && Global::mouse_wheel != 0.0f && state_ == State::Loaded) {
        // 以指针位置为基准缩放（触控板/滚轮都走这里）
        const ImVec2 focal(Global::mouse.x - (content_rect.min.x + canvas_size_.x * 0.5f),
                           Global::mouse.y - (content_rect.min.y + (show_info_ ? kInfoHeight + Theme::kGapSmall : 0.0f) +
                                              canvas_size_.y * 0.5f));
        const float factor = Global::mouse_wheel > 0.0f ? 1.15f : (1.0f / 1.15f);
        SetZoomAt(scale_ * factor, ImVec2(viewport_size_.x * 0.5f + focal.x, viewport_size_.y * 0.5f + focal.y));
        RevealToolbar();
    }

    // 工具条淡出 / 淡入（增强功能，默认关闭；打开后任何交互都会重新显示）
    if (toolbar_autohide_ && toolbar_ != nullptr) {
        const bool idle = (Global::time - last_interaction_) > toolbar_idle_;
        const float target = idle ? 0.0f : 1.0f;
        toolbar_opacity_ = Anim::SmoothTo(toolbar_opacity_, target, 10.0f, dt);
        toolbar_->opacity = toolbar_opacity_;
        toolbar_->interactive = toolbar_opacity_ > 0.5f;
        toolbar_->focus_inert = toolbar_opacity_ <= 0.5f;
        toolbar_->visible = toolbar_opacity_ > 0.02f;
    }
    if (Global::pointer_activity || Global::mouse_wheel != 0.0f) {
        RevealToolbar(); // 任何指针/滚轮操作都让工具条重新出现（点图片也走这里）
    }

    // 按钮可用性：到边界就置灰（复用 Widget::enabled 的禁用视觉）
    if (state_ == State::Loaded) {
        zoom_out_button_->enabled = scale_ > zoom_steps_.front() + 0.001f;
        zoom_in_button_->enabled = scale_ < zoom_steps_.back() - 0.001f;
        actual_button_->enabled = Absf(scale_ - 1.0f) > 0.001f;
        fit_button_->enabled = !fit_mode_;
    }
    if (state_ != State::Loaded && toolbar_ != nullptr) {
        toolbar_->visible = toolbar_visible_;
    }
}

bool ImageViewer::OnPadAction(InputAction action) {
    switch (action) {
    case InputAction::PageLeft:      // L
    case InputAction::TriggerLeft:   // ZL
        ZoomOut();
        RevealToolbar();
        return state_ == State::Loaded;
    case InputAction::PageRight:     // R
    case InputAction::TriggerRight:  // ZR
        ZoomIn();
        RevealToolbar();
        return state_ == State::Loaded;
    case InputAction::ActionX: // X = 重置
        ResetView();
        RevealToolbar();
        return state_ == State::Loaded;
    case InputAction::ActionY: // Y = 适应屏幕
        FitToWindow();
        RevealToolbar();
        return state_ == State::Loaded;
    case InputAction::Confirm: // A = 适应屏幕 / 100% 切换
        if (state_ != State::Loaded) {
            return false;
        }
        if (fit_mode_) {
            ActualSize();
        } else {
            FitToWindow();
        }
        RevealToolbar();
        return true;
    default:
        return false;
    }
}

void ImageViewer::OnThemeChanged() {
    // 颜色都从 Theme 现取，这里只需要让信息行重新排版（字号/文字不变）
    ApplyLabels();
}

} // namespace gui_dev::cv
