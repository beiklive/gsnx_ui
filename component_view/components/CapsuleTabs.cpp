#include "component_view/components/CapsuleTabs.h"

#include <cmath>

#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

// 与 GBAStation 一致的缓出：1 - (1 - t)^3
float EaseOutCubic(float t) {
    const float inv = 1.0f - Clampf(t, 0.0f, 1.0f);
    return 1.0f - inv * inv * inv;
}

} // namespace

CapsuleTabs::CapsuleTabs() : Widget("capsule_tabs") {
    focusable = true;
    focus_on_hover = true;
    focus_frame = false; // 胶囊本身就是焦点指示（聚焦时描边加亮并偏向强调色）
    overflow = Overflow::Visible;
    background = 0; // 不画自己的底色/边框，只有胶囊
}

CapsuleTabs::CapsuleTabs(std::vector<std::string> values) : CapsuleTabs() {
    setLabels(std::move(values));
}

CapsuleTabs& CapsuleTabs::setLabels(std::vector<std::string> values, int start_index) {
    labels = std::move(values);
    const int count = static_cast<int>(labels.size());
    index = count <= 0 ? 0 : static_cast<int>(Clampf(static_cast<float>(start_index), 0.0f,
                                                     static_cast<float>(count - 1)));
    slide_ = 1.0f; // 换数据不播动画，直接归位
    slide_dir_ = 0;
    return *this;
}

CapsuleTabs& CapsuleTabs::setIndex(int value, bool notify) {
    const int count = static_cast<int>(labels.size());
    if (count <= 0) {
        return *this;
    }
    int next = value;
    if (wrap) {
        next = ((value % count) + count) % count;
    } else {
        next = value < 0 ? 0 : (value >= count ? count - 1 : value);
    }
    if (next == index) {
        return *this;
    }

    // 方向：能绕回时按环形最短路径走（同 GBAStation），否则就是单纯的左 / 右
    if (wrap) {
        const int forward = (next - index + count) % count;
        const int backward = (index - next + count) % count;
        slide_dir_ = forward <= backward ? 1 : -1;
    } else {
        slide_dir_ = next > index ? 1 : -1;
    }
    index = next;
    slide_ = 0.0f; // 从相邻格滑进来
    if (notify) {
        emit selectionChanged(index);
    }
    return *this;
}

const char* CapsuleTabs::currentLabel() const {
    return labels.empty() ? "" : labels[static_cast<std::size_t>(index)].c_str();
}

float CapsuleTabs::SlotOffset() const {
    return static_cast<float>(slide_dir_) * (1.0f - EaseOutCubic(slide_));
}

float CapsuleTabs::LabelCenterX(float center_x, int item_index) const {
    const float scale = DrawScale();
    return center_x + (static_cast<float>(item_index - index) + SlotOffset()) * style.spacing * scale;
}

