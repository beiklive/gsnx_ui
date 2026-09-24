// 暂停菜单的视觉参数（对应需求 §25）。
//
// 只做「视觉语言」：黑 / 白 / 红三色、高对比、斜切几何、不规则边缘。
// 不复制任何具体作品的角色/Logo/字体/素材。
//
// 所有几何与时长都必须是 Theme 参数，绘制代码里不许出现魔法数字。
#pragma once

#include <imgui.h>

#include "gamemenu/MenuAnimation.h"

namespace gui_dev::gamemenu {

struct GameMenuTheme {
    // ---- 三色体系 ----------------------------------------------------------
    ImU32 background = IM_COL32(0x0B, 0x0B, 0x0D, 0xFF);    // 近黑
    ImU32 panel = IM_COL32(0x13, 0x13, 0x16, 0xF7);         // 黑
    ImU32 panel_deep = IM_COL32(0x08, 0x08, 0x0A, 0xFF);
    ImU32 white = IM_COL32(0xF5, 0xF5, 0xF7, 0xFF);         // 主文字
    ImU32 white_dim = IM_COL32(0x98, 0x98, 0xA1, 0xFF);     // 次级文字
    ImU32 red = IM_COL32(0xE2, 0x1B, 0x25, 0xFF);           // 高饱和强调红
    ImU32 red_dim = IM_COL32(0x7E, 0x0E, 0x15, 0xFF);
    ImU32 red_soft = IM_COL32(0xE2, 0x1B, 0x25, 0x66);
    ImU32 danger = IM_COL32(0xE2, 0x1B, 0x25, 0xFF);
    ImU32 edge = IM_COL32(0xF5, 0xF5, 0xF7, 0x3C);          // 白色细边框

    // ---- 遮罩 --------------------------------------------------------------
    // 需求 §13：不要把游戏画面全黑掉，0.35~0.55
    float dim_alpha = 0.46f;
    ImU32 dim_color = IM_COL32(0x00, 0x00, 0x00, 0xFF); // alpha 由 dim_alpha 控制

    // ---- 几何（全部是 1280x720 设计空间里的值）-----------------------------
    float skew = 13.0f;              // 斜切量（px）
    float border_width = 2.0f;       // 常态边框
    float border_width_focus = 3.5f; // 聚焦边框
    float focus_offset = 16.0f;      // 聚焦右移（需求 8~16）
    float focus_expand = 34.0f;      // 聚焦加宽（需求 20~30）
    float focus_height_expand = 6.0f;// 聚焦微增高
    float press_shrink = 0.035f;     // 按压收缩比例
    float arrow_gap = 26.0f;         // 箭头与按钮左沿的距离
    float arrow_size = 9.0f;

    // 装饰（也放主题里，避免绘制代码里散落尺寸）
    float accent_plate_offset = 11.0f; // 红色错位背板偏移
    float focus_tick_length = 16.0f;   // 焦点框四角 L 形长度
    float sweep_width = 62.0f;         // 扫描高光条宽度

    // 720p 布局：菜单靠右，游戏画面保留在左侧（需求 §15）
    float menu_right_margin = 48.0f;
    float menu_width = 520.0f;   // 约 40% 屏宽
    float menu_top = 48.0f;
    float menu_bottom_margin = 32.0f;
    float row_height = 56.0f;
    float row_gap = 8.0f;
    float group_gap = 19.0f;   // 分组之间（用分隔线，不用文字）
    float panel_padding = 28.0f;

    // 字号（720p 设计空间）
    float title_size = 36.0f;
    float text_size = 25.0f;
    float small_size = 18.0f;
    float slot_index_size = 34.0f;

    // ---- 动画 --------------------------------------------------------------
    MenuAnimationConfig animation;
};

const GameMenuTheme& DefaultGameMenuTheme();

// 把主题里所有颜色的 alpha 乘上 scale。
// 逐项出现的淡入直接用它，避免在元素的每个颜色上穿一个 alpha 参数。
GameMenuTheme ThemeWithAlpha(const GameMenuTheme& theme, float scale);

} // namespace gui_dev::gamemenu
