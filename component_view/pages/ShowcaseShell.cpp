#include "component_view/pages/ShowcaseShell.h"

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Label.h"
#include "component_view/components/PropertyPanel.h"
#include "component_view/components/TabBar.h"
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

constexpr float kTabColumnWidth = 236.0f;
constexpr float kContentX = kTabColumnWidth + 24.0f;
constexpr float kContentWidth = 1240.0f - kContentX;

} // namespace

void ShowcaseShell::OnBuild() {
    pages_ = CreateControlPages();

    // ---- 左侧 Tab 列 -------------------------------------------------------
    Box* tab_column = Root().Emplace<Box>("tab_column");
    tab_column->Surface();
    tab_column->SetPosition(0.0f, 0.0f);
    tab_column->SetSize(kTabColumnWidth, 634.0f);
    tab_column->padding = EdgeInsets::All(8.0f);

    tabs_ = tab_column->Emplace<TabBar>("tab_bar");
    tabs_->focus_zone = 1; // 左列 = 分区 1
    tabs_->SetSize(0.0f, 634.0f - 16.0f);
    tabs_->tab_size = ImVec2(0.0f, 36.0f);
    tabs_->immediate = false; // A 才切换，光标移动只移动焦点（更像主机 UI）
    for (const auto& page : pages_) {
        tabs_->AddTab(page->Name(), page->Icon());
    }
    connect(tabs_, &TabBar::currentChanged, this, [this](int index) { SelectTab(index, true); });

    // ---- 右侧页头 ----------------------------------------------------------
    title_label_ = Root().Emplace<Label>("", Theme::kFontTitle, Theme::kTextBright);
    title_label_->SetPosition(kContentX, 0.0f);
    title_label_->SetSize(kContentWidth, 42.0f);

    summary_label_ = Root().Emplace<Label>("", Theme::kFontSmall, Theme::kTextMuted);
    summary_label_->SetPosition(kContentX, 42.0f);
    summary_label_->SetSize(kContentWidth, 22.0f);

    // ---- 展示区 ------------------------------------------------------------
    showcase_ = Root().Emplace<Box>("showcase");
    showcase_->SetPosition(kContentX, 70.0f);
    showcase_->SetSize(kContentWidth, 312.0f);
    showcase_->SetBackground(Theme::kBgPanel);
    showcase_->SetBorder(1.0f, Theme::kBorder);
    showcase_->SetRadius(Theme::kRadius);
    showcase_->SetPadding(EdgeInsets::All(20.0f));
    showcase_->layout = LayoutMode::Vertical;
    showcase_->gap = ImVec2(0.0f, 14.0f);
    showcase_->align_x = Align::Center;
    showcase_->align_y = Align::Center;
    showcase_->overflow = Overflow::Hidden;

    // ---- 属性面板 ----------------------------------------------------------
    properties_ = Root().Emplace<PropertyPanel>();
    properties_->SetName("properties");
    properties_->SetPosition(kContentX, 392.0f);
    properties_->SetSize(kContentWidth, 242.0f);

    // 默认选中第一个控件页
    SelectTab(0, false);
    tabs_->SetIndex(0, false);
    tabs_->SetCursor(0, false);
    Global::SetFocus(tabs_);
}

ControlPage* ShowcaseShell::ActivePage() const {
    if (active_ < 0 || active_ >= static_cast<int>(pages_.size())) {
        return nullptr;
    }
    return pages_[static_cast<std::size_t>(active_)].get();
}

void ShowcaseShell::SelectTab(int index, bool move_focus_to_content) {
    if (pages_.empty()) {
        return;
    }
    const int clamped = static_cast<int>(
        Clampf(static_cast<float>(index), 0.0f, static_cast<float>(pages_.size()) - 1.0f));
    if (clamped != active_) {
        if (ControlPage* previous = ActivePage()) {
            previous->OnLeave();
        }
        active_ = clamped;
        ControlPage* page = ActivePage();
        showcase_->Clear();
        Global::modal = nullptr;
        Overlay().Clear();
        Overlay().background = 0;
        Overlay().visible = false;
        page->SetHost(showcase_);
        page->Build(showcase_, ui());
        showcase_->SetFocusZone(2); // 右侧内容 = 分区 2
        page->BuildOverlay(&Overlay());
        page->OnEnter();
        title_label_->text = page->Title();
        summary_label_->text = page->Summary();
    }
    RefreshHints();
    if (move_focus_to_content) {
        // 焦点交到展示区第一个可聚焦控件
        if (Widget* first = showcase_->FirstFocusable()) {
            Global::SetFocus(first);
        } else {
            Global::SetFocus(tabs_);
        }
    } else {
        Global::SetFocus(tabs_);
    }
}

void ShowcaseShell::RefreshHints() {
    ClearHints();
    ControlPage* page = ActivePage();
    if (page != nullptr) {
        for (const auto& hint : page->Navigation()) {
            AddHint(hint.first, hint.second);
        }
    }
    AddHint(Icons::Button::L, "翻页/分区");
    AddHint(Icons::Button::Minus, "回到标签");
}

void ShowcaseShell::OnInput() {
    ControlPage* page = ActivePage();
    if (page == nullptr) {
        return;
    }
    // 只处理控件没有消费掉的按键（控件优先，页面兜底）
    for (InputAction action : {InputAction::Cancel, InputAction::Minus}) {
        if (Global::pad.Pressed(action) && Global::Available(action)) {
            // 回到左侧标签列
            Global::SetFocus(tabs_);
            Global::MarkConsumed(action);
            (void)page;
        }
    }
}

void ShowcaseShell::OnUpdate(float dt) {
    ControlPage* page = ActivePage();
    if (page == nullptr) {
        return;
    }
    page->OnUpdate(dt);

    // 属性面板每帧刷新：页面只描述状态，排版由 PropertyPanel 负责
    std::vector<PropSection> sections;
    page->FillProperties(sections);
    properties_->Clear();
    for (const PropSection& section : sections) {
        properties_->AddSection(section.title);
        for (const PropRow& row : section.rows) {
            properties_->AddRow(row.name, row.value, row.highlight);
        }
    }
}

} // namespace gui_dev::cv
