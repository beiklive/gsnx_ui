// VirtualKeyboard：自绘虚拟键盘（一等公民，不是 Input 的附属 Demo）。
//
// 全部由手柄操作，**绝不调用系统键盘**：
//   ↑ ↓ ← →   在按键之间移动焦点（行内错位也用最近邻，不会跳错）
//   A         输入当前字符 / 触发功能键
//   B         取消（on_cancel）
//   X         退格     Y         空格
//   L / R     切换字符页（字母 / 符号 / 数字）
//   ZL / ZR   切换 Shift（大小写） / 循环字符页
//   +         直接确定    -      退格
//
// 功能键：[SHIFT][SPACE][◀][▶][⌫][DEL][SEL][CLR][取消][确定]
// 光标移动用 [◀]/[▶]，[SEL] 全选（之后输入/删除会替换选区）。
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class VirtualKeyboard : public Widget {
public:
    enum class Page { Letters, Symbols, Numbers };

    struct Key {
        enum class Kind { Character, Shift, Space, CaretLeft, CaretRight, Backspace, Delete, SelectAll, Clear, Cancel, Confirm };
        Kind kind = Kind::Character;
        std::string label;  // 显示文本
        std::string insert; // Character 输入的内容（未考虑 Shift）
        Rect rect;          // 布局矩形（本帧计算）
        int row = 0;
        int column = 0;
    };

    VirtualKeyboard();

    std::string prompt;
    std::string buffer;
    int max_length = 32;
    bool password = false;
    bool shift = false;
    Page page = Page::Letters;
    bool show_preview = true;
    bool loop_columns = false;

    float key_height = 42.0f;
    float key_gap = 6.0f;
    float preview_height = 54.0f;
    int columns = 10;

    ImU32 panel_bg = Theme::kBgSideBar;
    ImU32 key_bg = Theme::kKeyBg;
    ImU32 key_bg_focus = Theme::kKeyBgActive;
    ImU32 key_bg_function = Theme::kKeyBgTop;
    ImU32 key_text = Theme::kTextPrimary;
    ImU32 key_text_focus = Theme::kTextBright;
    ImU32 preview_bg = Theme::kBgInput;

    std::function<void(const std::string&)> on_accept;
    std::function<void()> on_cancel;
    std::function<void(const std::string&)> on_changed;

    // ---- 状态 API ----------------------------------------------------------
    void SetInitial(std::string text);
    void SetPrompt(std::string value);
    void Reset();
    const std::vector<Key>& Keys() const { return keys_; }
    int CursorIndex() const { return cursor_; }
    void SetCursorIndex(int value);
    int Caret() const { return caret_; }
    int SelectionStart() const { return selection_start_; }
    int SelectionEnd() const { return selection_end_; }
    std::string PageName() const;
    std::string DisplayBuffer() const;

    // ---- 文本编辑（虚拟键盘与 InputField 共用同一套语义） ------------------
    void InsertText(const std::string& value);
    void Backspace();
    void DeleteForward();
    void MoveCaret(int delta);
    void SelectAll();
    void ClearBuffer();
    bool HasSelection() const { return selection_start_ >= 0 && selection_end_ > selection_start_; }

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    bool OnPadAction(InputAction action) override;

private:
    void BuildLayout(const Rect& content);
    void MoveFocus(int dx, int dy);
    void ActivateKey(const Key& key);
    void FocusKey(int index, bool animate = true);
    int CharCount() const;
    void ClampBuffer();

    std::vector<Key> keys_;
    int cursor_ = 0;
    int caret_ = 0;
    int selection_start_ = -1;
    int selection_end_ = -1;
    float press_mix_ = 0.0f;
    float focus_x_ = 0.0f; // 焦点框平滑位置
    float focus_y_ = 0.0f;
    float focus_w_ = 0.0f;
    float focus_h_ = 0.0f;
    float flash_ = 0.0f;
    std::string last_char_;
    bool layout_dirty_ = true;
    ImVec2 layout_size_{0.0f, 0.0f};
};

} // namespace gui_dev::cv
