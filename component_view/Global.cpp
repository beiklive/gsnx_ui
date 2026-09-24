#include "component_view/Global.h"

#include <cfloat>

#include "component_view/Widget.h"
#include "ui/UiContext.h"

namespace gui_dev::cv::Global {
namespace {

bool prev_mouse_down[3] = {false, false, false};

} // namespace

void BeginFrame(UiContext& ui) {
    ImGuiIO& io = ImGui::GetIO();

    // 后端已把 drawable 换算成 720p 设计空间：这里拿到的就是逻辑坐标。
    canvas_pos = ImVec2(0.0f, 0.0f);
    canvas_size = io.DisplaySize.x > 0.0f ? io.DisplaySize : canvas_size;
    ui_scale = io.DisplayFramebufferScale.x > 0.0f ? io.DisplayFramebufferScale.x : 1.0f;
    draw_list = ImGui::GetForegroundDrawList();

    delta_time = ui.DeltaTime();
    pad = ui.Pad();
    compact = canvas_size.x < 900.0f;
    platform_name = PlatformName();
    ++frame_index;

    const bool finite = io.MousePos.x > -FLT_MAX * 0.5f && io.MousePos.y > -FLT_MAX * 0.5f;
    mouse_available = finite;
    mouse = finite ? io.MousePos : ImVec2(-FLT_MAX, -FLT_MAX);

    for (int button = 0; button < 3; ++button) {
        const bool down = io.MouseDown[button];
        mouse_pressed[button] = down && !prev_mouse_down[button];
        mouse_released[button] = !down && prev_mouse_down[button];
        mouse_down[button] = down;
        prev_mouse_down[button] = down;
    }

    hovered = nullptr;
    for (std::size_t i = 0; i < kInputActionCount; ++i) {
        consumed[i] = false;
    }
}

bool Available(InputAction action) {
    const std::size_t index = static_cast<std::size_t>(action);
    return index < kInputActionCount && !consumed[index];
}

void MarkConsumed(InputAction action) {
    const std::size_t index = static_cast<std::size_t>(action);
    if (index < kInputActionCount) {
        consumed[index] = true;
    }
}

void EndFrame() {
    for (int button = 0; button < 3; ++button) {
        mouse_pressed[button] = false;
        mouse_released[button] = false;
    }
}

void SetFocus(Widget* widget) {
    if (focused == widget) {
        return;
    }
    // 只改状态：focusIn/focusOut 由 Widget::UpdateInteraction 检测跳变后发射，
    // 保证「手动 SetFocus」和「导航自动换焦点」走同一条信号路径。
    Widget* previous = focused;
    focused = widget;
    if (previous != nullptr) {
        previous->focused = false;
    }
    if (focused != nullptr) {
        focused->focused = true;
    }
}

void NavigateFocus(const std::vector<Widget*>& focusables) {
    if (focusables.empty()) {
        SetFocus(nullptr);
        return;
    }

    Widget* current = focused;
    bool valid = false;
    for (Widget* item : focusables) {
        if (item == current) {
            valid = true;
            break;
        }
    }
    if (!valid) {
        SetFocus(focusables.front());
        current = focused;
    }
    if (current == nullptr) {
        return;
    }

    ImVec2 dir(0.0f, 0.0f);
    if (pad.Pressed(InputAction::Left)) {
        dir.x = -1.0f;
    } else if (pad.Pressed(InputAction::Right)) {
        dir.x = 1.0f;
    } else if (pad.Pressed(InputAction::Up)) {
        dir.y = -1.0f;
    } else if (pad.Pressed(InputAction::Down)) {
        dir.y = 1.0f;
    }
    if (dir.x == 0.0f && dir.y == 0.0f) {
        return;
    }
    // 控件自己消费这个方向时，导航让位（例如垂直列表里的 ↑↓、滑条里的 ←→）
    if ((dir.x != 0.0f && current->capture_horizontal) || (dir.y != 0.0f && current->capture_vertical)) {
        return;
    }

    // 最近邻选择：主方向投影距离 + 2 倍垂直偏移。
    // 先在同一个焦点分区里找；找不到且是左右方向时，才允许跨分区
    //（这正是「内容区按 ← 回到左侧标签列、标签列按 → 进入内容」的实现）。
    const ImVec2 center = current->rect.Center();
    const int zone = current->focus_zone;

    auto pick = [&](bool same_zone_only) {
        Widget* best = nullptr;
        float best_score = FLT_MAX;
        for (Widget* item : focusables) {
            if (item == current || !item->visible || !item->enabled) {
                continue;
            }
            const bool same_zone = (item->focus_zone == zone);
            if (same_zone_only != same_zone) {
                continue;
            }
            const ImVec2 item_center = item->rect.Center();
            const ImVec2 delta(item_center.x - center.x, item_center.y - center.y);
            const float along = delta.x * dir.x + delta.y * dir.y;
            if (along <= 1.0f) {
                continue;
            }
            const float perpendicular = Absf(dir.x != 0.0f ? delta.y : delta.x);
            const float score = along + perpendicular * 2.0f;
            if (score < best_score) {
                best_score = score;
                best = item;
            }
        }
        return best;
    };

    Widget* best = pick(true);
    if (best == nullptr && dir.x != 0.0f) {
        best = pick(false);
    }
    if (best != nullptr) {
        SetFocus(best);
    }
}

} // namespace gui_dev::cv::Global
