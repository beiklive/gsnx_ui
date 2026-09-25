// Header：区块标题（左侧一段竖条 + 标题文字 + 底部分隔线）。
//
// 几何照搬 GBAStation SettingPage 的 SettingsSectionHeaderView（同一套 720p 手持尺寸）：
//   高 58；竖条 4x24 圆角 2，贴左 2px、垂直居中；
//   标题 20px，左内边距 18，垂直居中（中心 = 高度一半 = y+29）；
//   底部分隔线 1px，左右缩进 18 / 6，离底 6px（y+52）。
// 颜色不写死：竖条 / 文字 / 分隔线都取调色板角色色，切主题自动跟随；
// 需要固定色时用 setBarColor / setTextColor / setDividerColor 覆盖。
//
// 用法：页面按「Header + 一组控件」切段，用 Header 把内容水平分隔开
// （size.x 给成内容区宽度，分隔线就会横跨整段）。
#pragma once

#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Header : public Widget {
public:
    struct Style {
        float height = 58.0f;            // 整条高度
        float bar_width = 4.0f;          // 左侧竖条
        float bar_height = 24.0f;
        float bar_radius = 2.0f;
        float bar_offset_x = 2.0f;       // 竖条离左边缘
        float text_offset_x = 18.0f;     // 标题离左边缘
        float text_size = Theme::kFontHeader; // 20
        float info_size = Theme::kFontSmall;  // 右侧补充文字 14
        float info_gap = 12.0f;          // 标题与右侧补充文字的最小间距
        float divider_inset_x = 18.0f;   // 分隔线左缩进
        float divider_inset_right = 6.0f; // 分隔线右缩进
        float divider_offset_y = 6.0f;   // 分隔线离底部的距离
        float divider_width = 1.0f;
        bool divider = true;             // 是否画底部分隔线
    };

    Header();
    explicit Header(std::string title);

    std::string title;
    std::string info; // 右侧补充文字（例如“共 15 项”），可空
    Style style;

    Header& setTitle(std::string value);
    Header& setInfo(std::string value);
    // 颜色覆盖：默认全部跟主题（alpha = 0 表示跟主题）
    Header& setBarColor(const ImVec4& color);
    Header& setTextColor(const ImVec4& color);
    Header& setDividerColor(const ImVec4& color);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;

private:
    ImVec4 bar_color_{0.0f, 0.0f, 0.0f, 0.0f};     // alpha = 0 表示跟主题
    ImVec4 text_color_{0.0f, 0.0f, 0.0f, 0.0f};
    ImVec4 divider_color_{0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace gui_dev::cv
