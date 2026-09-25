#include "component_view/components/GlassTabs.h"

#include <cmath>

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

// 环形模糊核：中心 1 个 + 内环 4 个 + 外环 8 个（单位是模糊半径的比例）
constexpr int kBlurTapCount = 13;
constexpr float kBlurTaps[kBlurTapCount][2] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},  {-1.0f, 0.0f},  {0.0f, 1.0f},  {0.0f, -1.0f},
    {0.72f, 0.72f}, {-0.72f, 0.72f}, {0.72f, -0.72f}, {-0.72f, -0.72f},
    {0.45f, 0.0f}, {-0.45f, 0.0f}, {0.0f, 0.45f}, {0.0f, -0.45f},
};

struct GlassPalette {
    ImVec4 tint;
    ImVec4 veil;
    ImVec4 rim;
    ImVec4 outer;
    ImVec4 sheen;
    ImVec4 specular;
    ImVec4 capsule;
    ImVec4 capsule_rim;
    ImVec4 text;
    ImVec4 text_active;
    ImVec4 icon;
    ImVec4 icon_active;
};

GlassPalette PaletteFor(bool light) {
    GlassPalette p;
    if (light) {
        p.tint = Theme::rgba(228, 236, 248, 255);
        p.veil = Theme::rgba(255, 255, 255, 255);
        p.rim = Theme::rgba(255, 255, 255, 255);
        p.outer = Theme::rgba(96, 108, 126, 140);
        p.sheen = Theme::rgba(255, 255, 255, 255);
        p.specular = Theme::rgba(255, 255, 255, 255);
        p.capsule = Theme::rgba(255, 255, 255, 255);
        p.capsule_rim = Theme::rgba(255, 255, 255, 255);
        p.text = Theme::rgba(52, 60, 74, 235);
        p.text_active = Theme::rgba(16, 20, 28, 255);
        p.icon = Theme::rgba(72, 82, 98, 235);
        p.icon_active = Theme::rgba(0, 92, 178, 255);
    } else {
        p.tint = Theme::rgba(26, 32, 46, 255);
        p.veil = Theme::rgba(255, 255, 255, 255);
        p.rim = Theme::rgba(255, 255, 255, 255);
        p.outer = Theme::rgba(0, 0, 0, 150);
        p.sheen = Theme::rgba(255, 255, 255, 255);
        p.specular = Theme::rgba(255, 255, 255, 255);
        p.capsule = Theme::rgba(255, 255, 255, 255);
        p.capsule_rim = Theme::rgba(255, 255, 255, 255);
        p.text = Theme::rgba(224, 232, 244, 220);
        p.text_active = Theme::rgba(255, 255, 255, 255);
        p.icon = Theme::rgba(196, 208, 226, 220);
        p.icon_active = Theme::rgba(120, 196, 255, 255);
    }
    return p;
}

} // namespace

GlassTabs::GlassTabs() : Widget("glass_tabs") {
    focusable = true;
    focus_only_self = true;      // 内部自己导航（←/→ 切 Tab）
    capture_horizontal = true;   // ←/→ 不给全局焦点导航，留着切 Tab；↑/↓ 仍然可以离开
    focus_on_hover = true;
    focus_frame = false;         // 玻璃自己就是焦点指示，不套外面的焦点框
    background = 0;
    border.width = 0.0f;
    shadow.enabled = true;       // 浮层给一点阴影，但很淡（见 Style / ApplyComponentBoxStyle）
    shadow.offset = ImVec2(0.0f, 10.0f);
    shadow.blur = 22.0f;
    shadow.color = Theme::U32(Theme::rgba(0, 0, 0, 90));
}

GlassTabs::GlassTabs(std::vector<Item> values) : GlassTabs() {
    setItems(std::move(values));
}

GlassTabs& GlassTabs::setItems(std::vector<Item> values, int start_index) {
    items_ = std::move(values);
    const int item_count = count();
    index_ = item_count <= 0 ? 0 : static_cast<int>(Clampf(static_cast<float>(start_index), 0.0f,
                                                           static_cast<float>(item_count - 1)));
    capsule_x_ = -1.0f;
    return *this;
}

GlassTabs& GlassTabs::setIndex(int value, bool notify) {
    const int item_count = count();
    if (item_count <= 0) {
        return *this;
    }
    const int next = ((value % item_count) + item_count) % item_count;
    if (next == index_) {
        return *this;
    }
    index_ = next;
    if (notify) {
        emit selectionChanged(index_);
    }
    return *this;
}

const char* GlassTabs::currentText() const {
    return items_.empty() ? "" : items_[static_cast<std::size_t>(index_)].text.c_str();
}

GlassTabs& GlassTabs::setBackdrop(ImTextureRef texture, const Rect& canvas_rect) {
    backdrop_ = texture;
    backdrop_rect_ = canvas_rect;
    return *this;
}

