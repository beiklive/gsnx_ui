#include "component_view/pages/ControlPage.h"

#include <cmath>
#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/components/Label.h"
#include "component_view/components/Progress.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"

namespace gui_dev::cv {

void ProgressPage::Build(Widget* host, UiContext& ui) {
    (void)ui;
    Box* row = helpers::Row(host, 22.0f);
    row->SetSize(0.0f, 190.0f);
    row->align_y = Align::Start;

    Box* column = helpers::Column(row, 16.0f);
    column->SetSize(520.0f, 190.0f);
    column->align_x = Align::Start;

    bars_.clear();
    Progress* determinate = column->Emplace<Progress>(0.0f, 0.0f, 100.0f);
    determinate->SetName("progress_determinate");
    determinate->SetSize(500.0f, 26.0f);
    determinate->SetLabel("下载进度");
    determinate->show_percentage = true;
    bars_.push_back(determinate);

    Progress* install = column->Emplace<Progress>(0.0f, 0.0f, 100.0f);
    install->SetName("progress_install");
    install->SetSize(500.0f, 26.0f);
    install->SetLabel("安装到 SD 卡");
    install->fill_color = Theme::kTeal;
    install->show_percentage = true;
    bars_.push_back(install);

    Progress* indeterminate = column->Emplace<Progress>(0.0f, 0.0f, 100.0f);
    indeterminate->SetName("progress_indeterminate");
    indeterminate->SetSize(500.0f, 26.0f);
    indeterminate->SetLabel("扫描游戏库");
    indeterminate->indeterminate = true;
    indeterminate->show_percentage = false;
    bars_.push_back(indeterminate);

    Box* right = helpers::Column(row, 14.0f);
    right->SetSize(300.0f, 190.0f);
    right->align_x = Align::Start;

    Progress* vertical = right->Emplace<Progress>(0.0f, 0.0f, 100.0f);
    vertical->SetName("progress_vertical");
    vertical->SetSize(40.0f, 130.0f);
    vertical->vertical = true;
    vertical->thickness = 22.0f;
    vertical->show_percentage = true;
    bars_.push_back(vertical);

    toggle_ = right->Emplace<Button>("暂停 / 继续");
    toggle_->SetName("progress_toggle");
    toggle_->Secondary().FitContent(18.0f, 44.0f);
    toggle_->SetIcon(Icons::Glyph(Icons::Button::A));
    toggle_->on_click = [this](Widget&) { running_ = !running_; };

    status_ = helpers::Caption(host, "", Theme::kFontSmall, Theme::kTeal);
}

void ProgressPage::OnUpdate(float dt) {
    elapsed_ += dt;
    if (!running_) {
        return;
    }
    // 三条进度条用不同速度推进，演示平滑动画
    value_ += dt * 9.0f;
    if (value_ > 100.0f) {
        value_ = 0.0f;
    }
    if (bars_.size() >= 4) {
        bars_[0]->SetValue(value_);
        bars_[1]->SetValue(std::fmod(value_ * 0.7f + 20.0f, 100.0f));
        bars_[3]->SetValue(value_);
        // bars_[2] 是不定态，不需要 value
    }
    if (status_ != nullptr) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "更新中：%.0f%% · %s · 平滑速度 8/s", value_, running_ ? "运行" : "暂停");
        status_->text = buffer;
    }
}

std::vector<std::pair<Icons::Button, std::string>> ProgressPage::Navigation() const {
    return {{Icons::Button::Down, "切到按钮"},
            {Icons::Button::A, "暂停 / 继续"},
            {Icons::Button::B, "返回标签"}};
}

void ProgressPage::FillProperties(std::vector<PropSection>& out) const {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.0f%%", value_);
    PushSection(out, "Value", {
                               Row("Value", buffer, true),
                               Row("Min / Max", "0 / 100"),
                               Row("Percentage", "显示"),
                               Row("Indeterminate", "扫描游戏库（不定态）", true),
                           });
    PushSection(out, "Layout", {
                               Row("Horizontal", "500x26"),
                               Row("Vertical", "40x130"),
                               Row("Rounded", "是（半径 = 高度/2）"),
                               Row("Fill / Track", "kTrackFill / kTrack"),
                           });
    PushSection(out, "Animation", {
                                   Row("Smoothing", "指数平滑 8/s", true),
                                   Row("Indeterminate", "0.55 屏/秒"),
                                   Row("State", running_ ? "运行中" : "已暂停", true),
                               });
    PushSection(out, "Navigation", {
                                     Row("Down → A", "切到按钮并暂停/继续"),
                                     Row("B", "回到左侧标签列"),
                                 });
}

} // namespace gui_dev::cv
