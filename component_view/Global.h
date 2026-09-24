// 全局变量：整个组件库共享的运行时状态。
//
// 组件在 OnDrawContent / OnUpdate 里直接读 Global::xxx，不需要一路传参；
// 每帧由宿主（demo.cpp / Page::Update）调用 BeginFrame / EndFrame 刷新。
#pragma once

#include <vector>

#include <imgui.h>

#include "component_view/Types.h"
#include "platform/Input.h"

namespace gui_dev {
class UiContext;
}

namespace gui_dev::cv {

class Widget;

namespace Global {

// ---- 画布 ----------------------------------------------------------------
// 当前 UI 逻辑区域（720p 设计空间）：后端把 drawable 除以 UiScale 得到它。
inline ImVec2 canvas_pos{0.0f, 0.0f};
inline ImVec2 canvas_size{1280.0f, 720.0f};
inline ImDrawList* draw_list = nullptr;

// ---- 帧信息 --------------------------------------------------------------
inline float delta_time = 1.0f / 60.0f;
inline int frame_index = 0;
inline float ui_scale = 1.0f;
inline bool compact = false;
inline const char* platform_name = "Desktop";

// ---- 鼠标 ----------------------------------------------------------------
inline ImVec2 mouse{-FLT_MAX, -FLT_MAX};
inline bool mouse_down[3]{false, false, false};
inline bool mouse_pressed[3]{false, false, false};
inline bool mouse_released[3]{false, false, false};
inline bool mouse_available = false;

// ---- 手柄 / 键盘（后端抽象后的动作） --------------------------------------
inline PadState pad;

// ---- 输入焦点 ------------------------------------------------------------
inline Widget* hovered = nullptr; // 鼠标下的组件
inline Widget* pressed = nullptr; // 鼠标按下时锁定的组件
inline Widget* focused = nullptr; // 手柄/键盘焦点
inline Widget* active = nullptr;  // 正在激活（拖拽/长按）的组件
inline Widget* modal = nullptr;   // 当前模态（Dialog/虚拟键盘）：焦点只在这个子树里流动

// ---- 输入消费 -------------------------------------------------------------
// 一帧里同一个按键只能被消费一次：页面/控件处理后 MarkConsumed，
// 后面的接收者（包括同一帧内刚拿到焦点的控件）就不会重复响应。
inline bool consumed[kInputActionCount] = {};
bool Available(InputAction action);
void MarkConsumed(InputAction action);

// ---- 组件 ID -------------------------------------------------------------
inline WidgetId next_widget_id = 1;

// 采样一帧的输入与画布。宿主每帧调用一次，必须在 Page::Update 之前。
void BeginFrame(UiContext& ui);
// 清理一次性事件（clicked / pressed / released）。
void EndFrame();

// ---- 焦点 -----------------------------------------------------------------
void SetFocus(Widget* widget);
// 手柄方向键 / 摇杆移动焦点，Confirm 触发点击。
void NavigateFocus(const std::vector<Widget*>& focusables);

// ---- 小工具 ---------------------------------------------------------------
// 画布矩形
inline Rect CanvasRect() { return Rect::FromPosSize(canvas_pos, canvas_size); }

} // namespace Global

} // namespace gui_dev::cv
