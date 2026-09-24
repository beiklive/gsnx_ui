// Dialog：模态对话框（模拟器 UI 很常用：保存状态、覆盖确认、错误提示）。
//
//   ← →     在按钮之间切换（最近邻，按钮竖排时用 ↑↓）
//   A       确认当前按钮
//   B       取消（等价于点 cancel_button）
//
// 支持：Modal + 遮罩、Focus Trap（打开时 Global::modal 指向自己，页面只收集它的焦点）、
// 按钮组、默认按钮、取消按钮、进入/退出动画、居中或自定义位置。
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class Dialog : public Widget {
public:
    Dialog();
    explicit Dialog(std::string title, std::string message = std::string());

    std::string title;
    std::string message;
    std::string icon;      // 左侧图标（Material 字形）
    std::vector<std::string> buttons{"取消", "确定"};
    int default_button = 1;
    int cancel_button = 0;
    bool dismiss_on_cancel = true; // B 是否直接关闭
    bool vertical_buttons = false;

    float button_width = 132.0f;
    float button_height = Theme::kControlHeight;
    float button_gap = 12.0f;
    float animation_speed = 12.0f;
    ImU32 panel_bg = Theme::kBgSideBar;
    ImU32 title_color = Theme::kTextBright;
    ImU32 message_color = Theme::kTextPrimary;
    ImU32 button_color = Theme::kButton;
    ImU32 button_focus_color = Theme::kAccentHover;
    ImU32 cancel_color = Theme::kBgWidget;

    // 打开/关闭时回调（result = 按钮索引，-1 表示取消退出）
    std::function<void(Dialog&, int)> on_result;

    void Open();
    void Close(int result = -1);
    bool IsOpen() const { return open_; }
    int FocusedButton() const { return cursor_; }
    void SetFocusedButton(int value);
    Rect ButtonRect(int index) const;

    Dialog& SetTitle(std::string value);
    Dialog& SetMessage(std::string value);
    Dialog& SetIcon(std::string glyph);
    Dialog& SetButtons(std::vector<std::string> labels, int default_index = 0, int cancel_index = 0);

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;

private:
    bool open_ = false;
    float open_mix_ = 0.0f; // 0 = 关闭，1 = 完全打开
    int cursor_ = 1;
    float cursor_anim_ = 0.0f;
    float press_mix_ = 0.0f;
};

} // namespace gui_dev::cv
