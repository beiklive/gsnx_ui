// Choice：选择类基础控件 —— Checkbox（复选框）与 RadioGroup（单选组）。
//
// 为什么需要它们（而不是复用现有的 ToggleButton / OptionButton）：
//   * ToggleButton 是「右对齐滑块开关」的列表行形态，适合设置项；
//   * OptionButton 是「左标题 + 右 [L] 值 [R]」的 L/R 选择器；
//   * Checkbox / Radio 是「方框/圆点 + 文字」的表单形态，弹窗和设置页里最常见的一种排版，
//     多个并列时（例如「覆盖 / 跳过 / 取消」三选一）用 RadioGroup 比一排按钮更省空间。
// 三者视觉与语义不重叠，所以各自独立；布局、焦点、命中测试仍然全部复用 Widget 基类。
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

// --------------------------------------------------------------- Checkbox ----
// 复选框：方框 + 勾选动画 + 文字。clicked / toggled 都会发。
class Checkbox : public Widget {
public:
    Checkbox();
    explicit Checkbox(std::string caption);

    std::string caption;
    bool checked = false;
    float box_size = 26.0f;
    float gap = 10.0f;              // 方框到文字的间距
    float font_size = 0.0f;         // 0 = Theme::kFontBody
    float corner = 5.0f;
    ImVec4 text_color = Theme::kTextPrimary;
    bool text_color_follows_theme = true;
    ImVec4 check_color{0.0f, 0.0f, 0.0f, 0.0f}; // alpha=0 -> Theme::kAccent
    float animation_speed = 16.0f;

    Checkbox& setChecked(bool value, bool notify = true);
    Checkbox& setCaption(std::string value);
    Checkbox& setBoxSize(float value);
    Checkbox& setColors(ImVec4 box_fill, ImVec4 text);

signals:
    Signal<bool> toggled; // 状态变化（带新值）

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    void Activate() override;
    void OnThemeChanged() override;

private:
    float check_mix_ = -1.0f; // <0 = 还没初始化：第一帧直接对齐 checked，不播动画
};

// ------------------------------------------------------------- RadioGroup ----
// 单选组：一排（或一列）单选行，内部自己导航，对外只暴露 index 与信号。
// 复合控件语义：焦点落在组本身（focus_only_self），方向键在组内切换，另一方向键离开。
class RadioGroup : public Widget {
public:
    enum class Orientation { Horizontal, Vertical };

    RadioGroup();
    explicit RadioGroup(std::vector<std::string> options);

    std::vector<std::string> options;
    int index = 0;
    Orientation orientation = Orientation::Vertical;
    bool wrap = false;               // 到头是否绕回
    float item_height = 44.0f;
    float item_gap = 8.0f;
    float marker_size = 22.0f;
    float label_gap = 10.0f;
    float font_size = 0.0f;
    ImVec4 text_color = Theme::kTextPrimary;
    bool text_color_follows_theme = true;
    ImVec4 mark_color{0.0f, 0.0f, 0.0f, 0.0f}; // alpha=0 -> Theme::kAccent
    float animation_speed = 16.0f;
    float horizontal_item_width = 150.0f;

    RadioGroup& setOptions(std::vector<std::string> values, int start_index = 0);
    RadioGroup& setIndex(int value, bool notify = true);
    RadioGroup& setOrientation(Orientation value);
    RadioGroup& setColors(ImVec4 mark, ImVec4 text);
    const char* currentOption() const;
    int count() const { return static_cast<int>(options.size()); }

signals:
    Signal<int> selectionChanged; // 选中项变化
    Signal<int> activated;        // A 键 / 再次点击已选中项

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;
    void Activate() override;
    void OnThemeChanged() override;

private:
    void SelectAt(int next, bool notify);
    Rect ItemRect(const Rect& content, int item) const;
    void SyncCapture();

    float mark_mix_ = -1.0f; // 选中圆点的出现动画（换项时从 0 长到 1）
    std::vector<Rect> item_rects_;
};

} // namespace gui_dev::cv
