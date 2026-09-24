// Slider：手柄滑条。
//
//   ← →        调整（步进 = step）
//   L / R      快速调整（fast_step）
//   ZL / ZR    大步调整（page_step）
//   A          确认（把当前值写回并触发 on_changed）
//   B          取消（回到聚焦时的值）
// 视觉：Track / Fill / Thumb / 焦点放大 / 数值百分比 / 平滑数值动画。
#pragma once

#include <functional>
#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Slider : public Widget {
public:
    Slider();
    explicit Slider(std::string label, float value = 0.0f, float min_value = 0.0f, float max_value = 100.0f);

    std::string label;
    float value = 50.0f;
    float min_value = 0.0f;
    float max_value = 100.0f;
    float step = 1.0f;
    float fast_step = 10.0f;
    float page_step = 25.0f;
    bool vertical = false;
    bool show_value = true;
    bool show_percent = true;
    bool revert_on_cancel = true;

    float track_thickness = 6.0f;
    float thumb_radius = 8.0f;
    float label_width = 0.0f; // 0 = 自动（按 label 宽度）
    ImU32 track_color = Theme::kTrack;
    ImU32 fill_color = Theme::kTrackFill;
    ImU32 thumb_color = Theme::kTextBright;
    ImU32 label_color = Theme::kTextPrimary;
    ImU32 value_color = Theme::kTextMuted;

    std::function<void(Slider&, float)> on_changed;

    Slider& SetRange(float min_value, float max_value);
    Slider& SetValue(float next, bool notify = false);
    Slider& SetLabel(std::string value);
    Slider& SetStep(float normal_step, float fast = 10.0f, float page = 25.0f);
    float Percent() const { return max_value > min_value ? (value - min_value) / (max_value - min_value) : 0.0f; }

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;

private:
    void Nudge(float delta, bool notify = true);
    float Clamp(float v) const { return Clampf(v, Minf(min_value, max_value), Maxf(min_value, max_value)); }
    float display_value_ = 0.0f;
    float thumb_mix_ = 0.0f;
    float focus_on_value_ = 0.0f;
    bool adjusting_ = false;
};

} // namespace gui_dev::cv
