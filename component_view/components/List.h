// List：手柄 UI 的核心控件。
//
// 一个 List = 一个焦点停靠点 + 内部索引（不用把每个条目都做成焦点组件），
// 因此「循环、翻页、快速滚动、焦点自动滚动、进入动画」都在一个地方实现。
//
// 交互：
//   ↑ ↓ / ← →   移动焦点（垂直 / 水平 / 网格）
//   A           激活当前项
//   B           不消费（交回页面返回上一级）
//   L / R       上一页 / 下一页
//   ZL / ZR     快速滚动（±5 项）
//   X / Y       第一项 / 最后一项
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class List : public Widget {
public:
    enum class Orientation { Vertical, Horizontal, Grid };

    struct Item {
        std::string text;
        std::string icon;
        std::string detail; // 右侧补充信息（大小、时间…）
        bool disabled = false;
    };

    List();
    explicit List(std::string widget_name);

    Orientation orientation = Orientation::Vertical;
    int columns = 2;                       // Grid 用
    ImVec2 item_size{0.0f, Theme::kListRowHeight}; // width=0 表示撑满
    ImVec2 item_gap{6.0f, 4.0f};
    float item_radius = Theme::kRadiusSmall;
    bool loop = true;
    bool show_index = false;         // 左侧序号
    bool zebra = true;               // 斑马纹
    bool item_enter_animation = true;
    float enter_speed = 4.0f;
    int fast_scroll_items = 5;
    float focus_indicator_width = 3.0f;
    ImU32 row_color = Theme::kListRow;
    ImU32 row_alt_color = Theme::kListRowAlt;
    ImU32 row_focus_color = Theme::kListRowFocus;
    ImU32 row_selected_color = Theme::kSelection;

    std::function<void(List&, int)> on_activate;
    std::function<void(List&, int)> on_focus_changed;

    List& AddItem(std::string text, std::string icon = std::string(), std::string detail = std::string());
    List& SetItemDisabled(int index, bool value = true);
    List& ClearItems();
    int ItemCount() const { return static_cast<int>(items_.size()); }
    const Item& GetItem(int index) const { return items_[static_cast<std::size_t>(index)]; }
    int FocusIndex() const { return index_; }
    List& SetFocusIndex(int value, bool ensure_visible = true);
    int SelectedIndex() const { return selected_; }
    void SetSelectedIndex(int value) { selected_ = value; }
    Rect ItemRect(int index) const;
    Rect IndicatorRect(int index) const; // 焦点左侧指示条（焦点动画用）
    int VisibleItemCount() const;

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;
    void OnAfterLayout() override;

private:
    void MoveFocus(int delta);
    ImVec2 ItemsExtent() const;

    std::vector<Item> items_;
    std::vector<float> enter_mix_;
    int index_ = 0;
    int selected_ = -1;
    float index_anim_ = 0.0f; // 焦点指示条平滑位置（按项号插值）
    float indicator_mix_ = 0.0f;
};

} // namespace gui_dev::cv
