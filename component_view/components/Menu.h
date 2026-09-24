// Menu：游戏内菜单（垂直/水平、嵌套子菜单、分隔符、图标、快捷键提示、分页、LR 导航）。
//
//   ↑ ↓ / ← →   移动光标（方向由 vertical 决定）
//   A           激活：普通项 emit triggered(index)；有子菜单的进入子菜单
//   B           返回上一级子菜单（在根菜单时不消费，交回页面）
//   L / R       切换菜单分区（sections > 1 时显示分页指示）
//   ZL / ZR     第一项 / 最后一项
//
// 这是一个「高级组合控件」：以后接 GBAStation 的游戏运行菜单可以直接用它。
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Menu : public Widget {
public:
    struct Entry {
        std::string label;
        std::string icon;
        std::string shortcut; // 右侧快捷键提示，例如 "ZL"
        std::string value;    // 右侧状态值，例如 "开"
        bool separator = false;
        bool disabled = false;
        bool selected = false;
        int submenu = -1; // 非负表示激活后进入 submenus[submenu]
    };

    Menu();
    explicit Menu(std::string widget_name);

    std::string title;
    bool vertical = true;
    float row_height = 32.0f;
    float gap = 3.0f;
    float row_radius = Theme::kRadiusSmall;
    float icon_gap = 9.0f;
    float font_size = 0.0f;
    bool loop = true;
    bool show_index = false;
    bool show_breadcrumb = true;

    ImU32 row_color = 0;
    ImU32 row_focus_color = Theme::kListRowFocus;
    ImU32 row_selected_color = Theme::kSelection;
    ImU32 text_color = Theme::kTextPrimary;
    ImU32 text_color_focus = Theme::kTextBright;
    ImU32 text_color_disabled = Theme::kTextDisabled;
    ImU32 shortcut_color = Theme::kTextMuted;
    ImU32 indicator_color = Theme::kAccent;

signals:
    Signal<int> triggered;   // A 激活某项
    Signal<int> highlighted; // 光标项变化
public:

    // ---- 构建 --------------------------------------------------------------
    Menu& SetTitle(std::string value);
    Menu& AddEntry(std::string label, std::string icon = std::string(), std::string shortcut = std::string(),
                   std::string value = std::string());
    Menu& AddSeparator();
    Menu& SetEntryDisabled(int index, bool disabled = true);
    Menu& SetEntrySelected(int index, bool selected = true);
    // 进入子菜单：把 entry_index 变成子菜单入口，之后所有 AddEntry 都加进子菜单，
    // 构建完调用 LeaveSubmenu() 回到上一层。
    Menu& EnterSubmenu(int entry_index, std::string submenu_title);
    Menu& LeaveSubmenu();

    // ---- 状态 --------------------------------------------------------------
    int Cursor() const { return cursor_; }
    int Section() const { return section_; }
    int SectionCount() const { return static_cast<int>(sections_.size()); }
    int Depth() const { return static_cast<int>(stack_.size()); }
    int EntryCount() const { return static_cast<int>(CurrentNode().entries.size()); }
    Rect EntryRect(int index) const;
    std::string EntryLabel(int index) const;
    std::string Breadcrumb() const;
    Menu& SetCursor(int value, bool notify = false);
    void ActivateCurrent();
    bool GoBack();

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;
    void OnAfterLayout() override;

private:
    // 菜单节点：顶层分区（L/R 切换）+ 子菜单树（A 进入 / B 返回）
    struct Node {
        std::string title;
        std::vector<Entry> entries;
        std::vector<std::unique_ptr<Node>> children;
    };

    Node& CurrentNode();
    const Node& CurrentNode() const;
    std::vector<Entry>& CurrentEntries();
    // 子菜单面包屑占的高度（根菜单为 0）
    float BreadcrumbHeight() const;

    std::vector<Node> sections_;
    std::vector<Node*> stack_;
    int section_ = 0;
    int cursor_ = 0;
    float cursor_anim_ = 0.0f;
    float focus_mix_local_ = 0.0f;
};

} // namespace gui_dev::cv
