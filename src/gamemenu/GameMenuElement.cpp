#include "gamemenu/GameMenuElement.h"

#include <cmath>
#include <cstdio>

namespace gui_dev::gamemenu {
namespace {

constexpr float kPi = 3.14159265358979323846f;

// 复用 GameMenuDraw 里的公共实现（保留短名，绘制代码更可读）
inline ImU32 WithAlpha(ImU32 col, float alpha) { return ColorWithAlpha(col, alpha); }
inline ImU32 LerpColor(ImU32 a, ImU32 b, float t) { return ColorLerp(a, b, t); }

// 按压：press 由 1 衰减到 0，收缩量在中间达到峰值 → 先缩后弹回。
inline float PressShrink(float press, float amount) {
    return amount * std::sin(kPi * Clamp01(press));
}

} // namespace

// ---------------------------------------------------------------- 焦点框 ----

void GameMenuFocusFrame::Reset() {
    rect_ = Rect{};
    alpha_ = 0.0f;
}

void GameMenuFocusFrame::SnapTo(const Rect& target) { rect_ = target; }

void GameMenuFocusFrame::Update(float dt, const Rect& target, const MenuAnimationConfig& cfg) {
    const float speed = cfg.follow_smoothing;
    rect_.min.x = SmoothTo(rect_.min.x, target.min.x, speed, dt);
    rect_.min.y = SmoothTo(rect_.min.y, target.min.y, speed, dt);
    rect_.max.x = SmoothTo(rect_.max.x, target.max.x, speed, dt);
    rect_.max.y = SmoothTo(rect_.max.y, target.max.y, speed, dt);
    alpha_ = SmoothTo(alpha_, 1.0f, 10.0f, dt);
}

void GameMenuFocusFrame::Draw(ImDrawList* draw_list, const GameMenuTheme& theme, float strength) const {
    if (alpha_ <= 0.02f || strength <= 0.02f) {
        return;
    }
    const float a = alpha_ * strength;
    const Rect frame = rect_.Inflated(9.0f, 6.0f);
    AddCornerTicks(draw_list, frame, 13.0f, 2.0f, WithAlpha(theme.white, 0.85f * a));
    AddSkewBorder(draw_list, Rect{ImVec2(frame.min.x - 3.0f, frame.min.y - 3.0f),
                                  ImVec2(frame.max.x + 3.0f, frame.max.y + 3.0f)},
                  theme.skew * 0.5f, WithAlpha(theme.red, 0.5f * a), 1.0f);
}

// ------------------------------------------------------------------ 按钮 ----

void GameMenuButton::Configure(const char* label, Icons::Material icon, MenuButtonKind kind) {
    label_ = label != nullptr ? label : "";
    icon_ = icon;
    kind_ = kind;
    anim_.Reset();
    was_focused_ = false;
}

void GameMenuButton::Reset() {
    anim_.Reset();
    was_focused_ = false;
}

void GameMenuButton::Update(float dt, bool focused, const MenuAnimationConfig& cfg) {
    // 焦点切换：固定时长推进，几何量再套 EaseOutBack 产生轻微弹性（需求 §6.8）
    anim_.focus = MoveTowards(anim_.focus, focused ? 1.0f : 0.0f, cfg.focus_duration, dt);
    // 获得焦点的那一帧触发一次扫描高光（只触发一次，不循环，需求 §10）
    if (focused && !was_focused_) {
        anim_.sweep = 1.0f;
        anim_.flash = 1.0f;
    }
    was_focused_ = focused;

    anim_.press = DecayOnce(anim_.press, cfg.press_duration, dt);
    anim_.sweep = DecayOnce(anim_.sweep, cfg.sweep_duration, dt);
    anim_.flash = DecayOnce(anim_.flash, cfg.flash_duration, dt);
}

Rect GameMenuButton::Draw(ImDrawList* draw_list, const GameMenuTheme& theme, ImVec2 origin,
                          float width, float height, float time) const {
    const float f = EaseOutBack(anim_.focus);
    const float focus01 = anim_.focus;

    // 分组分隔线：不画按钮，只画一条不规则切线
    if (kind_ == MenuButtonKind::Separator) {
        const Rect line = MakeRect(origin.x, origin.y, width, height);
        AddJaggedRule(draw_list, line.min.x + 6.0f, line.max.x - 6.0f, line.Center().y, 2.5f, 9.0f,
                      WithAlpha(theme.white, 0.35f));
        return line;
    }

    // ---- 几何：未聚焦基准位置 → 聚焦右移 + 加宽 + 微增高 ----
    Rect rect = MakeRect(origin.x + theme.focus_offset * f,
                         origin.y - theme.focus_height_expand * 0.5f * f,
                         width + theme.focus_expand * f, height + theme.focus_height_expand * f);

    // 按压：绕中心轻微收缩
    const float shrink = PressShrink(anim_.press, theme.press_shrink);
    if (shrink > 0.0001f) {
        const ImVec2 c = rect.Center();
        const float half_w = rect.Width() * 0.5f * (1.0f - shrink);
        const float half_h = rect.Height() * 0.5f * (1.0f - shrink);
        rect = Rect{ImVec2(c.x - half_w, c.y - half_h), ImVec2(c.x + half_w, c.y + half_h)};
    }

    const bool danger = kind_ == MenuButtonKind::Danger;

    // ---- 红色扩张层：向右下偏移的红色背板（Persona 式错位板）----
    if (focus01 > 0.01f) {
        const Rect plate = rect.Offset(9.0f * focus01, 6.0f * focus01);
        AddSkewFilled(draw_list, plate, theme.skew, WithAlpha(theme.red, 0.95f * focus01));
    }
    // ---- 黑色主体 ----
    AddSkewFilled(draw_list, rect, theme.skew, theme.panel_deep);
    // 聚焦时主体内部左侧一条红条
    if (focus01 > 0.02f) {
        Rect bar = MakeRect(rect.min.x + 1.0f, rect.min.y, 6.0f * focus01, rect.Height());
        AddSkewFilledLeft(draw_list, bar, theme.skew * 0.4f, WithAlpha(theme.red, 0.95f * focus01));
    }
    // ---- 白色边框：聚焦更明显 ----
    const float border = Lerp(theme.border_width, theme.border_width_focus, focus01);
    const ImU32 border_col = WithAlpha(danger ? theme.red : theme.white,
                                       Lerp(danger ? 1.0f : 0.42f, 1.0f, focus01));
    AddSkewBorder(draw_list, rect, theme.skew, border_col, border);
    // ---- 危险项常态提示：右侧一条红杠 ----
    if (danger && focus01 < 0.5f) {
        Rect bar = MakeRect(rect.max.x - 5.0f, rect.min.y + 6.0f, 3.0f, rect.Height() - 12.0f);
        AddSkewFilled(draw_list, bar, theme.skew * 0.3f, WithAlpha(theme.red, 0.8f));
    }

    // ---- 扫描高光：只在获得焦点后的 220ms 内扫一次 ----
    if (anim_.sweep > 0.001f) {
        const float progress = 1.0f - anim_.sweep;
        const float peak = std::sin(kPi * Clamp01(progress));
        AddSweep(draw_list, rect, theme.skew * 0.6f, progress, 54.0f,
                 WithAlpha(theme.white, 0.22f * peak));
        AddSweep(draw_list, rect, theme.skew * 0.6f, progress, 18.0f,
                 WithAlpha(theme.red, 0.30f * peak));
    }

    // ---- 文字与图标：随焦点右移 ----
    const float text_shift = theme.focus_offset * 0.45f * focus01;
    const float center_y = rect.Center().y;
    const float icon_x = rect.min.x + theme.skew + 16.0f + text_shift;

    if (icon_ != Icons::Material::Count) {
        const float icon_size = theme.text_size * 1.25f;
        const ImVec2 extent = ImGui::GetFont()->CalcTextSizeA(icon_size, FLT_MAX, 0.0f,
                                                             Icons::Glyph(icon_));
        AddTextLeft(draw_list, ImVec2(icon_x, center_y - extent.y * 0.5f),
                    WithAlpha(theme.red, Lerp(0.7f, 1.0f, focus01)), icon_size, Icons::Glyph(icon_));
        AddTextLeftVCentered(draw_list, ImVec2(icon_x + extent.x + 12.0f, center_y),
                             LerpColor(theme.white_dim, theme.white, focus01), theme.text_size,
                             label_);
    } else {
        AddTextLeftVCentered(draw_list, ImVec2(icon_x, center_y),
                             LerpColor(theme.white_dim, theme.white, focus01), theme.text_size, label_);
    }

    // ---- 左侧箭头：滑入 + 透明度 + 轻微摆动（需求 §9）----
    if (focus01 > 0.02f) {
        const float swing = std::sin(time * 8.0f) * 3.0f * focus01;
        const float arrow_x = rect.min.x - theme.arrow_gap + (1.0f - focus01) * -14.0f + swing;
        const float a = theme.arrow_size * (0.6f + 0.4f * focus01);
        const ImVec2 tip(arrow_x + a, center_y);
        const ImVec2 p[3] = {ImVec2(arrow_x, center_y - a), tip, ImVec2(arrow_x, center_y + a)};
        draw_list->AddConvexPolyFilled(p, 3, WithAlpha(theme.red, 0.55f + 0.45f * focus01));
    }

    return rect;
}

// ------------------------------------------------------------------ 面板 ----

void GameMenuPanel::Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect,
                         const char* title, const char* subtitle, float enter) const {
    const float e = EaseOutCubic(Clamp01(enter));

    // 主体：黑色斜切板 + 白色细边
    AddSkewFilled(draw_list, rect, theme.skew, theme.panel);
    AddSkewBorder(draw_list, rect, theme.skew, WithAlpha(theme.white, 0.30f), 1.5f);

    // 顶部红色斜切标题条（宽度随 enter 展开）
    Rect header = MakeRect(rect.min.x, rect.min.y, rect.Width() * e, 58.0f);
    AddSkewFilled(draw_list, header, theme.skew, theme.red);
    Rect header_hand = MakeRect(rect.min.x + 7.0f, rect.min.y + 58.0f, rect.Width() * e * 0.42f, 5.0f);
    AddSkewFilled(draw_list, header_hand, theme.skew * 0.5f, WithAlpha(theme.panel_deep, 0.9f));

    // 右侧竖红条（漫画切割感）
    Rect side = MakeRect(rect.max.x - 8.0f, rect.min.y + 70.0f, 4.0f, rect.Height() - 86.0f);
    AddSkewFilled(draw_list, side, 2.0f, WithAlpha(theme.red, 0.85f));

    // 底部不规则边缘
    AddJaggedRule(draw_list, rect.min.x + 14.0f, rect.max.x - 22.0f, rect.max.y - 2.0f, 3.0f, 11.0f,
                  WithAlpha(theme.white, 0.25f));

    // 标题（比面板稍晚一点到位，形成层次）
    const float title_alpha = Clamp01((enter - 0.18f) / 0.5f);
    if (title_alpha > 0.01f) {
        AddDiamond(draw_list, ImVec2(rect.min.x + theme.panel_padding + 6.0f, rect.min.y + 29.0f), 7.0f,
                   WithAlpha(theme.panel_deep, title_alpha));
        AddTextLeftVCentered(draw_list, ImVec2(rect.min.x + theme.panel_padding + 22.0f,
                                               rect.min.y + 29.0f),
                             WithAlpha(theme.panel_deep, title_alpha), theme.title_size, title);
        // 白/红错位标题（漫画重影）
        AddTextLeftVCentered(draw_list, ImVec2(rect.min.x + theme.panel_padding + 25.0f,
                                               rect.min.y + 32.0f),
                             WithAlpha(theme.white, 0.25f * title_alpha), theme.title_size, title);
    }
    if (subtitle != nullptr && subtitle[0] != '\0' && title_alpha > 0.01f) {
        AddTextLeftVCentered(draw_list, ImVec2(rect.min.x + theme.panel_padding + 2.0f,
                                               rect.min.y + 74.0f),
                             WithAlpha(theme.white_dim, title_alpha), theme.small_size, subtitle);
    }
}

