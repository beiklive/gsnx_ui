#include "component_view/components/GlassBox.h"

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"

namespace gui_dev::cv {
namespace {

// 玻璃的几层颜色：深色主题偏冷白（在深底上像厚玻璃），浅色主题偏乳白（在浅底上像磨砂亚克力）
struct GlassPalette {
    ImVec4 veil;
    ImVec4 body;
    ImVec4 sheen;
    ImVec4 highlight;
    ImVec4 rim;
    ImVec4 outer;
    ImVec4 fog;
    ImVec4 text;
    ImVec4 hint;
};

GlassPalette PaletteFor(bool light) {
    GlassPalette p;
    if (light) {
        p.veil = Theme::rgba(255, 255, 255, 120);
        p.body = Theme::rgba(214, 226, 245, 90);
        p.sheen = Theme::rgba(255, 255, 255, 190);
        p.highlight = Theme::rgba(255, 255, 255, 230);
        p.rim = Theme::rgba(255, 255, 255, 235);
        p.outer = Theme::rgba(120, 132, 150, 70);
        p.fog = Theme::rgba(255, 255, 255, 120);
        p.text = Theme::rgba(40, 46, 56, 255);
        p.hint = Theme::rgba(60, 68, 80, 200);
    } else {
        p.veil = Theme::rgba(150, 180, 225, 60);
        p.body = Theme::rgba(255, 255, 255, 26);
        p.sheen = Theme::rgba(255, 255, 255, 120);
        p.highlight = Theme::rgba(255, 255, 255, 130);
        p.rim = Theme::rgba(255, 255, 255, 200);
        p.outer = Theme::rgba(0, 0, 0, 120);
        p.fog = Theme::rgba(200, 220, 255, 90);
        p.text = Theme::rgba(238, 243, 249, 240);
        p.hint = Theme::rgba(226, 234, 245, 190);
    }
    return p;
}

} // namespace

GlassBox::GlassBox() : Box("glass_box") {
    draggable = true;
    focusable = false; // 玻璃浮层不参与手柄焦点，免得抢 tab / 内容区的导航
    // 材质由 OnDrawContent 自己画，所以这里只保留阴影，底/边框留空
    background = 0;
    background_follows_theme = false;
    border.width = 0.0f;
}

GlassBox::GlassBox(std::string value) : GlassBox() {
    label = std::move(value);
}

GlassBox& GlassBox::setLabel(std::string value) {
    label = std::move(value);
    return *this;
}

GlassBox& GlassBox::setHint(std::string value) {
    hint = std::move(value);
    return *this;
}

void GlassBox::OnThemeChanged() {
    Box::OnThemeChanged();
    // Box::OnThemeChanged 会调 applyComponentStyle()，那里会给「空底色」补上 kBgWidget，
    // 玻璃必须保持透明底，这里复位。
    background = 0;
    background_follows_theme = false;
    border.width = 0.0f;
}

ImVec2 GlassBox::MeasureContent(const ImVec2& available) {
    (void)available;
    return ImVec2(280.0f, 180.0f); // 没显式给尺寸时的默认大小
}

void GlassBox::OnUpdate(float dt) {
    if (!draggable) {
        return;
    }
    const float speed = style.slosh_speed;

    if (down && Global::mouse_available) { // down = 按下时锁在本控件（Widget::UpdateInteraction 维护）
        const ImVec2 mouse = Global::mouse;
        if (!dragging_) {
            dragging_ = true;
            velocity_ = ImVec2(0.0f, 0.0f);
        } else {
            const ImVec2 delta(mouse.x - drag_last_.x, mouse.y - drag_last_.y);
            if (delta.x != 0.0f || delta.y != 0.0f) {
                // 位置是相对父内容区的，直接改 position；同时夹在画布里，别拖丢了
                const ImVec2 canvas = Global::canvas_size;
                position.x = Clampf(position.x + delta.x, 0.0f, Maxf(canvas.x - rect.Width(), 0.0f));
                position.y = Clampf(position.y + delta.y, 0.0f, Maxf(canvas.y - rect.Height(), 0.0f));
                velocity_ = delta;
                emit movedTo(position);
            }
        }
        drag_last_ = mouse;
    } else if (dragging_) {
        dragging_ = false; // 松手：速度开始衰减，高光回正
    }

    // 液面晃动：把（衰减中的）拖动速度转成高光偏移，再指数回正
    const ImVec2 target(Clampf(-velocity_.x * style.drag_gain, -style.slosh_max, style.slosh_max),
                        Clampf(-velocity_.y * style.drag_gain, -style.slosh_max, style.slosh_max));
    slosh_.x = Anim::SmoothTo(slosh_.x, target.x, speed, dt);
    slosh_.y = Anim::SmoothTo(slosh_.y, target.y, speed, dt);
    if (dt > 0.0f && !dragging_) {
        velocity_.x = Anim::SmoothTo(velocity_.x, 0.0f, 16.0f, dt);
        velocity_.y = Anim::SmoothTo(velocity_.y, 0.0f, 16.0f, dt);
    }
}

void GlassBox::DrawMaterial(ImDrawList* dl, const Rect& body) const {
    const float opacity = EffectiveOpacity();
    if (opacity <= 0.002f || !body.Valid()) {
        return;
    }
    const GlassPalette palette = PaletteFor(Theme::IsLight());
    const float r = Minf(style.corner_radius, Minf(body.Width(), body.Height()) * 0.5f);

    // 1) 主体：冷色 veil + 白雾，两层叠出「厚度」
    Draw::RoundedRectFilled(dl, body, Theme::U32(palette.veil, opacity * style.veil_alpha), r, r, r, r);
    Draw::RoundedRectFilled(dl, body, Theme::U32(palette.body, opacity * style.body_alpha), r, r, r, r);

    // 2) 顶部高光：3 层圆角矩形从上往下衰减（比方形渐变安全，圆角不会露出方角）
    const float band[3] = {0.58f, 0.34f, 0.16f};
    const float band_alpha[3] = {0.35f, 0.35f, 0.30f};
    for (int i = 0; i < 3; ++i) {
        const Rect sheen = Rect::FromPosSize(body.min, ImVec2(body.Width(), body.Height() * band[i]));
        Draw::RoundedRectFilled(dl, sheen, Theme::U32(palette.sheen, opacity * style.sheen_alpha * band_alpha[i]), r, r,
                                r, r);
    }

    // 3) 镜面光斑：多层椭圆叠出软边，位置带液面偏移
    const ImVec2 highlight_center(body.min.x + body.Width() * 0.32f + slosh_.x,
                                  body.min.y + body.Height() * 0.24f + slosh_.y);
    for (int i = 0; i < 5; ++i) {
        const float t = static_cast<float>(i) / 4.0f;
        const ImVec2 radius(body.Width() * 0.30f * (1.0f - t * 0.62f), body.Height() * 0.34f * (1.0f - t * 0.62f));
        const float a = style.highlight_alpha * (0.16f + 0.20f * (1.0f - t));
        dl->AddEllipseFilled(highlight_center, radius, Theme::U32(palette.highlight, opacity * a));
    }

    // 4) 边缘透镜带：外沿压暗 + 内侧亮线 + 两圈边缘雾气
    Draw::RoundedRectOutline(dl, body, Theme::U32(palette.outer, opacity * 0.9f), 1.0f, r, r, r, r);
    const Rect inner = body.Inset(1.0f, 1.0f, 1.0f, 1.0f);
    Draw::RoundedRectOutline(dl, inner, Theme::U32(palette.rim, opacity * style.rim_alpha), style.rim_width,
                             Maxf(r - 1.0f, 0.0f), Maxf(r - 1.0f, 0.0f), Maxf(r - 1.0f, 0.0f), Maxf(r - 1.0f, 0.0f));
    const float fog_inset[2] = {4.0f, 9.0f};
    for (int i = 0; i < 2; ++i) {
        const Rect fog = body.Inset(fog_inset[i], fog_inset[i], fog_inset[i], fog_inset[i]);
        if (!fog.Valid()) {
            continue;
        }
        const float a = style.edge_fog * (i == 0 ? 1.0f : 0.55f);
        const float fr = Maxf(r - fog_inset[i], 0.0f);
        Draw::RoundedRectOutline(dl, fog, Theme::U32(palette.fog, opacity * a), 2.0f, fr, fr, fr, fr);
    }

    // 5) 文字：左上角标签 + 居中提示
    if (!label.empty() && style.label_size > 0.0f) {
        const ImVec2 extent = Draw::MeasureText(nullptr, style.label_size, label.c_str(), 0.0f);
        Draw::Text(dl, nullptr, style.label_size,
                   ImVec2(body.min.x + 14.0f, body.min.y + (r > 14.0f ? 12.0f : 6.0f)),
                   Theme::U32(palette.text, opacity), label.c_str());
        (void)extent;
    }
    if (!hint.empty() && style.hint_size > 0.0f) {
        const ImVec2 extent = Draw::MeasureText(nullptr, style.hint_size, hint.c_str(), 0.0f);
        Draw::Text(dl, nullptr, style.hint_size,
                   ImVec2(body.Center().x - extent.x * 0.5f, body.Center().y - extent.y * 0.5f),
                   Theme::U32(palette.hint, opacity), hint.c_str());
    }
}

void GlassBox::OnDrawContent(ImDrawList* dl, const Rect& content) {
    if (dl == nullptr) {
        return;
    }
    // 用自身绘制矩形（含焦点缩放等变换），玻璃的轮廓必须和阴影/命中矩形一致
    DrawMaterial(dl, DrawRect());
}

} // namespace gui_dev::cv
