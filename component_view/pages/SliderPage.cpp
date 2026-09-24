#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Label.h"
#include "component_view/components/Progress.h"
#include "component_view/components/Slider.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void SliderPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* column = helpers::Column(host, 16.0f);
    column->SetSize(700.0f, 214.0f);
    column->align_x = Align::Start;

    volume_ = column->Emplace<Slider>("音量", 65.0f, 0.0f, 100.0f);
    volume_->SetName("slider_volume");
    volume_->SetSize(680.0f, 52.1f);
    volume_->SetStep(1.0f, 10.0f, 25.0f);
    volume_->show_percent = true;
    volume_->on_changed = [this](Slider& slider, float value) {
        if (status_ != nullptr) {
            char buffer[96];
            std::snprintf(buffer, sizeof(buffer), "音量已应用：%d%%", static_cast<int>(value));
            status_->text = buffer;
        }
        if (meter_ != nullptr) {
            meter_->SetValue(value);
        }
    };

    speed_ = column->Emplace<Slider>("快进倍速", 2.0f, 0.5f, 8.0f);
    speed_->SetName("slider_speed");
    speed_->SetSize(680.0f, 52.1f);
    speed_->SetStep(0.5f, 2.0f, 4.0f);
    speed_->show_percent = false; // 显示原始值
    speed_->fill_color = Theme::kOrange;
    speed_->on_changed = [this](Slider& slider, float value) {
        if (status_ != nullptr) {
            char buffer[96];
            std::snprintf(buffer, sizeof(buffer), "倍速已应用：%.1fx", value);
            status_->text = buffer;
        }
    };

    // 与进度条联动，直观看到数值变化
    Box* meter_row = helpers::Row(column, 14.0f);
    meter_row->SetSize(680.0f, 40.0f);
    meter_row->Emplace<Label>("联动", Theme::kFontSmall, Theme::kTextMuted);
    meter_ = meter_row->Emplace<Progress>(65.0f, 0.0f, 100.0f);
    meter_->SetSize(560.0f, 17.9f);
    meter_->show_percentage = true;

    Box* info = helpers::Row(host, 18.0f);
    info->SetSize(0.0f, 34.0f);
    info->Emplace<Label>("← →  1 步 · L R  10 步 · ZL ZR  25 步 · A 应用 · B 回滚到聚焦时的值", Theme::kFontSmall,
                         Theme::kTextMuted);
    status_ = info->AddLabel("", Theme::kFontSmall, Theme::kTeal);
}

void SliderPage::OnUpdate(float dt) {
    (void)dt;
    if (meter_ != nullptr && volume_ != nullptr) {
        meter_->SetValue(volume_->value);
    }
}

std::vector<std::pair<Icons::Button, std::string>> SliderPage::Navigation() const {
    return {{Icons::Button::Left, "减小"},
            {Icons::Button::Right, "增大"},
            {Icons::Button::L, "快速减小"},
            {Icons::Button::R, "快速增大"},
            {Icons::Button::ZL, "大步减小"},
            {Icons::Button::ZR, "大步增大"},
            {Icons::Button::A, "应用"},
            {Icons::Button::B, "取消（回滚）"}};
}

void SliderPage::FillProperties(std::vector<PropSection>& out) const {
    char volume[64] = {};
    char speed[64] = {};
    if (volume_ != nullptr) {
        std::snprintf(volume, sizeof(volume), "%.0f / %.0f (%d%%)", volume_->value, volume_->max_value,
                      static_cast<int>(volume_->Percent() * 100.0f));
    }
    if (speed_ != nullptr) {
        std::snprintf(speed, sizeof(speed), "%.1fx / %.1fx", speed_->value, speed_->max_value);
    }
    PushSection(out, "Value", {
                               Row("音量", volume, true),
                               Row("快进倍速", speed, true),
                               Row("Min / Max", "0~100 / 0.5~8"),
                               Row("Step", "1 / 0.5"),
                               Row("Fast Step (L R)", "10 / 2.0"),
                               Row("Page Step (ZL ZR)", "25 / 4.0"),
                           });
    PushSection(out, "Visual", {
                               Row("Track", "6px 圆角"),
                               Row("Fill", "kTrackFill / kOrange"),
                               Row("Thumb", "8px + 焦点放大 1.22", true),
                               Row("Value Label", "百分比 / 原始值"),
                               Row("Ticks", "步进小于等于 24 档时显示"),
                           });
    PushSection(out, "State", {
                               Row("Focused", Global::focused != nullptr ? Global::focused->name : "(无)", true),
                               Row("Adjusting", "← → 调整中"),
                               Row("Cancel Rollback", "B 键回到聚焦时的值", true),
                           });
    PushSection(out, "Navigation", {
                                     Row("← →", "微调（step）"),
                                     Row("L / R", "快调 · ZL / ZR 大步"),
                                     Row("A / B", "应用 · 取消并回滚"),
                                 });
}

} // namespace gui_dev::cv
