// TabBar：左侧/顶部标签栏（Demo 本身的主导航）。
//
// 一个 TabBar = 一个焦点停靠点 + 内部光标；指示条带滑动动画。
//   ↑ ↓ / ← →   移动光标（方向由 orientation 决定）
//   A           选中当前项（immediate = true 时移动即切换）
//   L / R       上一页 / 下一页（wrap_pages）
//   ZL / ZR     第一项 / 最后一项
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class TabBar : public Widget {
public:
    enum class Orientation { Vertical, Horizontal };

    struct Tab {
        std::string text;
        std::string icon;
        bool disabled = false;
    };

    TabBar();
    explicit TabBar(std::string widget_name);

    Orientation orientation = Orientation::Vertical;
    ImVec2 tab_size{0.0f, 38.0f}; // width=0 表示撑满
    float gap = 4.0f;
    float indicator_width = 4.0f;
    float icon_gap = 10.0f;
    float font_size = 0.0f;
    bool immediate = true; // 移动光标立即切换
    bool wrap_pages = true;
    float transition_speed = 18.0f;

    ImU32 tab_color = 0;
    ImU32 tab_cursor_color = Theme::kListRowFocus;
    ImU32 tab_active_color = Theme::kSelection;
    ImU32 text_color = Theme::kTextPrimary;
    ImU32 text_color_focus = Theme::kTextBright;
    ImU32 text_color_active = Theme::kTextBright;
    ImU32 indicator_color = Theme::kAccent;

    std::function<void(TabBar&, int)> on_changed;

    TabBar& AddTab(std::string text, std::string icon = std::string(), bool disabled = false);
    int TabCount() const { return static_cast<int>(tabs_.size()); }
    int Index() const { return index_; }
    int Cursor() const { return cursor_; }
    TabBar& SetIndex(int value, bool notify = false);
    TabBar& SetCursor(int value, bool ensure_visible = true);
    Rect TabRect(int index) const;
    int VisiblePageSize() const;

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;
    void OnAfterLayout() override;

private:
    std::vector<Tab> tabs_;
    int index_ = 0;
    int cursor_ = 0;
    float cursor_anim_ = 0.0f;
    float focus_mix_local_ = 0.0f;
    float section_shift_ = 0.0f; // 翻页时指示条的整体动画偏移
};

} // namespace gui_dev::cv
