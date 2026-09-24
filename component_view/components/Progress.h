// Progress：进度条。加载 / 安装 / 下载 / 扫描 / 解压这些场景。
//
// 只读控件（不可聚焦）：Value / Percentage / 水平竖直 / 圆角 / 填充 / 平滑动画 / 不定态。
#pragma once

#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Progress : public Widget {
public:
    Progress();
    explicit Progress(float value, float min_value = 0.0f, float max_value = 100.0f);

    std::string label;
    float value = 0.0f;
    float min_value = 0.0f;
    float max_value = 100.0f;
    bool vertical = false;
    bool indeterminate = false;
    bool rounded = true;
    bool show_percentage = true;
    bool show_value_text = false;
    float thickness = 9.0f;
    float label_gap = 7.0f;
    float animation_speed = 8.0f;
    float indeterminate_speed = 0.55f; // 每秒跑过的比例
    ImU32 track_color = Theme::kTrack;
    ImU32 fill_color = Theme::kTrackFill;
    ImU32 label_color = Theme::kTextPrimary;
    ImU32 text_color = Theme::kTextBright;

signals:
    Signal<float> valueChanged; // 进度变化（0..1 的比例）
public:
    Progress& SetValue(float next);
    Progress& SetRange(float min_v, float max_v);
    Progress& SetLabel(std::string text);
    float Percent() const { return max_value > min_value ? Clampf((value - min_value) / (max_value - min_value), 0.0f, 1.0f) : 0.0f; }

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;

private:
    float display_percent_ = 0.0f;
    float phase_ = 0.0f; // 不定态相位
};

} // namespace gui_dev::cv
