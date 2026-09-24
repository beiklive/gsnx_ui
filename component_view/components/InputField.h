// InputField：文本输入框（**绝不调用系统键盘**）。
//
// 它只负责展示与编辑状态：
//   Text / Placeholder / Prefix / Suffix / MaxLength / Password / ReadOnly / Multiline
//   Cursor / Selection / 光标闪烁
//
// 手柄：
//   A   请求编辑 —— 页面接到 on_edit_requested 后打开 VirtualKeyboard
//   X   清空
//   Y   退格
//   B   取消编辑（退出编辑态）
// 真正的字符输入全部由 VirtualKeyboard 完成。
#pragma once

#include <functional>
#include <string>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class InputField : public Widget {
public:
    InputField();
    explicit InputField(std::string label, std::string text = std::string());

    std::string label;
    std::string placeholder;
    std::string text;
    std::string prefix;
    std::string suffix;
    std::string hint; // 右下角提示（例如 "A 编辑"）

    int max_length = 32;
    bool password = false;
    bool read_only = false;
    bool multiline = false;
    bool editing = false; // 正在被虚拟键盘编辑
    float font_size = 0.0f;
    float label_gap = 8.0f;
    float box_radius = Theme::kRadiusSmall;

    int cursor = 0;            // 字符索引（UTF-8 安全）
    int selection_start = -1;  // -1 表示没有选区
    int selection_end = -1;

    ImU32 box_bg = Theme::kBgInput;
    ImU32 box_border = Theme::kBorder;
    ImU32 box_border_focus = Theme::kAccent;
    ImU32 text_color = Theme::kTextPrimary;
    ImU32 placeholder_color = Theme::kTextMuted;
    ImU32 cursor_color = Theme::kTextBright;
    ImU32 selection_color = Theme::kSelection;

    std::function<void(InputField&)> on_edit_requested;
    std::function<void(InputField&, const std::string&)> on_changed;

    // ---- 文本操作（虚拟键盘直接调用） --------------------------------------
    void SetText(std::string value, bool notify = true);
    void Insert(const std::string& value);
    void Backspace();
    void DeleteForward();
    void MoveCursor(int delta_chars);
    void Clear();
    void SelectAll();
    bool HasSelection() const { return selection_start >= 0 && selection_end > selection_start; }
    std::string SelectedText() const;
    void DeleteSelection();
    // 光标/选区的字符数（不是字节数）
    int CharCount() const;
    int CursorByteOffset() const;

    InputField& SetLabel(std::string value);
    InputField& SetPlaceholder(std::string value);
    InputField& SetPrefix(std::string value);
    InputField& SetSuffix(std::string value);
    InputField& SetMaxLength(int value);
    InputField& SetPassword(bool value);
    InputField& SetReadOnly(bool value);
    float ResolvedFontSize() const { return font_size > 0.0f ? font_size : Theme::kFontBody; }

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;

private:
    float cursor_blink_ = 0.0f;
    float focus_edge_ = 0.0f;
};

} // namespace gui_dev::cv
