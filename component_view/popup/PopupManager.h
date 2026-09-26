// PopupManager：弹窗栈 + 遮罩 + 输入优先级 + 焦点接管。
//
// 渲染顺序（由 Page::Render 按 UILayer 从下往上调）：
//     Background → 普通 UI(Content) → Popup(5000~9000) → Focus(9900) → Toast(9999)
// 输入顺序（和渲染顺序不同，这是刻意的）：
//     Popup → 普通 UI；Toast 只画不吃输入
// 焦点归属：
//     有弹窗时只有最上层弹窗内部的控件可聚焦（Focus Trap），关闭后自动恢复到打开前的焦点
//
// 用法（页面里）：
//     Popup* p = Popups().ShowConfirm("删除游戏", "确定删除？", [this] { DeleteGame(); });
//     Popups().CloseTop();
//     Popups().CloseAll();
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "component_view/FocusManager.h"
#include "component_view/popup/Popup.h"

namespace gui_dev::cv {

class PopupManager {
public:
    explicit PopupManager(FocusManager* focus);
    ~PopupManager();

    PopupManager(const PopupManager&) = delete;
    PopupManager& operator=(const PopupManager&) = delete;

    // ---- 创建 / 展示 -------------------------------------------------------
    // 接管所有权；弹窗关闭动画播完后由 Advance() 回收
    Popup* Show(std::unique_ptr<Popup> popup);

    // 常用形态：都是「配置好的 Popup」，不是另一套渲染系统
    Popup* ShowInfo(std::string title, std::string message, std::string button_text = "确定");
    Popup* ShowConfirm(std::string title, std::string message, std::function<void()> on_confirm,
                       std::string confirm_text = "确认", std::string cancel_text = "取消");
    Popup* ShowSelection(std::string title, std::string message, std::vector<Popup::ButtonSpec> options,
                         PopupButtonLayout layout = PopupButtonLayout::Vertical);
    Popup* ShowProgress(std::string title, std::string message, bool indeterminate = false);
    Popup* ShowRichText(std::string title, std::vector<RichText::Run> runs, float view_height = 0.0f,
                        PopupKind kind = PopupKind::Info);
    Popup* ShowImage(std::string title, ImTextureRef texture, float width, float height);
    Popup* ShowCustom(std::string title, std::function<void(Widget& content)> builder,
                      PopupKind kind = PopupKind::Custom);

    // ---- 每帧（Page 按下面的顺序调，顺序保证输入优先级正确） ---------------
    // 1) 布局（栈内每一层都摆好）
    void Layout();
    // 2) 命中测试：从最上层往下找；模态弹窗命中不到具体控件时返回它的根（挡住背景 hover）
    Widget* HitTest(const ImVec2& point);
    bool BlocksBackground() const; // 有模态弹窗（且遮罩关闭时也拦）
    bool AnyModal() const;
    // 3) 作用域：把「谁可聚焦」收成最上层弹窗
    void CollectFocusables(const std::vector<Widget*>& fallback, std::vector<Widget*>& out) const;
    // 4) 更新弹窗内部控件（关闭动画中的弹窗不收输入）
    void UpdateTree(float dt);
    // 5) 关闭请求：B 键（在控件拿到按键之后处理）
    void HandleDismiss();
    // 6) 生命周期推进 + 回收已关闭的弹窗（同时弹出焦点作用域）
    void Advance(float dt);
    // 7) 绘制：从栈底到栈顶
    void Draw(ImDrawList* dl);
    // 焦点自动滚动：焦点落在弹窗内的滚动容器里时把它滚进可见区
    bool EnsureVisible(Widget* target);

    // ---- 查询 / 操作 -------------------------------------------------------
    Popup* Top() const;
    Popup* Find(const std::string& name) const;
    int Count() const { return static_cast<int>(popups_.size()); }
    bool Empty() const { return popups_.empty(); }
    void CloseTop();
    void Close(const std::string& name);
    void CloseAll();
    // 页面切换：只关掉 Scope::Page 的弹窗，Global 的保留
    void ClosePageScoped();

    void RefreshTheme();

    PopupStyle& defaults() { return defaults_; }
    const PopupStyle& defaults() const { return defaults_; }

private:
    void FocusPopup(Popup& popup);
    void PushScopeFor(Popup& popup);
    void RebuildScopes();
    void RemoveClosed();

    std::vector<std::unique_ptr<Popup>> popups_;
    FocusManager* focus_ = nullptr;
    PopupStyle defaults_;
    Widget* background_focus_ = nullptr; // 第一个弹窗打开前的焦点（整栈关完后恢复它）
};

} // namespace gui_dev::cv