// -------------------------------------------------------------- 分类签 ------

void GameMenuTab::Configure(const char* label) { label_ = label != nullptr ? label : ""; }

void GameMenuTab::Reset() {
    anim_.Reset();
    was_selected_ = false;
}

void GameMenuTab::Update(float dt, bool selected, const MenuAnimationConfig& cfg) {
    anim_.focus = MoveTowards(anim_.focus, selected ? 1.0f : 0.0f, cfg.focus_duration, dt);
    if (selected && !was_selected_) {
        anim_.flash = 1.0f;
    }
    was_selected_ = selected;
    anim_.flash = DecayOnce(anim_.flash, cfg.flash_duration, dt);
}

Rect GameMenuTab::Draw(ImDrawList* draw_list, const GameMenuTheme& theme, ImVec2 origin, float width,
                       float height) const {
    const float f = EaseOutBack(anim_.focus);
    Rect rect = MakeRect(origin.x + 6.0f * f, origin.y, width + 8.0f * f, height);

    if (anim_.focus > 0.02f) {
        AddSkewFilled(draw_list, rect, theme.skew * 0.7f, WithAlpha(theme.red, 0.92f * anim_.focus));
        Rect plate = rect.Offset(5.0f * f, 3.0f * f);
        AddSkewFilled(draw_list, plate, theme.skew * 0.7f, WithAlpha(theme.white, 0.10f * anim_.focus));
    }
    if (anim_.flash > 0.01f) {
        AddSkewFilled(draw_list, rect, theme.skew * 0.7f, WithAlpha(theme.white, 0.22f * anim_.flash));
    }
    const ImU32 text_col = LerpColor(theme.white_dim, theme.panel_deep, anim_.focus);
    AddTextLeftVCentered(draw_list, ImVec2(rect.min.x + 12.0f, rect.Center().y), text_col,
                         theme.text_size, label_);
    return rect;
}