ImVec2 GlassTabs::MeasureContent(const ImVec2& available) {
    (void)available;
    const float width = static_cast<float>(Maxf(count(), 1)) * 120.0f;
    return ImVec2(width, style.height);
}

Rect GlassTabs::ItemRect(int item) const {
    const Rect body = DrawContentRect();
    const int item_count = Maxf(count(), 1);
    const float inner_w = Maxf(body.Width() - style.padding * 2.0f, 1.0f);
    const float item_w =
        (inner_w - style.item_gap * static_cast<float>(item_count - 1)) / static_cast<float>(item_count);
    const float x = body.min.x + style.padding + static_cast<float>(item) * (item_w + style.item_gap);
    return Rect::FromPosSize(ImVec2(x, body.min.y + style.padding),
                             ImVec2(item_w, Maxf(body.Height() - style.padding * 2.0f, 1.0f)));
}

int GlassTabs::ItemAtX(float x) const {
    for (int i = 0; i < count(); ++i) {
        if (ItemRect(i).Contains(ImVec2(x, ItemRect(i).Center().y))) {
            return i;
        }
    }
    return -1;
}

void GlassTabs::DrawBackdropSample(ImDrawList* dl, const Rect& dst, const ImVec2& uv_min, const ImVec2& uv_max,
                                   float radius, ImU32 color) const {
    if (dl == nullptr || !dst.Valid() || color == 0) {
        return;
    }
    dl->AddImageRounded(backdrop_, dst.min, dst.max, uv_min, uv_max, color, radius);
}

void GlassTabs::DrawGlass(ImDrawList* dl, const Rect& body) const {
    const float opacity = EffectiveOpacity();
    if (opacity <= 0.002f || !body.Valid()) {
        return;
    }
    const GlassPalette palette = PaletteFor(Theme::IsLight());
    const float r = Minf(style.corner_radius, Minf(body.Width(), body.Height()) * 0.5f);
    const bool sample_backdrop = hasBackdrop();

    // 玻璃覆盖区域在背景纹理里的基础 UV（把画布坐标映射成纹理 UV）
    ImVec2 uv_min(0.0f, 0.0f);
    ImVec2 uv_max(1.0f, 1.0f);
    if (sample_backdrop) {
        uv_min = ImVec2((body.min.x - backdrop_rect_.min.x) / backdrop_rect_.Width(),
                        (body.min.y - backdrop_rect_.min.y) / backdrop_rect_.Height());
        uv_max = ImVec2((body.max.x - backdrop_rect_.min.x) / backdrop_rect_.Width(),
                        (body.max.y - backdrop_rect_.min.y) / backdrop_rect_.Height());
    }

    const ImVec2 uv_center((uv_min.x + uv_max.x) * 0.5f, (uv_min.y + uv_max.y) * 0.5f);

    // ---- 1) 模糊：13 抽头环形核，每个抽头就是一次带偏移的贴图采样 ----
    if (sample_backdrop) {
        const int taps = Maxf(Minf(style.blur_taps, kBlurTapCount), 1);
        const float radius = Maxf(style.blur_radius, 0.0f);
        const ImU32 tap_color = Theme::U32(Theme::rgba(255, 255, 255, 255), opacity * style.blur_alpha /
                                                                               static_cast<float>(taps));
        for (int i = 0; i < taps; ++i) {
            const ImVec2 tap(kBlurTaps[i][0] * radius, kBlurTaps[i][1] * radius);
            const ImVec2 offset_uv(tap.x / backdrop_rect_.Width(), tap.y / backdrop_rect_.Height());
            DrawBackdropSample(dl, body, ImVec2(uv_min.x + offset_uv.x, uv_min.y + offset_uv.y),
                               ImVec2(uv_max.x + offset_uv.x, uv_max.y + offset_uv.y), r, tap_color);
        }
    } else {
        // 没有背景纹理：只画材质底（在深色内容上仍然像一块玻璃）
        Draw::RoundedRectFilled(dl, body, Theme::U32(palette.tint, opacity * 0.5f), r, r, r, r);
    }

    // ---- 2) 折射：从最外圈往里画「UV 放大」的采样，越靠边放大越多、权重越低 ----
    //     多圈叠加后，边缘的背景被掰弯、中心保持原样 —— 透镜感的来源。
    if (sample_backdrop && style.lens_bands > 0) {
        const int bands = Minf(style.lens_bands, 8);
        for (int b = 0; b < bands; ++b) {
            const float inset = static_cast<float>(b) * style.lens_inset;
            const Rect band = body.Inset(inset, inset, inset, inset);
            if (!band.Valid()) {
                break;
            }
            const float zoom = 1.0f + style.lens_scale * static_cast<float>(bands - b);
            const ImVec2 b_uv_min((uv_min.x - uv_center.x) / zoom + uv_center.x, (uv_min.y - uv_center.y) / zoom + uv_center.y);
            const ImVec2 b_uv_max((uv_max.x - uv_center.x) / zoom + uv_center.x, (uv_max.y - uv_center.y) / zoom + uv_center.y);
            // 权重：越往外越强，但整体很低，只是把边缘"掰"一点
            const float weight = style.lens_alpha / static_cast<float>(bands) *
                                 (1.0f + static_cast<float>(b));
            const float fr = Maxf(r - inset, 0.0f);
            dl->PushClipRect(band.min, band.max, true);
            DrawBackdropSample(dl, body, b_uv_min, b_uv_max, r,
                               Theme::U32(Theme::rgba(255, 255, 255, 255), opacity * weight * 0.35f));
            dl->PopClipRect();
            (void)fr;
        }
    }

    // ---- 3) 材质：染色 + 白雾（提亮/降饱和的观感）----
    Draw::RoundedRectFilled(dl, body, Theme::U32(palette.tint, opacity * style.tint_alpha), r, r, r, r);
    Draw::RoundedRectFilled(dl, body, Theme::U32(palette.veil, opacity * style.veil_alpha), r, r, r, r);

    // ---- 4) 顶部高光 + 镜面光斑（带拖动晃动）----
    const float band[3] = {0.52f, 0.30f, 0.14f};
    const float band_alpha[3] = {0.36f, 0.34f, 0.30f};
    for (int i = 0; i < 3; ++i) {
        const Rect sheen = Rect::FromPosSize(body.min, ImVec2(body.Width(), body.Height() * band[i]));
        Draw::RoundedRectFilled(dl, sheen, Theme::U32(palette.sheen, opacity * style.sheen_alpha * band_alpha[i]), r,
                                r, r, r);
    }
    const float wander = std::sin(phase_) * 8.0f;
    const ImVec2 spot(body.min.x + body.Width() * 0.22f + slosh_.x + wander,
                      body.min.y + body.Height() * 0.30f + slosh_.y);
    for (int i = 0; i < 5; ++i) {
        const float t = static_cast<float>(i) / 4.0f;
        const ImVec2 radius(body.Width() * 0.22f * (1.0f - t * 0.6f), body.Height() * 0.42f * (1.0f - t * 0.6f));
        const float a = style.specular_alpha * (0.14f + 0.18f * (1.0f - t));
        dl->AddEllipseFilled(spot, radius, Theme::U32(palette.specular, opacity * a));
    }

    // ---- 5) 边缘：外暗边 + 内亮边（很细，Apple 那套的边缘高光）----
    Draw::RoundedRectOutline(dl, body, Theme::U32(palette.outer, opacity * 0.85f), 1.0f, r, r, r, r);
    const float rr = Maxf(r - 1.0f, 0.0f);
    Draw::RoundedRectOutline(dl, body.Inset(1.0f, 1.0f, 1.0f, 1.0f),
                             Theme::U32(palette.rim, opacity * style.rim_alpha), style.rim_width, rr, rr, rr, rr);
}

