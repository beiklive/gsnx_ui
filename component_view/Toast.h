// Toast：通用通知（右上角滑入 → 停留 3s → 滑出，多 Toast 队列 + 自动补位）。
//
// 视觉上就是「一个从右边滑进来的 Button Box」：底色 / 圆角 / 边框 / 阴影 / 字体 / 内边距
// 全部走 Global::component_style（和 Button 同一个来源，共用 Draw::ComponentBox 这套画法），
// Toast 只额外画左侧状态色条 + Material 图标 + 文本 —— 没有第二套 Box 样式。
//
// 业务代码只调 ShowSuccess / ShowError / ShowInfo，不关心坐标、动画、生命周期、排列。
//
// 每帧流程（Page 负责驱动）：
//   Update(dt)：生命周期（Entering → Visible → Exiting → 删除）+ X 轴滑入/滑出 + 每帧重算 targetY 并平滑跟随
//   Draw(dl)  ：算最终矩形 → 复用组件框画法 → 色条 → 图标 → 文本
//
// 线程：Toast 是 UI 层服务，所有 Show* / Update / Draw 都必须在 UI 线程调用（本项目 UI 单线程；
// 以后真有后台线程要发通知，再在 Show* 前面挂一个线程安全队列，UI 线程里排空即可）。
#pragma once

#include <string>
#include <vector>

#include <imgui.h>

#include "component_view/Theme.h"
#include "component_view/Types.h"

namespace gui_dev::cv {

enum class ToastType {
    Success,
    Error,
    Info,
};

// 一个 Toast 自己的状态机：Entering → Visible → Exiting →（被移除）
enum class ToastState {
    Entering,
    Visible,
    Exiting,
};

// 尺寸与时长（720p 手持基准；全局一份，改这里就能整体调）
struct ToastStyle {
    float min_width = 200.0f;
    float max_width = 320.0f;
    float min_height = Theme::kControlHeight; // 和按钮/行高一致（56）
    float bar_width = 4.0f;     // 左侧状态色条宽度
    float bar_inset = 2.0f;     // 色条离边框的留白（左/上/下都是它）
    float icon_size = 22.0f;    // Material 图标字号
    float gap = 10.0f;          // 色条 ↔ 图标 ↔ 文本 的间距
    float padding_x = 16.0f;    // 色条之外的内容左右留白
    float padding_y = 12.0f;    // 内容上下留白
    float text_size = Theme::kFontBody; // 文本字号统一 16
    float enter_duration = 0.25f;
    float exit_duration = 0.25f;
    float visible_duration = 3.0f; // 入场完成之后才开始算
    float reflow_speed = 14.0f;    // Y 轴补位的指数趋近速度
    float right_margin = 5.0f;     // 离屏幕右边
    float top_margin = 20.0f;      // 离屏幕顶部
    float spacing = 10.0f;         // 多个 Toast 之间的垂直间距
    float dedup_window = 0.0f;     // 0 = 不去重（每次 Show 都建一条）；>0 时同类型同文案在这个窗口内只刷新不新建
};

// 单个 Toast 的数据（位置只存「当前值」和「目标值」，X/Y 两套动画互不影响）
struct Toast {
    ToastType type = ToastType::Info;
    std::string message;

    ToastState state = ToastState::Entering;
    float state_time = 0.0f; // 当前状态已经过了多久（Visible 的 3s 从这里算）
    float elapsed = 0.0f;    // 从创建到现在（去重用）

    float width = 0.0f;   // 目标尺寸（按文本量出来的）
    float height = 0.0f;

    float current_y = 0.0f; // Y：跟随 target_y
    float target_y = 0.0f;
    float slide = 0.0f;     // X：0 = 完全在屏幕右边外，1 = 停在目标位置

    bool placed = false; // 第一帧先对齐 target_y，别从 0 飞下来
};

// 状态色：只给左侧色条和图标用，不动 Button 的 Box 配色
ImU32 ToastAccentColor(ToastType type);

class ToastManager {
public:
    // ---- 业务代码唯一需要知道的接口（UI 线程） -----------------------------
    void Show(ToastType type, std::string message);
    void ShowSuccess(std::string message) { Show(ToastType::Success, std::move(message)); }
    void ShowError(std::string message) { Show(ToastType::Error, std::move(message)); }
    void ShowInfo(std::string message) { Show(ToastType::Info, std::move(message)); }

    // ---- 宿主每帧调（Page 已经接好） ---------------------------------------
    void Update(float dt);
    void Draw(ImDrawList* dl);

    void Clear() { toasts_.clear(); }
    bool Empty() const { return toasts_.empty(); }
    int Count() const { return static_cast<int>(toasts_.size()); }
    ToastStyle& Style() { return style_; }
    const ToastStyle& Style() const { return style_; }

private:
    void Measure(Toast& toast) const; // 按文本算宽高（含换行）

    std::vector<Toast> toasts_;
    ToastStyle style_;
};

} // namespace gui_dev::cv