// ------------------------------------------------------- 数值选择器 / 开关 ----

void GameMenuSelector::Reset() {
    slide_ = 0.0f;
    direction_ = 0;
}

void GameMenuSelector::OnValueChanged(int direction) {
    slide_ = 1.0f;
    direction_ = direction;
}

void GameMenuSelector::Update(float dt, const MenuAnimationConfig& cfg) {
    slide_ = DecayOnce(slide_, cfg.sweep_duration, dt);
}

void GameMenuSelector::Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect,
                            const char* value, bool focused, float time) const {
    const float f = focused ? 1.0f : 0.0f;
    const float gap = 8.0f + 3.0f * f + 5.0f * slide_;
    const float arrow = 5.0f + 1.5f * f;
    const float center_y = rect.Center().y;
    const float right = rect.max.x - 4.0f;

    // 文字随修改方向滑动 + 淡入（需求 §21）
    const float shift = static_cast<float>(direction_) * 12.0f * slide_;
    const float text_alpha = 1.0f - 0.35f * slide_;
    const float width = TextWidth(theme.text_size, value);

    AddTextLeftVCentered(draw_list, ImVec2(right - width - gap - arrow - 6.0f + shift, center_y),
                         WithAlpha(LerpColor(theme.white, theme.white, f), text_alpha),
                         theme.text_size, value);

    // 左箭头
    const float lx = right - width - gap - arrow - 6.0f - arrow * 1.6f + shift;
    const ImVec2 left_tri[3] = {ImVec2(lx + arrow * 0.9f, center_y - arrow),
                                ImVec2(lx - arrow * 0.6f, center_y),
                                ImVec2(lx + arrow * 0.9f, center_y + arrow)};
    draw_list->AddConvexPolyFilled(left_tri, 3, WithAlpha(theme.white, 0.35f + 0.65f * f));

    // 右箭头
    const float rx = right + 2.0f - arrow * 0.4f;
    const ImVec2 right_tri[3] = {ImVec2(rx - arrow * 0.9f, center_y - arrow),
                                 ImVec2(rx + arrow * 0.6f, center_y),
                                 ImVec2(rx - arrow * 0.9f, center_y + arrow)};
    draw_list->AddConvexPolyFilled(right_tri, 3, WithAlpha(theme.white, 0.35f + 0.65f * f));
}