void GlassTabs::OnDrawContent(ImDrawList* dl, const Rect& content) {
    (void)content;
    DrawGlass(dl, DrawRect());
}

void GlassTabs::OnDrawOverlay(ImDrawList* dl, const Rect& content) {
    (void)content;
    if (dl == nullptr || items_.empty()) {
        return;
    }
    const float opacity = EffectiveOpacity();
    const GlassPalette palette = PaletteFor(Theme::IsLight());
    const Rect body = DrawRect();

    // 选中胶囊：默认直接切（不滑动），需要的话把 style.capsule_slide 设 >0
    const Rect selected = ItemRect(index_);
    const Rect target = selected.Inset(2.0f, 0.0f, 2.0f, 0.0f);
    float pill_x = target.min.x;
    float pill_w = target.Width();
    if (style.capsule_slide > 0.0f) {
        if (capsule_x_ < 0.0f) {
            capsule_x_ = pill_x;
            capsule_w_ = pill_w;
        }
        const float speed = 1.0f / Maxf(style.capsule_slide, 0.01f) * 6.0f;
        capsule_x_ = Anim::SmoothTo(capsule_x_, pill_x, speed, Global::delta_time);
        capsule_w_ = Anim::SmoothTo(capsule_w_, pill_w, speed, Global::delta_time);
        pill_x = capsule_x_;
        pill_w = capsule_w_;
    }
    const Rect pill = Rect{ImVec2(pill_x, target.min.y), ImVec2(pill_x + pill_w, target.max.y)};
    const float pill_r = Minf(style.item_radius, pill.Height() * 0.5f);
    Draw::RoundedRectFilled(dl, pill, Theme::U32(palette.capsule, opacity * style.capsule_alpha), pill_r, pill_r,
                            pill_r, pill_r);
    Draw::RoundedRectOutline(dl, pill, Theme::U32(palette.capsule_rim, opacity * 0.28f), 1.0f, pill_r, pill_r, pill_r,
                             pill_r);

    // 图标 + 文字
    for (int i = 0; i < count(); ++i) {
        const Rect item = ItemRect(i);
        const bool active = (i == index_);
        const ImVec4 icon_color = active ? palette.icon_active : palette.icon;
        const ImVec4 text_color = active ? palette.text_active : palette.text;

        if (!items_[static_cast<std::size_t>(i)].icon.empty()) {
            const ImVec2 extent = Draw::MeasureText(nullptr, style.icon_size,
                                                    items_[static_cast<std::size_t>(i)].icon.c_str(), 0.0f);
            float ink_top = 0.0f;
            float ink_bottom = 0.0f;
            float y = item.min.y + item.Height() * 0.36f - extent.y * 0.5f;
            if (Draw::GlyphInkExtent(nullptr, style.icon_size, items_[static_cast<std::size_t>(i)].icon.c_str(),
                                     ink_top, ink_bottom)) {
                y = item.min.y + item.Height() * 0.36f - (ink_top + ink_bottom) * 0.5f;
            }
            Draw::Text(dl, nullptr, style.icon_size,
                       ImVec2(item.Center().x - extent.x * 0.5f, y),
                       Theme::U32(icon_color, opacity), items_[static_cast<std::size_t>(i)].icon.c_str());
        }
        if (!items_[static_cast<std::size_t>(i)].text.empty()) {
            const ImVec2 extent = Draw::MeasureText(nullptr, style.text_size,
                                                    items_[static_cast<std::size_t>(i)].text.c_str(), 0.0f);
            Draw::Text(dl, nullptr, style.text_size,
                       ImVec2(item.Center().x - extent.x * 0.5f, item.min.y + item.Height() * 0.72f - extent.y * 0.5f),
                       Theme::U32(text_color, opacity), items_[static_cast<std::size_t>(i)].text.c_str());
        }
    }
    (void)body;
}

