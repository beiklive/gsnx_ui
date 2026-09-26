// FocusManager：焦点归属的统一管理（作用域栈 + Focus Trap + 焦点保存/恢复）。
//
// 为什么不把焦点状态搬到这里？—— Widget::focused / Global::focused 已经是一等状态，
// 被 FocusRing、UpdateInteraction、信号（focusIn/focusOut）依赖。这里只补「谁有资格拿焦点」：
//
//   PushScope(name, root)  把 root 子树变成唯一可聚焦范围（弹窗打开时用）
//   PopScope()             退出范围，并把焦点恢复到打开前的那个控件
//
// 于是天然得到需求里的两个行为：
//   * Focus Trap：弹窗打开后，背景页面的控件不在作用域内，方向键只能在弹窗内部移动，
//     鼠标 hover / 触摸点击也抢不走焦点（Global::SetFocus 会拒绝作用域外目标）。
//   * 焦点恢复：GameList 里焦点在 Game B → 打开 Confirm → 关闭后焦点回到 Game B。
//
// 与 FocusNavigator 的分工：范围由这里定，**怎么选下一个** 仍然是 Global::NavigateFocus
// 那套空间最近邻算法（已经被手柄/触摸/指针三种输入共用），这里只负责喂给它正确的候选列表。
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class FocusManager {
public:
    // 一层焦点作用域。saved_focus 是「进入这层之前」的焦点，退出时用它恢复。
    struct Scope {
        std::string name;
        Widget* root = nullptr;
        Widget* saved_focus = nullptr;
    };

    void Reset();

    // 压入作用域：root 子树成为唯一可聚焦范围；当前焦点被记下来等 PopScope 恢复
    void PushScope(std::string name, Widget* root);
    // 弹出最内层作用域并恢复焦点（没有作用域时什么都不做）
    void PopScope();
    // 按名字弹出（弹窗可以嵌套，按名字更安全；找不到返回 false）
    bool PopScopeByName(const std::string& name);

    bool Empty() const { return scopes_.empty(); }
    int Depth() const { return static_cast<int>(scopes_.size()); }
    // 最内层作用域的根；nullptr = 没有作用域（整页可聚焦）
    Widget* ScopeRoot() const;
    const std::vector<Scope>& scopes() const { return scopes_; }

    // 该控件当前是否允许持有焦点（无作用域 = 允许；有作用域 = 必须落在最内层子树里）
    bool Allows(const Widget* widget) const;

    // 收集当前作用域内的可聚焦控件；无作用域时用 fallback（页面已经收集好的那份）
    void CollectFocusables(const std::vector<Widget*>& fallback, std::vector<Widget*>& out) const;

    // 焦点交给当前作用域里第一个可聚焦控件（弹窗打开时用）。返回是否有控件可给。
    bool FocusFirst();

    // 调试用：当前作用域链（最外层 → 最内层）
    std::string Describe() const;

private:
    // saved_focus 还活着且还能用吗（切 tab 后旧页控件 visible=false，不算可用）
    static bool Usable(Widget* widget);
    // 弹出一层之后把焦点接回去
    void RestoreAfterPop(const Scope& popped);

    std::vector<Scope> scopes_;
};

} // namespace gui_dev::cv