// ------------------------------------------------------------- 设置选项行 ----

void MenuOptionRow::Reset() {
    anim.Reset();
    selector.Reset();
    toggle = ToggleOn() ? 1.0f : 0.0f;
}

void MenuOptionRow::Update(float dt, const MenuAnimationConfig& cfg) {
    // 焦点由外部传入 Draw，这里只推进反馈类动画
    anim.flash = DecayOnce(anim.flash, cfg.flash_duration, dt);
    selector.Update(dt, cfg);
    toggle = SmoothTo(toggle, ToggleOn() ? 1.0f : 0.0f, 18.0f, dt);
}

void MenuOptionRow::Change(int direction) {
    if (kind == MenuOptionKind::Toggle) {
        value = value != 0 ? 0 : 1;
    } else if (kind == MenuOptionKind::Choice && value_count > 0) {
        value = (value + direction + value_count) % value_count;
    } else {
        return;
    }
    selector.OnValueChanged(direction);
    anim.flash = 1.0f;
}

const char* MenuOptionRow::CurrentValue() const {
    if (kind == MenuOptionKind::Command) {
        return "";
    }
    if (kind == MenuOptionKind::Toggle) {
        return ToggleOn() ? "ON" : "OFF";
    }
    if (values != nullptr && value >= 0 && value < value_count) {
        return values[value];
    }
    return "-";
}