int CapsuleTabs::ItemAtX(float x) const {
    if (labels.empty()) {
        return -1;
    }
    const float scale = DrawScale();
    const float center_x = DrawContentRect().Center().x;
    const float reach = style.spacing * scale * 0.5f;
    int best = -1;
    float best_distance = reach;
    for (int i = 0; i < count(); ++i) {
        const float distance = Absf(x - LabelCenterX(center_x, i));
        if (distance <= best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

ImVec2 CapsuleTabs::MeasureContent(const ImVec2& available) {
    if (labels.empty()) {
        return ImVec2(0.0f, style.capsule_height);
    }
    // 高度：胶囊 + 阴影往下的余量（避免阴影被算到条带外面）
    const float height = style.capsule_height + style.shadow_offset.y + style.shadow_blur;
    // 宽度：不显式给尺寸时，按「所有标签摊开」算，再夹到父给的空间
    const float spread = style.spacing * static_cast<float>(count() - 1) + style.capsule_width;
    const float width = available.x > 0.0f ? Minf(spread, available.x) : spread;
    return ImVec2(width, height);
}

void CapsuleTabs::OnUpdate(float dt) {
    if (dt > 0.0f && slide_ < 1.0f) {
        slide_ = Minf(1.0f, slide_ + dt * style.slide_speed);
    }
}

void CapsuleTabs::Activate() {
    if (labels.empty()) {
        return;
    }
    // 触摸 / 鼠标：选中点到的那个标签；手柄 A：对当前项发 activated
    if (Global::mouse_available && Global::hovered == this) {
        const int hit = ItemAtX(Global::mouse.x);
        if (hit >= 0) {
            if (hit == index) {
                emit activated(index);
            } else {
                setIndex(hit);
            }
            return;
        }
    }
    emit activated(index);
}

bool CapsuleTabs::OnPadAction(InputAction action) {
    if (action == InputAction::PageLeft) {
        const int next = index - 1;
        if (next < 0 && !wrap) {
            return false; // 到头了不消费，留给页面级快捷键
        }
        setIndex(next);
        return true;
    }
    if (action == InputAction::PageRight) {
        const int next = index + 1;
        if (next >= count() && !wrap) {
            return false;
        }
        setIndex(next);
        return true;
    }
    return false;
}

void CapsuleTabs::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (dl == nullptr || labels.empty()) {
        return;
    }
    const float scale = DrawScale();
    const float spacing = style.spacing * scale;
    const float capsule_w = style.capsule_width * scale;
    const float capsule_h = style.capsule_height * scale;
    const float radius = capsule_h * 0.5f; // 高的一半 = 完全胶囊形
    const float offset_slots = SlotOffset();
    const ImVec2 center = content.Center();

    // ---- 1) 胶囊：跟着「选中项」滑，离中心越近越实（prominence 线性插值）----
    // 注意画在标签之前：胶囊是半透明的，后画的文字才不会被蒙上一层。
    const float prominence = Maxf(0.0f, 1.0f - Absf(offset_slots));
    if (prominence > 0.02f) {
        const float capsule_x = center.x + offset_slots * spacing;
        const Rect capsule =
            Rect::FromPosSize(ImVec2(capsule_x - capsule_w * 0.5f, center.y - capsule_h * 0.5f),
                              ImVec2(capsule_w, capsule_h));

        ShadowStyle shadow;
        shadow.enabled = true;
        shadow.offset = ImVec2(style.shadow_offset.x * scale, style.shadow_offset.y * scale);
        shadow.blur = style.shadow_blur * scale;
        shadow.color = Theme::U32(Theme::kCapsuleShadow, prominence);
        Draw::SoftShadow(dl, capsule, shadow, radius, radius, radius, radius);

        const float fill_alpha = style.fill_alpha + (style.fill_alpha_max - style.fill_alpha) * prominence +
                                 focus_mix * 0.06f;
        Draw::RoundedRectFilled(dl, capsule, Theme::U32(Theme::kCapsuleFill, fill_alpha), radius, radius, radius,
                                radius);

        const float stroke_alpha = style.stroke_alpha + (style.stroke_alpha_max - style.stroke_alpha) * prominence +
                                   focus_mix * 0.25f;
        // 聚焦时描边往强调色偏，胶囊本身就是焦点指示（基类焦点框已关掉）
        const ImU32 stroke_color = Theme::Mix(Theme::U32(Theme::kCapsuleStroke), Theme::U32(Theme::kAccent),
                                              focus_mix * 0.85f);
        Draw::RoundedRectOutline(dl, capsule, Theme::Alpha(stroke_color, Minf(stroke_alpha, 1.0f)),
                                 Maxf(scale, 1.0f), radius, radius, radius, radius);
    }

    // ---- 2) 标签：字号 / 透明度都按离中心的距离衰减 ----
    for (int i = 0; i < count(); ++i) {
        const float relative = static_cast<float>(i - index) + offset_slots;
        const float distance = Absf(relative);
        if (distance > style.fade_span) {
            continue; // 太远的直接不画（GBAStation 也是这么裁的，聚在中心附近）
        }
        const float item_prominence = Maxf(0.0f, 1.0f - distance);
        const float font_size = (style.font_min + (style.font_max - style.font_min) * item_prominence) * scale;
        const float alpha = (style.alpha_min + (1.0f - style.alpha_min) * item_prominence) * EffectiveOpacity();
        const char* text = labels[static_cast<std::size_t>(i)].c_str();
        const ImVec2 extent = Draw::MeasureText(nullptr, font_size, text, 0.0f);
        Draw::Text(dl, nullptr, font_size,
                   ImVec2(center.x + relative * spacing - extent.x * 0.5f, center.y - extent.y * 0.5f),
                   Theme::U32(Theme::kTextPrimary, alpha), text);
    }
}

} // namespace gui_dev::cv
