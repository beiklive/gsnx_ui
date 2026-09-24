// PropertyPanel：页面右下角的属性面板。
//
// 页面每帧往里塞「分组 + 属性行」，面板自己排版（左名称右值，当前状态行高亮），
// 这样每个控件页都只需要描述状态，不用关心排版。
#pragma once

#include <string>
#include <vector>

#include "component_view/Types.h"
#include "component_view/Widget.h"

namespace gui_dev::cv {

class PropertyPanel : public Widget {
public:
    PropertyPanel();

    bool two_columns = true;
    float row_height = 19.0f;
    float section_gap = 10.0f;
    float title_gap = 5.0f;
    float column_gap = 26.0f;
    float name_ratio = 0.46f;
    float font_size = 14.0f;
    ImU32 section_title_color = Theme::kAccent;
    ImU32 name_color = Theme::kTextMuted;
    ImU32 value_color = Theme::kTextPrimary;
    ImU32 highlight_color = Theme::kTeal;
    ImU32 highlight_bg = 0;

    // ---- 每帧填充 ----------------------------------------------------------
    void Clear();
    void AddSection(std::string title);
    void AddRow(std::string name, std::string value, bool highlight = false);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;

private:
    struct Section {
        std::string title;
        std::vector<PropRow> rows;
    };
    std::vector<Section> sections_;
};

} // namespace gui_dev::cv