void MenuOptionRow::Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect,
                         bool focused, float time) const {
    // 聚焦：整行右移 + 红底（与按钮同一套视觉语言）
    const float f = focused ? 1.0f : 0.0f;
    Rect row = rect.Offset(10.0f * f, 0.0f);
    if (focused) {
        Rect plate = row.Offset(6.0f, 4.0f);
        AddSkewFilled(draw_list, plate, theme.skew * 0.6f, WithAlpha(theme.red, 0.9f));
        AddSkewFilled(draw_list, row, theme.skew * 0.6f, theme.panel_deep);
        AddSkewBorder(draw_list, row, theme.skew * 0.6f, theme.white, 2.0f);
    } else {
        AddSkewFilled(draw_list, row, theme.skew * 0.6f, WithAlpha(theme.panel_deep, 0.55f));
        AddSkewBorder(draw_list, row, theme.skew * 0.6f, WithAlpha(theme.white, 0.18f), 1.0f);
    }
    // 修改反馈：一次短闪
    if (anim.flash > 0.01f) {
        AddSkewFilled(draw_list, row, theme.skew * 0.6f,
                      WithAlpha(theme.red, 0.28f * anim.flash));
    }

    const float label_x = row.min.x + theme.skew + 14.0f;
    AddTextLeftVCentered(draw_list, ImVec2(label_x, row.Center().y),
                         LerpColor(theme.white_dim, theme.white, f), theme.text_size, label);

    if (kind == MenuOptionKind::Toggle) {
        // 开关：[ 槽 ] + 红色填充条随状态生长
        const float w = 62.0f;
        const float h = 22.0f;
        Rect slot = MakeRect(row.max.x - w - 10.0f, row.Center().y - h * 0.5f, w, h);
        AddSkewFilled(draw_list, slot, 4.0f, WithAlpha(theme.background, 0.9f));
        Rect fill = MakeRect(slot.min.x, slot.min.y, w * toggle, h);
        AddSkewFilled(draw_list, fill, 4.0f, WithAlpha(theme.red, 0.95f));
        AddSkewBorder(draw_list, slot, 4.0f, WithAlpha(theme.white, 0.7f), 1.5f);
        AddTextCentered(draw_list, slot, ToggleOn() ? theme.white : theme.white_dim, theme.small_size,
                        ToggleOn() ? "ON" : "OFF");
    } else if (kind == MenuOptionKind::Choice) {
        const Rect area = MakeRect(row.max.x - 300.0f, row.min.y, 290.0f, row.Height());
        selector.Draw(draw_list, theme, area, CurrentValue(), focused, time);
    } else {
        // Command：右侧一个斜箭头
        const float x = row.max.x - 26.0f;
        const ImVec2 tri[3] = {ImVec2(x, row.Center().y - 7.0f), ImVec2(x + 9.0f, row.Center().y),
                               ImVec2(x, row.Center().y + 7.0f)};
        draw_list->AddConvexPolyFilled(tri, 3, WithAlpha(theme.white, 0.75f));
    }
}

// -------------------------------------------------------------- 存档槽 ------

void GameMenuSaveSlot::Reset() {
    anim_.Reset();
    was_focused_ = false;
}

void GameMenuSaveSlot::Update(float dt, bool focused, const MenuAnimationConfig& cfg) {
    anim_.focus = MoveTowards(anim_.focus, focused ? 1.0f : 0.0f, cfg.focus_duration, dt);
    if (focused && !was_focused_) {
        anim_.sweep = 1.0f;
    }
    was_focused_ = focused;
    anim_.sweep = DecayOnce(anim_.sweep, cfg.sweep_duration, dt);
    anim_.press = DecayOnce(anim_.press, cfg.press_duration, dt);
}

