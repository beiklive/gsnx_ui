#include "gamemenu/views/DialogView.h"

#include <imgui.h>

namespace gui_dev::gamemenu {

DialogView::DialogView(const char* title, const char* const* lines, int line_count,
                       const ButtonDef* buttons, int button_count, int default_focus,
                       DialogDelegate& delegate)
    : title_(title != nullptr ? title : ""), delegate_(delegate) {
    line_count_ = line_count < kMaxLines ? line_count : kMaxLines;
    for (int i = 0; i < line_count_; ++i) {
        lines_[i] = lines[i];
    }
    button_count_ = button_count < kMaxButtons ? button_count : kMaxButtons;
    for (int i = 0; i < button_count_; ++i) {
        buttons_[i].Configure(buttons[i].label, Icons::Material::Count,
                              buttons[i].danger ? MenuButtonKind::Danger : MenuButtonKind::Normal);
    }
    focus_ = default_focus >= 0 && default_focus < button_count_ ? default_focus : 0;
}

void DialogView::OnEnter(GameMenuContext& ctx) {
    (void)ctx;
    enter_ = 0.0f;
    for (GameMenuButton& button : buttons_) {
        button.Reset();
    }
    focus_frame_.Reset();
}

void DialogView::Update(GameMenuContext& ctx) {
    enter_ = AdvanceOnce(enter_, ctx.theme->animation.enter_duration, ctx.dt);
    for (int i = 0; i < button_count_; ++i) {
        buttons_[i].Update(ctx.dt, i == focus_, ctx.theme->animation);
    }
    focus_frame_.Update(ctx.dt, button_rects_[focus_], ctx.theme->animation);

    const ImVec2 mouse = ImGui::GetMousePos();
    const bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    for (int i = 0; i < button_count_; ++i) {
        if (button_rects_[i].Contains(mouse)) {
            focus_ = i;
            if (clicked) {
                buttons_[i].TriggerPress();
                const bool close = delegate_.OnDialogResult(ctx, i);
                if (close) {
                    ctx.host->RequestPop();
                }
            }
        }
    }
}

bool DialogView::OnAction(GameMenuContext& ctx, InputAction action) {
    switch (action) {
    case InputAction::Left:
        focus_ = (focus_ + button_count_ - 1) % button_count_;
        return true;
    case InputAction::Right:
        focus_ = (focus_ + 1) % button_count_;
        return true;
    case InputAction::Up:
    case InputAction::Down:
        // 对话框只有一行按钮，上下键不做事（避免误操作）
        return true;
    case InputAction::Confirm:
        buttons_[focus_].TriggerPress();
        if (delegate_.OnDialogResult(ctx, focus_)) {
            ctx.host->RequestPop();
        }
        return true;
    default:
        return false;
    }
}

void DialogView::DrawScreenOverlay(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& screen) {
    const float a = EaseOutCubic(enter_) * 0.42f;
    draw_list->AddRectFilled(screen.min, screen.max, ColorWithAlpha(IM_COL32(0, 0, 0, 255), a));
    (void)ctx;
}

void DialogView::Draw(GameMenuContext& ctx, ImDrawList* draw_list, const Rect& area) {
    (void)area; // 对话框居中于整屏，而不是挤在菜单面板里
    const GameMenuTheme& theme = *ctx.theme;
    const Rect screen = ctx.screen;
    const float e = EaseOutBack(Clamp01(enter_));
    const float alpha = EaseOutCubic(Clamp01(enter_));
    const GameMenuTheme faded = ThemeWithAlpha(theme, alpha);

    // 自适应：随画布缩放，但夹在 [420, 640] 之间
    float width = screen.Width() * 0.46f;
    if (width < 420.0f) { width = 420.0f; }
    if (width > 640.0f) { width = 640.0f; }
    // 高度按内容算：标题 54 + 正文行 + 间隔 + 按钮 44 + 下边距
    const float height = 190.0f + static_cast<float>(line_count_) * 36.0f;
    const float dx = (1.0f - e) * 70.0f;
    const float dy = (1.0f - e) * 26.0f;
    const Rect panel = MakeRect(screen.Center().x - width * 0.5f + dx,
                                screen.Center().y - height * 0.5f + dy, width, height);

    // 背板 + 主体
    AddSkewFilled(draw_list, panel.Offset(12.0f, 10.0f), faded.skew, ColorWithAlpha(faded.red, 0.9f));
    AddSkewFilled(draw_list, panel, faded.skew, faded.panel_deep);
    AddSkewBorder(draw_list, panel, faded.skew, faded.white, 2.0f);

    // 标题条
    const float header_h = faded.title_size + 30.0f;
    const Rect header = MakeRect(panel.min.x, panel.min.y, panel.Width() * 0.72f, header_h);
    AddSkewFilled(draw_list, header, faded.skew, ColorWithAlpha(faded.red, 0.96f));
    AddTextLeftVCentered(draw_list, ImVec2(header.min.x + faded.skew + 20.0f, header.Center().y),
                         faded.panel_deep, faded.title_size, title_);

    // 正文
    float y = panel.min.y + header_h + 26.0f;
    for (int i = 0; i < line_count_; ++i) {
        AddTextLeftVCentered(draw_list, ImVec2(panel.min.x + faded.skew + 26.0f, y),
                             i == 0 ? faded.white : faded.white_dim, faded.text_size, lines_[i]);
        y += 36.0f;
    }

    // 按钮行（右对齐到面板内沿）
    const float btn_h = 52.0f;
    const float btn_w = (panel.Width() - faded.panel_padding * 2.0f - 14.0f * (button_count_ - 1)) /
                        static_cast<float>(button_count_);
    float btn_x = panel.min.x + faded.panel_padding;
    const float btn_y = panel.max.y - btn_h - 22.0f;
    for (int i = 0; i < button_count_; ++i) {
        button_rects_[i] = buttons_[i].Draw(draw_list, faded, ImVec2(btn_x, btn_y), btn_w - theme.focus_expand, btn_h,
                                            ctx.time);
        btn_x += btn_w + 14.0f;
    }
    focus_frame_.Draw(draw_list, faded, alpha);
}

} // namespace gui_dev::gamemenu
