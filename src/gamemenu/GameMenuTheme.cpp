#include "gamemenu/GameMenuTheme.h"

#include "gamemenu/GameMenuDraw.h"

namespace gui_dev::gamemenu {

const GameMenuTheme& DefaultGameMenuTheme() {
    static const GameMenuTheme theme;
    return theme;
}

GameMenuTheme ThemeWithAlpha(const GameMenuTheme& theme, float scale) {
    const float s = Clamp01(scale);
    GameMenuTheme out = theme;
    out.background = ColorWithAlpha(theme.background, s);
    out.panel = ColorWithAlpha(theme.panel, s);
    out.panel_deep = ColorWithAlpha(theme.panel_deep, s);
    out.white = ColorWithAlpha(theme.white, s);
    out.white_dim = ColorWithAlpha(theme.white_dim, s);
    out.red = ColorWithAlpha(theme.red, s);
    out.red_dim = ColorWithAlpha(theme.red_dim, s);
    out.red_soft = ColorWithAlpha(theme.red_soft, s);
    out.danger = ColorWithAlpha(theme.danger, s);
    out.edge = ColorWithAlpha(theme.edge, s);
    out.dim_color = ColorWithAlpha(theme.dim_color, s);
    return out;
}

GameMenuLayout ResolveGameMenuLayout(const GameMenuTheme& theme, const Rect& screen) {
    GameMenuLayout layout;

    // 宽度：按比例取，并夹在上下限之间。
    float width = screen.Width() * theme.menu_width_ratio;
    if (width < theme.menu_min_width) {
        width = theme.menu_min_width;
    }
    if (width > theme.menu_max_width) {
        width = theme.menu_max_width;
    }
    // 窄画布下别把游戏画面挤没：至少给左侧留 52%
    const float width_limit = screen.Width() * 0.48f;
    if (width > width_limit) {
        width = width_limit;
    }
    layout.panel_width = width;
    layout.panel_x = screen.max.x - theme.menu_right_margin - width;

    // 高度：可用高度封顶，避免超长画布下面板被拉成一条。
    const float available = screen.Height() - theme.menu_top - theme.menu_bottom_margin;
    float height = available > theme.menu_max_height ? theme.menu_max_height : available;
    layout.panel_height = height;
    // 画布比面板高时垂直居中（多出来的空间上下均分）
    layout.panel_y = screen.min.y + theme.menu_top;
    const float slack = available - height;
    if (slack > 0.0f) {
        layout.panel_y += slack * 0.5f;
    }

    // 槽位栅格：内容区够宽就多排一列
    layout.wide = (width - theme.panel_padding * 2.0f) >= theme.slot_wide_threshold;
    layout.slot_columns = layout.wide ? theme.slot_columns_wide : theme.slot_columns_narrow;
    if (layout.slot_columns < 1) {
        layout.slot_columns = 1;
    }
    constexpr int kSlotsPerPage = 6;
    layout.slot_rows = (kSlotsPerPage + layout.slot_columns - 1) / layout.slot_columns;
    return layout;
}

} // namespace gui_dev::gamemenu
