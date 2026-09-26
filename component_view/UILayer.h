// UILayer：统一的 UI 层级（Z-Order）概念。
//
// 目的：不要把「谁画在上面」「谁先收输入」「谁持有焦点」混成一个东西。
// 这里只定义 **渲染层级**；输入优先级与焦点归属分别在 PopupManager / FocusManager 里定义，
// 三者分开维护，改一层不会牵动另一层。
//
// 层级划分（数值即渲染顺序，越小越先画、越靠下）：
//
//     0 ~ 999    Background   背景 / 底板（页面底色、壁纸）
//  1000 ~ 4999   Content      普通 UI（页面组件树、页面 overlay）
//  5000 ~ 9000   Popup        模态弹窗（栈内每层 +kPopupLayerStep，越高越靠上）
//        9900     Focus        焦点框 Overlay（必须在弹窗之上：弹窗里的按钮有焦点时框不能被弹窗压住）
//        9999     Toast        全局通知（视觉最顶层）
//
// 输入优先级（和渲染顺序不同，见 PopupManager）：
//     Popup → Content（Toast 默认不拦输入）
#pragma once

namespace gui_dev::cv {

enum class UILayer : int {
    Background = 0,
    Content = 1000,
    Popup = 5000,
    Focus = 9900,
    Toast = 9999,
};

// 弹窗栈里每深一层加多少（5000 + index * 10；夹在 Popup..Focus 之间）
inline constexpr int kPopupLayerStep = 10;
inline constexpr int kPopupLayerLimit = 9000;

inline constexpr int LayerZ(UILayer layer) { return static_cast<int>(layer); }

// 弹窗栈第 index 层（0 = 最下面的那个弹窗）的渲染层级
inline constexpr int PopupLayerZ(int stack_index) {
    const int z = LayerZ(UILayer::Popup) + stack_index * kPopupLayerStep;
    return z > kPopupLayerLimit ? kPopupLayerLimit : z;
}

// 由数值反查所属层级（调试/断言用）
inline constexpr UILayer LayerOf(int z) {
    if (z >= LayerZ(UILayer::Toast)) {
        return UILayer::Toast;
    }
    if (z >= LayerZ(UILayer::Focus)) {
        return UILayer::Focus;
    }
    if (z >= LayerZ(UILayer::Popup)) {
        return UILayer::Popup;
    }
    if (z >= LayerZ(UILayer::Content)) {
        return UILayer::Content;
    }
    return UILayer::Background;
}

} // namespace gui_dev::cv
