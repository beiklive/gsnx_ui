#include "component_view/Toast.h"

#include <algorithm>
#include <utility>

#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/Theme.h"
#include "component_view/Anim.h" // 组件库自己的 easing / 指数趋近
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

namespace anim = gui_dev::cv::Anim;

// 状态 → 图标：全部走项目现有的 Material Icons 字体，不引第二套图标库
gui_dev::Icons::Material ToastIcon(ToastType type) {
    switch (type) {
    case ToastType::Success:
        return gui_dev::Icons::Material::CheckCircle;
    case ToastType::Error:
        return gui_dev::Icons::Material::ErrorOutline;
    case ToastType::Info:
        break;
    }
    return gui_dev::Icons::Material::Info;
}

float ToastTextSize(const ToastStyle& style) {
    return style.text_size > 0.0f ? style.text_size : Theme::kFontBody;
}

// 一个 Toast 内容的水平占位：色条 + 间距 + 图标 + 间距 + 右侧留白
float ToastChromeWidth(const ToastStyle& style) {
    return style.bar_inset * 2.0f + style.bar_width + style.gap + style.icon_size + style.gap + style.padding_x;
}

} // namespace

ImU32 ToastAccentColor(ToastType type) {
    switch (type) {
    case ToastType::Success:
        return Theme::U32(Theme::kSuccess); // 绿
    case ToastType::Error:
        return Theme::U32(Theme::kError); // 红
    case ToastType::Info:
        break;
    }
    return Theme::U32(Theme::kAccent); // 蓝
}

void ToastManager::Measure(Toast& toast) const {
    const float text_size = ToastTextSize(style_);
    const float chrome = ToastChromeWidth(style_);
    const ImVec2 natural = Draw::MeasureText(nullptr, text_size, toast.message.c_str(), 0.0f);

    float width = style_.min_width;
    float wrap_width = 0.0f; // 0 = 不换行
    const float available = Maxf(style_.max_width - chrome, 40.0f);
    if (natural.x > available) {
        width = style_.max_width; // 长文本：撑到最大宽度再换行
        wrap_width = available;
    } else {
        width = Clampf(natural.x + chrome, style_.min_width, style_.max_width);
    }
    const ImVec2 text_extent = Draw::MeasureText(nullptr, text_size, toast.message.c_str(), wrap_width);
    toast.width = width;
    toast.height = Maxf(style_.min_height, text_extent.y + style_.padding_y * 2.0f);
}

void ToastManager::Show(ToastType type, std::string message) {
    if (message.empty()) {
        return;
    }
    // 去重：同类型 + 同文案在短窗口内重复出现 → 只刷新停留时间，不堆一屏一样的通知
    for (Toast& toast : toasts_) {
        if (toast.type == type && toast.message == message && toast.elapsed <= style_.dedup_window) {
            toast.elapsed = 0.0f;
            toast.state_time = 0.0f;
            if (toast.state == ToastState::Exiting) {
                toast.state = ToastState::Entering; // 正在退出就让它再滑回来
            } else {
                toast.state = ToastState::Visible;
                toast.slide = 1.0f;
            }
            return;
        }
    }

    Toast toast;
    toast.type = type;
    toast.message = std::move(message);
    toast.state = ToastState::Entering;
    Measure(toast);
    toasts_.push_back(std::move(toast));
}

void ToastManager::Update(float dt) {
    if (toasts_.empty()) {
        return;
    }
    dt = Maxf(dt, 0.0f);

    // 1) 生命周期：Entering → Visible（3s 从这里开始算）→ Exiting → 删除
    for (Toast& toast : toasts_) {
        toast.elapsed += dt;
        toast.state_time += dt;
        if (toast.state == ToastState::Entering && toast.state_time >= style_.enter_duration) {
            toast.state = ToastState::Visible;
            toast.state_time = 0.0f;
        } else if (toast.state == ToastState::Visible && toast.state_time >= style_.visible_duration) {
            toast.state = ToastState::Exiting;
            toast.state_time = 0.0f;
        }
    }
    toasts_.erase(std::remove_if(toasts_.begin(), toasts_.end(),
                                 [this](const Toast& toast) {
                                     return toast.state == ToastState::Exiting &&
                                            toast.state_time >= style_.exit_duration;
                                 }),
                  toasts_.end());
    if (toasts_.empty()) {
        return;
    }

    // 2) 每帧重算队列目标位置：前一个消失，后面的 targetY 自动上移（不存死坐标）
    float y = style_.top_margin;
    for (Toast& toast : toasts_) {
        toast.target_y = y;
        y += toast.height + style_.spacing;
    }

    // 3) 动画：X 用「定时长 + EaseOutCubic」，Y 用指数趋近 —— 两套互不影响
    for (Toast& toast : toasts_) {
        switch (toast.state) {
        case ToastState::Entering: {
            const float t = anim::Clamp01(toast.state_time / Maxf(style_.enter_duration, 0.001f));
            toast.slide = anim::EaseOutCubic(t);
            break;
        }
        case ToastState::Visible:
            toast.slide = 1.0f;
            break;
        case ToastState::Exiting: {
            const float t = anim::Clamp01(toast.state_time / Maxf(style_.exit_duration, 0.001f));
            toast.slide = 1.0f - anim::EaseOutCubic(t);
            break;
        }
        }
        if (!toast.placed) {
            toast.current_y = toast.target_y; // 新 Toast 直接落在自己的槽位，不从 0 飞下来
            toast.placed = true;
        } else {
            toast.current_y = anim::SmoothTo(toast.current_y, toast.target_y, style_.reflow_speed, dt);
        }
    }

}