Rect GameMenuSaveSlot::Draw(ImDrawList* draw_list, const GameMenuTheme& theme, const Rect& rect,
                            int index, const GameMenuSlotData& data, float time) const {
    const float f = EaseOutBack(anim_.focus);
    // 聚焦：轻微放大 + 位移（需求 §16）
    Rect card = rect.Offset(6.0f * f, -3.0f * f);
    if (f > 0.001f) {
        const ImVec2 c = card.Center();
        const float sx = 1.0f + 0.028f * f;
        const float sy = 1.0f + 0.020f * f;
        card = Rect{ImVec2(c.x - card.Width() * 0.5f * sx, c.y - card.Height() * 0.5f * sy),
                    ImVec2(c.x + card.Width() * 0.5f * sx, c.y + card.Height() * 0.5f * sy)};
    }

    // 缩略图占位（真实实现应来自模拟器截图 API；demo 用抽象条纹）
    const float footer_h = 46.0f;
    const Rect thumb = MakeRect(card.min.x + 9.0f, card.min.y + 8.0f, card.Width() - 18.0f,
                                card.Height() - footer_h - 14.0f);
    if (anim_.focus > 0.01f) {
        AddSkewFilled(draw_list, card.Offset(7.0f * anim_.focus, 5.0f * anim_.focus), theme.skew * 0.6f,
                      WithAlpha(theme.red, 0.95f * anim_.focus));
    }
    AddSkewFilled(draw_list, card, theme.skew * 0.6f, theme.panel_deep);
    AddSkewBorder(draw_list, card, theme.skew * 0.6f,
                  data.exists ? WithAlpha(theme.white, 0.25f) : WithAlpha(theme.white, 0.12f), 1.0f);

    if (data.exists) {
        // 抽象"截图"：斜条纹 + 红色色块，避免依赖任何外部素材
        AddSkewFilled(draw_list, thumb, 4.0f, IM_COL32(0x1C, 0x1C, 0x22, 0xFF));
        for (int i = 0; i < 5; ++i) {
            Rect stripe = MakeRect(thumb.min.x + 8.0f + i * 26.0f, thumb.min.y + 4.0f, 10.0f,
                                   thumb.Height() - 8.0f);
            AddSkewFilled(draw_list, stripe, 5.0f, WithAlpha(theme.red, 0.12f + 0.05f * i));
        }
        AddSkewFilled(draw_list, MakeRect(thumb.min.x, thumb.max.y - 7.0f, thumb.Width(), 7.0f), 4.0f,
                      WithAlpha(theme.white, 0.10f));
    } else {
        AddSkewFilled(draw_list, thumb, 4.0f, IM_COL32(0x10, 0x10, 0x13, 0xFF));
        AddTextCentered(draw_list, thumb, WithAlpha(theme.white_dim, 0.7f), theme.text_size, "空");
    }

    // 底部信息条：槽位编号 + 时间 + 游戏时长（全部限制在卡内）
    const float info_y = card.max.y - 24.0f;
    const Rect badge = MakeRect(card.min.x + 9.0f, card.max.y - 38.0f, 42.0f, 26.0f);
    char index_text[8];
    std::snprintf(index_text, sizeof(index_text), "%02d", index + 1);
    AddSkewFilled(draw_list, badge, 4.0f,
                  data.exists ? WithAlpha(theme.red, 0.9f) : WithAlpha(theme.white, 0.12f));
    AddTextCentered(draw_list, badge, theme.white, theme.slot_index_size, index_text);

    const float text_x = badge.max.x + 9.0f;
    if (data.exists) {
        AddTextLeftVCentered(draw_list, ImVec2(text_x, info_y - 8.0f),
                             WithAlpha(theme.white, 0.92f), theme.small_size, data.time_text);
        AddTextLeftVCentered(draw_list, ImVec2(text_x, info_y + 9.0f),
                             WithAlpha(theme.white_dim, 0.92f), theme.small_size, data.play_text);
    } else {
        AddTextLeftVCentered(draw_list, ImVec2(text_x, info_y),
                             WithAlpha(theme.white_dim, 0.6f), theme.small_size, "空存档槽");
    }

    // 扫描高光（一次性）
    if (anim_.sweep > 0.001f) {
        const float progress = 1.0f - anim_.sweep;
        AddSweep(draw_list, card, theme.skew * 0.6f, progress, 46.0f,
                 WithAlpha(theme.white, 0.20f * std::sin(kPi * progress)));
    }
    // 聚焦时的动态角标
    if (anim_.focus > 0.02f) {
        const float pulse = 0.6f + 0.4f * std::sin(time * 6.0f);
        AddCornerTicks(draw_list, card.Inflated(4.0f, 4.0f), 10.0f, 2.0f,
                       WithAlpha(theme.white, 0.7f * anim_.focus * pulse));
    }
    return card;
}

} // namespace gui_dev::gamemenu
