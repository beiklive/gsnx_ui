// 全局变量：整个组件库共享的运行时状态。
//
// 组件在 OnDrawContent / OnUpdate 里直接读 Global::xxx，不需要一路传参；
// 每帧由宿主（demo.cpp / Page::Update）调用 BeginFrame / EndFrame 刷新。
#pragma once

#include <functional>
#include <string>
#include <vector>

#include <imgui.h>

#include "component_view/Theme.h"
#include "component_view/Types.h"
#include "platform/Input.h"

namespace gui_dev {
class UiContext;
}

namespace gui_dev::cv {

class Widget;
class FocusManager;
class PopupManager;

namespace Global {

// ---- 画布 ----------------------------------------------------------------
// 当前 UI 逻辑区域（720p 设计空间）：后端把 drawable 除以 UiScale 得到它。
inline ImVec2 canvas_pos{0.0f, 0.0f};
inline ImVec2 canvas_size{1280.0f, 720.0f};
inline ImDrawList* draw_list = nullptr;

// ---- 全局部件样式（所有 Box / Button 建好时套用；单个实例可以再用链式接口覆盖） ----
// 约定：1px 灰白边框、5px 圆角、向右下角的阴影、流光聚焦框与控件留 2px 边距。
struct ComponentStyle {
    // 边框
    ImVec4 border_color = Theme::rgb(190, 190, 195); // 灰白
    float border_width = 1.0f;
    // 圆角
    float corner_radius = 5.0f;
    // 阴影（右下角）
    ImVec2 shadow_offset{4.0f, 4.0f};
    float shadow_blur = 10.0f;
    ImVec4 shadow_color = Theme::rgba(0, 0, 0, 120);
    // 流光聚焦框
    float focus_margin = 2.0f;      // 聚焦框与控件之间的边距
    float focus_width = 3.0f;       // 聚焦框粗细（比原来 2px 再加 1px）
    float focus_flow_speed = 0.28f; // 每秒流动多少圈
    float focus_saturation = 0.75f; // 流光颜色饱和度（0 = 白灰流光）
    float focus_brightness = 1.0f;
    // 内容
    float content_padding = 12.0f; // 内容到边框的留白（图标四周等距就是靠它）
    // LR 选择器（OptionButton / ValueButton）：[L] 固定间隔 [R] 里那一格的宽度
    float lr_slot_width = 90.0f;
    // 上面那一格放不下时，文字横向滚动显示
    float marquee_speed = 26.0f; // px/s
    // ---- 弹窗（Popup）----
    // 弹窗不另立一套视觉：圆角/边框/阴影直接复用上面的 corner_radius / border_* / shadow_*，
    // 这里只放「弹窗特有、但必须统一」的尺寸与遮罩参数。
    float popup_padding = 24.0f;        // 弹窗内边距（header、标题、内容、按钮都在这条内边距之内）
    float popup_header_size = 32.0f;    // Header 左边那个方形图标 Box 的边长
    float popup_header_gap = 12.0f;     // 方形图标 Box 与文字之间的间距
    float popup_gap = 16.0f;            // 弹窗内部各行间距
    float popup_min_width = 300.0f; // 弹窗最小宽度 = 自适应宽度的下限（内容决定宽度，只有很窄时才用它兜底）
    float popup_max_width_ratio = 0.74f; // 相对画布宽度
    float popup_max_height_ratio = 0.82f;
    float popup_backdrop_alpha = 0.55f;
    float popup_button_min_width = 132.0f;
};
inline ComponentStyle component_style;

// ---- 帧信息 --------------------------------------------------------------
inline float delta_time = 1.0f / 60.0f;
// 应用启动以来的秒数（流光/呼吸这类动画用它，保证与帧率无关）
inline float time = 0.0f;
inline int frame_index = 0;
inline float ui_scale = 1.0f;
inline bool compact = false;
inline const char* platform_name = "Desktop";
// 焦点框样式：false = 组件库流光框，true = pause_menu 风格的装饰框。
inline bool pause_focus_frame = false;

// ---- 鼠标 ----------------------------------------------------------------
inline ImVec2 mouse{-FLT_MAX, -FLT_MAX};
inline ImVec2 mouse_delta{0.0f, 0.0f};
inline bool mouse_down[3]{false, false, false};
inline bool mouse_pressed[3]{false, false, false};
inline bool mouse_released[3]{false, false, false};
inline bool mouse_available = false;
inline bool pointer_touch = false;
inline bool pointer_activity = false;
inline bool pointer_dragging = false;
inline ImVec2 pointer_drag_origin{-FLT_MAX, -FLT_MAX};
inline Widget* pointer_drag_host = nullptr;
inline float mouse_wheel = 0.0f; // 本帧滚轮累计刻度（EndFrame 清零）

// ---- 手柄 / 键盘（后端抽象后的动作） --------------------------------------
inline PadState pad;

// ---- 输入焦点 ------------------------------------------------------------
inline Widget* hovered = nullptr; // 鼠标下的组件
inline Widget* pressed = nullptr; // 鼠标按下时锁定的组件
inline Widget* focused = nullptr; // 手柄/键盘焦点
inline Widget* active = nullptr;  // 正在激活（拖拽/长按）的组件

// ---- 焦点作用域（弹窗 Focus Trap） ----------------------------------------
// 弹窗打开时 PopupManager 把自己那套 FocusManager 挂在这里。Global::SetFocus 会拒绝
// 作用域之外的目标，于是弹窗打开期间背景页面既拿不到手柄焦点，也不会被 hover 抢走焦点。
// nullptr = 没有作用域限制（普通页面）。
inline FocusManager* focus_manager = nullptr;
bool FocusScopeAllows(const Widget* widget);

// 方向键自动重复（长按连续移动）：由 Global::NavigateFocus 内部驱动，
// 页面/弹窗都不需要自己处理重复节奏。
inline PadRepeat nav_repeat;

// ---- 输入消费 -------------------------------------------------------------
// 一帧里同一个按键只能被消费一次：页面/控件处理后 MarkConsumed，
// 后面的接收者（包括同一帧内刚拿到焦点的控件）就不会重复响应。
inline bool consumed[kInputActionCount] = {};
bool Available(InputAction action);
void MarkConsumed(InputAction action);

// 用全局约定样式（Global::component_style）造一份「组件框」参数：
// Widget 走自己的字段，非 Widget 的（比如 Toast）直接用这个，保证框长得一模一样。
BoxVisual ComponentBoxVisual();

// 让「约定样式」跟随当前主题（边框色 / 阴影浓淡）。切完主题调一次，
// 再对页面根节点调 Widget::RefreshThemeTree() 让组件重新取色。
void ApplyTheme();

// ---- 弹窗系统（宿主注入）--------------------------------------------------
// Page 每帧把自己那套 PopupManager 挂在这里；组件（例如 ImageViewer 的"确认图片"）需要在
// 不持有 Page 的情况下弹确认框时用它 —— 仍然只有一套 PopupManager，不新建弹窗系统。
inline PopupManager* popup_manager = nullptr;

// ---- 图片加载（宿主注入）--------------------------------------------------
// 组件层不直接调平台接口：宿主（App / Page）启动时注册「按路径加载 / 释放」，
// 需要按路径显示图片的组件（ImageViewer）只消费这个钩子 —— 不新建 ImageLoader / TextureCache。
// 现成实现见 demo.cpp / examples：load 里用 Backend::LoadTexture + 自己的缓存保活。
struct ImageHandle {
    ImTextureRef texture{};
    int width = 0;
    int height = 0;
    long long file_size = 0; // 文件字节数（宿主填；0 = 未知，界面不显示）
    std::string error;       // 失败原因（直接显示在 Failed 界面上）

    bool Valid() const { return texture.GetTexID() != ImTextureID_Invalid && width > 0 && height > 0; }
};

struct ImageSource {
    std::function<ImageHandle(const char* path)> load;
    std::function<void(ImageHandle& handle)> release;
};

inline ImageSource image_source;

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