void ToastManager::Draw(ImDrawList* dl) {
    if (dl == nullptr || toasts_.empty()) {
        return;
    }
    const float display_w = ImGui::GetIO().DisplaySize.x > 0.0f ? ImGui::GetIO().DisplaySize.x : Global::canvas_size.x;
    const float rest_x = display_w - style_.right_margin; // 停靠位置：右边缘贴这里
    const float text_size = ToastTextSize(style_);

    for (const Toast& toast : toasts_) {
        // X：slide=0 时整个 Box 在屏幕右边外，slide=1 时右边缘贴 margin
        const float parked_x = rest_x - toast.width; // 停靠位置（左边缘）
        const float offscreen_x = display_w;         // 完全滑出屏幕
        const float left = offscreen_x + (parked_x - offscreen_x) * toast.slide;
        const Rect box = Rect::FromPosSize(ImVec2(left, toast.current_y), ImVec2(toast.width, toast.height));

        // 1) 和 Button 一模一样的框（同一套 Global::component_style + Draw::ComponentBox）：
        //    左侧两角 5px 圆角、右侧两角直角
        BoxVisual visual = Global::ComponentBoxVisual();
        visual.background = Theme::U32(Theme::kBgWidget);
        const float left_radius = style_.left_radius;
        visual.tl = left_radius;
        visual.bl = left_radius;
        visual.tr = 0.0f;
        visual.br = 0.0f;
        Draw::ComponentBox(dl, box, visual);

        // 2) 左侧状态色条：圆角长条（bar_radius），左/上/下都离边框 style_.bar_inset
        const float inset = style_.bar_inset;
        const Rect bar = Rect::FromPosSize(ImVec2(box.min.x + inset, box.min.y + inset),
                                           ImVec2(style_.bar_width, box.Height() - inset * 2.0f));
        Draw::RoundedRectFilled(dl, bar, ToastAccentColor(toast.type), style_.bar_radius, style_.bar_radius,
                                style_.bar_radius, style_.bar_radius);

        // 3) Material 图标（现成的字体图标，不引第二套）
        const float icon_x = bar.max.x + style_.gap;
        const float icon_size = style_.icon_size;
        const char* glyph = gui_dev::Icons::Glyph(ToastIcon(toast.type));
        const ImVec2 icon_extent = Draw::MeasureText(nullptr, icon_size, glyph, 0.0f);
        float icon_y = box.Center().y - icon_extent.y * 0.5f;
        float ink_top = 0.0f;
        float ink_bottom = 0.0f;
        if (Draw::GlyphInkExtent(nullptr, icon_size, glyph, ink_top, ink_bottom)) {
            icon_y = box.Center().y - (ink_top + ink_bottom) * 0.5f; // 按墨迹居中，和按钮图标一致
        }
        Draw::Text(dl, nullptr, icon_size, ImVec2(icon_x, icon_y), ToastAccentColor(toast.type), glyph);

        // 4) 文本：在剩余宽度里换行 + 垂直居中
        const float text_x = icon_x + style_.icon_size + style_.gap;
        const float text_max = Maxf(box.max.x - style_.padding_x - text_x, 20.0f);
        const ImVec2 text_extent = Draw::MeasureText(nullptr, text_size, toast.message.c_str(), text_max);
        Draw::Text(dl, nullptr, text_size,
                   ImVec2(text_x, box.Center().y - text_extent.y * 0.5f), Theme::U32(Theme::kTextPrimary),
                   toast.message.c_str(), text_max);
    }
}

} // namespace gui_dev::cv
