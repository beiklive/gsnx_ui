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

// 画布/元素矩形（几何与布局解析共用）
struct Rect {
    ImVec2 min{};
    ImVec2 max{};

    float Width() const { return max.x - min.x; }
    float Height() const { return max.y - min.y; }
    ImVec2 Size() const { return ImVec2(Width(), Height()); }
    ImVec2 Center() const { return ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f); }
    bool Contains(const ImVec2& p) const {
        return p.x >= min.x && p.x < max.x && p.y >= min.y && p.y < max.y;
    }
    Rect Inflated(float x, float y) const {
        return Rect{ImVec2(min.x - x, min.y - y), ImVec2(max.x + x, max.y + y)};
    }
    Rect Offset(float x, float y) const {
        return Rect{ImVec2(min.x + x, min.y + y), ImVec2(max.x + x, max.y + y)};
    }
};

inline Rect MakeRect(float x, float y, float w, float h) {
    return Rect{ImVec2(x, y), ImVec2(x + w, y + h)};
}

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
    // 红色错位色块：**每次获得焦点重新掷一次**，让每个条目/每次聚焦都不一样
    // （Persona 式「不规则边缘 + 轻微位移」）。取值范围都写在这里。
    // 注意：色块压在按钮下面，只有露出来的边才可见 —— 所以
    // row_gap 必须留得下 plate_offset_y 的最大值，否则变化会被下一个按钮盖住。
    float plate_offset_x_min = -18.0f; // 负值 = 往左错位（左边缘也参与变化）
    float plate_offset_x_max = 12.0f;
    float plate_offset_y_min = 5.0f;   // 向下错位 = 底部露出的高度
    float plate_offset_y_max = 18.0f;
    float plate_extend_min = -6.0f;    // 宽度额外伸缩（负 = 比本体窄）
    float plate_extend_max = 24.0f;
    float plate_skew_jitter = 8.0f;    // 斜切量的随机浮动
    float plate_smoothing = 15.0f;     // 换形状时的过渡速度（SmoothTo）
    float focus_tick_length = 16.0f;   // 焦点框四角 L 形长度
    float sweep_width = 62.0f;         // 扫描高光条宽度

    // 720p 布局：菜单**靠左**弹出，游戏画面保留在右侧
    // 这些是 1280x720 下的基准值；实际画布不同比例时由 ResolveLayout 自适应。
    float menu_left_margin = 48.0f;
    float menu_width = 520.0f;      // 基准宽（720p 下约 41% 屏宽）
    float menu_width_ratio = 0.41f;  // 画布更宽时按比例取宽
    float menu_min_width = 440.0f;   // 再窄也不小于这个
    float menu_max_width = 640.0f;   // 再宽也不超过这个（免得盖住太多游戏画面）
    float menu_top = 48.0f;
    float menu_bottom_margin = 32.0f;
    float menu_max_height = 700.0f;  // 画布很高时面板不跟着无限拉长
    float slot_wide_threshold = 540.0f; // 内容区宽于此时槽位改 3 列 2 行
    int slot_columns_narrow = 2;
    int slot_columns_wide = 3;
    float row_height = 56.0f;
    float row_gap = 12.0f;     // 留出红色错位块向下露出的空间
    float group_gap = 14.0f;   // 分组之间（用分隔线，不用文字）
    float panel_padding = 24.0f;

    // 字号（720p 设计空间）
    float title_size = 36.0f;
    float text_size = 25.0f;
    float small_size = 18.0f;
    float slot_index_size = 34.0f;

    // ---- 动画 --------------------------------------------------------------
    MenuAnimationConfig animation;
};

const GameMenuTheme& DefaultGameMenuTheme();

// ---- 自适应布局 ------------------------------------------------------------
// 720p 的设计值只是基准：画布比例变化时（桌面窗口、非 16:9）由这里算出实际布局。
// scale = min(h/720, w/1280) 保证逻辑画布 ≥ 1280x720，所以这里只需要处理
// 「更宽」与「更高」两种情况。
struct GameMenuLayout {
    float panel_width = 520.0f;
    float panel_height = 640.0f;
    float panel_x = 0.0f;
    float panel_y = 48.0f;
    int slot_columns = 2;
    int slot_rows = 3;
    bool wide = false; // 画布比 16:9 更宽（槽位可多排一列）
};

GameMenuLayout ResolveGameMenuLayout(const GameMenuTheme& theme, const Rect& screen);

// 把主题里所有颜色的 alpha 乘上 scale。
// 逐项出现的淡入直接用它，避免在元素的每个颜色上穿一个 alpha 参数。
GameMenuTheme ThemeWithAlpha(const GameMenuTheme& theme, float scale);

} // namespace gui_dev::gamemenu