void GlassTabs::OnUpdate(float dt) {
    phase_ += dt * 0.6f;

    // 拖动（鼠标 / 触摸走同一条指针路径）
    if (draggable && Global::mouse_available) {
        if (down) {
            const ImVec2 mouse = Global::mouse;
            if (!dragging_) {
                dragging_ = true;
                velocity_ = ImVec2(0.0f, 0.0f);
            } else {
                const ImVec2 delta(mouse.x - drag_last_.x, mouse.y - drag_last_.y);
                if (delta.x != 0.0f || delta.y != 0.0f) {
                    const ImVec2 canvas = Global::canvas_size;
                    position.x = Clampf(position.x + delta.x, 0.0f, Maxf(canvas.x - rect.Width(), 0.0f));
                    position.y = Clampf(position.y + delta.y, 0.0f, Maxf(canvas.y - rect.Height(), 0.0f));
                    velocity_ = delta;
                    emit movedTo(position);
                }
            }
            drag_last_ = mouse;
        } else {
            dragging_ = false;
        }
    }

    const ImVec2 slosh_target(Clampf(-velocity_.x * 0.5f, -12.0f, 12.0f), Clampf(-velocity_.y * 0.5f, -12.0f, 12.0f));
    slosh_.x = Anim::SmoothTo(slosh_.x, slosh_target.x, 7.5f, dt);
    slosh_.y = Anim::SmoothTo(slosh_.y, slosh_target.y, 7.5f, dt);
    if (!dragging_ && dt > 0.0f) {
        velocity_.x = Anim::SmoothTo(velocity_.x, 0.0f, 16.0f, dt);
        velocity_.y = Anim::SmoothTo(velocity_.y, 0.0f, 16.0f, dt);
    }
}

void GlassTabs::Activate() {
    if (Global::mouse_available && Global::hovered == this) {
        const int hit = ItemAtX(Global::mouse.x);
        if (hit >= 0) {
            setIndex(hit);
            return;
        }
    }
    setIndex(index_); // 键盘/手柄确认：先什么都不做（切 Tab 用 ←/→）
}

bool GlassTabs::OnPadAction(InputAction action) {
    if (action == InputAction::Left || action == InputAction::PageLeft) {
        setIndex(index_ - 1);
        return true;
    }
    if (action == InputAction::Right || action == InputAction::PageRight) {
        setIndex(index_ + 1);
        return true;
    }
    if (action == InputAction::Confirm) {
        if (Global::mouse_available && Global::hovered == this) {
            const int hit = ItemAtX(Global::mouse.x);
            if (hit >= 0) {
                setIndex(hit);
                return true;
            }
        }
        return false;
    }
    return false;
}

} // namespace gui_dev::cv
