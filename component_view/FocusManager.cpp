#include "component_view/FocusManager.h"

#include "component_view/Global.h"

namespace gui_dev::cv {

void FocusManager::Reset() {
    scopes_.clear();
}

bool FocusManager::Usable(Widget* widget) {
    if (widget == nullptr) {
        return false;
    }
    // visible / enabled / focusable 三者缺一不可：切页后旧页控件还活着但隐藏了
    return widget->visible && widget->enabled && widget->focusable;
}

void FocusManager::PushScope(std::string name, Widget* root) {
    Scope scope;
    scope.name = std::move(name);
    scope.root = root;
    scope.saved_focus = Global::focused; // 弹窗关掉要还回去的那个
    scopes_.push_back(std::move(scope));
}

Widget* FocusManager::ScopeRoot() const {
    if (scopes_.empty()) {
        return nullptr;
    }
    return scopes_.back().root;
}

bool FocusManager::Allows(const Widget* widget) const {
    if (scopes_.empty()) {
        return true; // 没有作用域：整页都能拿焦点
    }
    if (widget == nullptr) {
        return false;
    }
    // 从控件往上找，看能不能碰到最内层作用域的根
    const Widget* root = scopes_.back().root;
    if (root == nullptr) {
        return true;
    }
    for (const Widget* node = widget; node != nullptr; node = node->parent) {
        if (node == root) {
            return true;
        }
    }
    return false;
}

void FocusManager::CollectFocusables(const std::vector<Widget*>& fallback, std::vector<Widget*>& out) const {
    out.clear();
    Widget* root = ScopeRoot();
    if (root == nullptr) {
        out = fallback;
        return;
    }
    root->CollectFocusables(out);
}

bool FocusManager::FocusFirst() {
    Widget* root = ScopeRoot();
    if (root == nullptr) {
        return false;
    }
    if (Widget* first = root->FirstFocusable()) {
        Global::SetFocus(first);
        return true;
    }
    return false;
}

void FocusManager::RestoreAfterPop(const Scope& popped) {
    // 1) 优先还原「打开这层之前」的焦点（需求：确认弹窗关掉后回到原来那个控件）
    if (Usable(popped.saved_focus) && Allows(popped.saved_focus)) {
        Global::SetFocus(popped.saved_focus);
        return;
    }
    // 2) 还剩下层（弹窗套弹窗）：焦点落回下一层的第一个可聚焦控件
    if (!scopes_.empty() && FocusFirst()) {
        return;
    }
    // 3) 兜底：清掉失效焦点，等下一次方向键由 NavigateFocus 重新建立初始焦点
    Global::SetFocus(nullptr);
}

void FocusManager::PopScope() {
    if (scopes_.empty()) {
        return;
    }
    const Scope popped = scopes_.back();
    scopes_.pop_back();
    RestoreAfterPop(popped);
}

bool FocusManager::PopScopeByName(const std::string& name) {
    for (std::size_t i = scopes_.size(); i > 0; --i) {
        if (scopes_[i - 1].name == name) {
            // 把这层之上的都弹掉（弹窗按名字关闭时，它上面的子弹窗一起收）
            while (scopes_.size() >= i) {
                PopScope();
            }
            return true;
        }
    }
    return false;
}

std::string FocusManager::Describe() const {
    std::string text;
    for (const Scope& scope : scopes_) {
        if (!text.empty()) {
            text += " > ";
        }
        text += scope.name.empty() ? "(unnamed)" : scope.name;
    }
    return text.empty() ? std::string("(no scope)") : text;
}

} // namespace gui_dev::cv
