#include "component_view/components/PropertyPanel.h"

#include "component_view/Draw.h"

namespace gui_dev::cv {

PropertyPanel::PropertyPanel() : Widget("property_panel") {
    interactive = false;
    focusable = false;
    overflow = Overflow::Hidden; // 内容超出面板时裁剪，不画到面板外面
    padding = EdgeInsets::All(10.0f);
    background = Theme::kBgSideBar;
    corner_radius = Theme::kRadius;
    border = BorderStyle{1.0f, Theme::kBorder, 0.0f};
}

void PropertyPanel::Clear() {
    sections_.clear();
}

void PropertyPanel::AddSection(std::string title) {
    Section section;
    section.title = std::move(title);
    sections_.push_back(std::move(section));
}

void PropertyPanel::AddRow(std::string name, std::string value, bool highlight) {
    PropRow row;
    row.name = std::move(name);
    row.value = std::move(value);
    row.highlight = highlight;
    Section* target = nullptr;
    for (auto it = sections_.rbegin(); it != sections_.rend(); ++it) {
        target = &(*it);
        break;
    }
    if (target == nullptr) {
        AddSection("");
        target = &sections_.back();
    }
    target->rows.push_back(std::move(row));
}

ImVec2 PropertyPanel::MeasureContent(const ImVec2& available) {
    (void)available;
    // 面板尺寸由页面给定
    return ImVec2(320.0f, 220.0f);
}

void PropertyPanel::OnDrawContent(ImDrawList* dl, const Rect& content) {
    const float scale = DrawScale();
    const float font = (font_size > 0.0f ? font_size : Theme::kFontSmall) * scale;
    const float row_h = row_height * scale;
    const float title_gap = this->title_gap * scale;
    const float section_gap = this->section_gap * scale;
    const float column_gap = this->column_gap * scale;

    const float column_width = two_columns ? (content.Width() - column_gap) * 0.5f : content.Width();
    const int column_count = two_columns ? 2 : 1;
    float cursor_y[2] = {content.min.y, content.min.y};
    const float column_x[2] = {content.min.x, content.min.x + column_width + column_gap};

    // 简单装箱：把每组放到当前较矮的那一列
    for (const Section& section : sections_) {
        int column = 0;
        if (column_count == 2 && cursor_y[1] < cursor_y[0]) {
            column = 1;
        }
        float y = cursor_y[column];
        const float x = column_x[column];

        if (!section.title.empty()) {
            Draw::Text(dl, nullptr, font, ImVec2(x, y), Tint(section_title_color), section.title.c_str());
            y += font + title_gap;
        }
        for (const PropRow& row : section.rows) {
            if (row.highlight) {
                const ImU32 bg = highlight_bg != 0 ? highlight_bg : Theme::Alpha(Theme::kTeal, 0.14f);
                Draw::RoundedRectFilled(dl, Rect::FromPosSize(ImVec2(x - 4.0f * scale, y - 1.0f * scale),
                                                             ImVec2(column_width + 8.0f * scale, row_h)),
                                        Tint(bg), 3.0f * scale, 3.0f * scale, 3.0f * scale, 3.0f * scale);
            }
            const float name_width = column_width * name_ratio;
            const char* name = Draw::Ellipsize(nullptr, font, row.name.c_str(), name_width - 6.0f * scale);
            Draw::Text(dl, nullptr, font, ImVec2(x, y), Tint(name_color), name);

            const char* value = Draw::Ellipsize(nullptr, font, row.value.c_str(), column_width - name_width - 6.0f * scale);
            const float value_width = Draw::MeasureText(nullptr, font, value, 0.0f).x;
            Draw::Text(dl, nullptr, font, ImVec2(x + column_width - value_width, y),
                       Tint(row.highlight ? highlight_color : value_color), value);
            y += row_h;
        }
        cursor_y[column] = y + section_gap;
    }
}

} // namespace gui_dev::cv
