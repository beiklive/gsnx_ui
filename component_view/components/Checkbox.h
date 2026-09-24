// Checkbox / Radio：手柄开关控件。
//
// Checkbox：A 或 X 切换；勾选动画；焦点颜色变化；Disabled。
// RadioGroup：整组是一个焦点停靠点，↑↓ 移动高亮，A 选定（手柄 UI 的常见做法）。
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Checkbox : public Widget {
public:
    Checkbox();
    explicit Checkbox(std::string label, bool checked = false);

    std::string label;
    bool checked = false;
    float box_size = 26.0f;
    float label_gap = 12.0f;
    float font_size = 0.0f;
    ImU32 box_bg = Theme::kBgWidget;
    ImU32 box_bg_checked = Theme::kAccent;
    ImU32 box_border = Theme::kBorderStrong;
    ImU32 check_color = Theme::kTextBright;
    ImU32 label_color = Theme::kTextPrimary;
    ImU32 label_color_focus = Theme::kTextBright;
    float transition_speed = 16.0f;

    std::function<void(Checkbox&, bool)> on_changed;

    Checkbox& SetLabel(std::string value);
    Checkbox& SetChecked(bool value, bool notify = false);
    Checkbox& Toggle(bool notify = true);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;
    void Activate() override;

private:
    float check_mix_ = 0.0f;
    float hover_mix_ = 0.0f;
    void Apply(bool value, bool notify);
};

// 单选组：一个焦点停靠点 + 高亮光标 + 已选项
class RadioGroup : public Widget {
public:
    RadioGroup();
    explicit RadioGroup(std::string widget_name);

    struct Option {
        std::string text;
        std::string detail;
        bool disabled = false;
    };

    bool vertical = true;
    float row_height = 40.0f;
    float dot_size = 24.0f;
    float label_gap = 12.0f;
    float font_size = 0.0f;
    bool loop = true;
    ImU32 dot_bg = Theme::kBgWidget;
    ImU32 dot_border = Theme::kBorderStrong;
    ImU32 dot_selected = Theme::kAccent;
    ImU32 label_color = Theme::kTextPrimary;
    ImU32 label_color_focus = Theme::kTextBright;
    ImU32 highlight_bg = Theme::kListRowFocus;

    std::function<void(RadioGroup&, int)> on_changed;

    RadioGroup& AddOption(std::string text, std::string detail = std::string(), bool disabled = false);
    int OptionCount() const { return static_cast<int>(options_.size()); }
    int Value() const { return value_; }
    int Cursor() const { return cursor_; }
    RadioGroup& SetValue(int value, bool notify = false);
    RadioGroup& SetCursor(int value, bool ensure_visible = true);
    Rect OptionRect(int index) const;

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;
    void OnAfterLayout() override;

private:
    std::vector<Option> options_;
    std::vector<float> select_mix_;
    int value_ = 0;
    int cursor_ = 0;
    float cursor_anim_ = 0.0f;
    float highlight_mix_ = 0.0f;
};

} // namespace gui_dev::cv
