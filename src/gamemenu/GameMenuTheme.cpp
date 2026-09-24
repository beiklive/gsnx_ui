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

} // namespace gui_dev::gamemenu
