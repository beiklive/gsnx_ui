// Badge：机种徽标（运行时画出来的圆角矩形 + 居中短文本，没有图片、没有九宫格）。
//
// 绘制方法（从现网 NanoVG 实现提取，几何完全一致）：
//   textW   = 文本宽度(text)
//   badgeW  = fixed_width ? min_width : max(min_width, textW + 2 * pad_x)
//   badgeH  = height
//   圆角    = radius（<0 表示胶囊 = height / 2）
//   画法    = 圆角矩形填平台色 + 文本 NVG_ALIGN_CENTER|MIDDLE 落在 (x + badgeW/2, y + badgeH/2)
//   badgeWidth() 返回 badgeW，调用方用它接着摆右侧元素（间距 10）
//
// 文字与颜色不在控件里硬编码：setPlatform() 走 PlatformBadgeInfoOf() 查表注入
// （表 = 机种 id → 文字 + 底色，机种 id 顺序同 EmuPlatform，1=GBA … 14=Dolphin）。
// 变体（网格/列表/详情、iisu 封面卡、GameDataView、GridItem）用 setStyle() 选。
#pragma once

#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

// 机种 id：顺序 = EmuPlatform 枚举顺序，从 1 开始（Web 端一致）
enum class EmuPlatform : int {
    Unknown = 0,
    GBA = 1,
    GBC = 2,
    GB = 3,
    FC = 4,
    SFC = 5,
    NDS = 6,
    _3DS = 7,
    MD = 8,
    Arcade = 9,
    DC = 10,
    PSP = 11,
    PS1 = 12,
    Saturn = 13,
    Dolphin = 14, // GC / Wii
};

// 查表结果：徽标文字 + 底色（表里 alpha 都是 220；其它 = 空文字 + (100,100,100,200)）
struct PlatformBadgeInfo {
    const char* text;
    ImVec4 background;
};

// id → 表（唯一数据源，控件与宿主都从这里取）
PlatformBadgeInfo PlatformBadgeInfoOf(EmuPlatform platform);

// 变体：几何与文字色不同，底色都来自平台色（GameDataView 例外，是固定蓝）
enum class BadgeStyle {
    GridListDetail, // 网格 / 列表 / 右侧详情面板：12 / 20 / minW 36 / pad 8 / 圆角 4 / 主题文字色
    IisuCover,      // iisu 封面卡：12 / 17 / minW 30 / pad 14 / 胶囊 / 白字（alpha 乘动画进度）
    GameDataView,   // 游戏详情侧栏：14 / 26 / 宽固定 62 / 圆角 5 / 固定蓝 + 白字 α245
    GridItem,       // borealis GridItem：12 / 20 / minW 36（文本 > 3 字符时 58）/ pad 8 / 圆角 4 / 白字
};

// 变体的几何参数（宿主也能直接读，不必建控件）
struct BadgeStyleParams {
    float font_size = 12.0f;
    float height = 20.0f;
    float min_width = 36.0f;
    float pad_x = 8.0f;
    float radius = 4.0f;             // <0 = 胶囊（height / 2）
    bool fixed_width = false;        // true = 宽度恒为 min_width
    float long_text_min_width = 0.0f; // >0 = 文本超过 3 字符时改用这个 minWidth
    bool white_text = false;         // true = 白字；false = 用主题正文色
    bool fixed_blue_bg = false;      // GameDataView 的固定 (79,153,222,205)
};

BadgeStyleParams BadgeStyleOf(BadgeStyle style);

class Badge : public Widget {
public:
    Badge();
    explicit Badge(EmuPlatform platform);

    // ---- 内容与外观（默认值 = GridListDetail 变体） -------------------------
    EmuPlatform platform = EmuPlatform::Unknown;
    std::string text;
    ImVec4 background = Theme::rgba(100, 100, 100, 200);
    ImVec4 foreground = Theme::kTextPrimary;
    float font_size = 12.0f;
    float height = 20.0f;
    float min_width = 36.0f;
    float pad_x = 8.0f;
    float radius = 4.0f;
    bool fixed_width = false;
    float long_text_min_width = 0.0f;
    float alpha = 1.0f; // 动画进度：乘在底色/文字 alpha 上（iisu 封面卡用）

    // ---- 链式接口 ----------------------------------------------------------
    Badge& setPlatform(EmuPlatform value); // 查表注入文字 + 底色
    Badge& setText(std::string value);
    Badge& setColors(ImVec4 bg, ImVec4 fg); // 显式设色（之后不再跟随主题）
    Badge& setFontSize(float value);
    Badge& setHeight(float value);
    Badge& setMinWidth(float value);
    Badge& setPadding(float value);
    Badge& setRadius(float value); // <0 = 胶囊
    Badge& setFixedWidth(bool value);
    Badge& setAlpha(float value);
    Badge& setStyle(BadgeStyle style);
    Badge& moveTo(float x, float y);

    // 实际徽标宽度（算完才有效；宿主用它 + 10 摆右侧元素）
    float badgeWidth() const;

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnThemeChanged() override;

private:
    float resolvedRadius() const { return radius < 0.0f ? height * 0.5f : radius; }
    bool foreground_follows_theme_ = true; // setColors() 之后置 false
};

} // namespace gui_dev::cv
