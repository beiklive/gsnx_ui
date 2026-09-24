// ImGui 能力导览 Demo：把 Dear ImGui 自身的能力分 Tab 展示（页面不滚动）。
//
// 说明：这个 demo 展示的是 **ImGui 原生控件与接口**，不是 component_view 的自绘控件；
// 只有外壳（窗口/Tab/状态栏）用了几行原生 API，其余全是 ImGui 自己的东西。
#pragma once

#include <string>
#include <vector>

#include "core/App.h"
#include "ui/Texture.h"

#include <imgui.h>

namespace gui_dev::tour {

// 各 Tab 共享状态（切 Tab 不丢）
struct State {
    // ---- 基础控件 ----
    bool check_a = true;
    bool check_b = false;
    bool check_c = false;
    int radio = 0;
    int selectable = 0;
    float progress = 0.35f;
    bool disabled_block = false;
    bool tooltip_enabled = true;

    // ---- 输入 ----
    char text[128] = "可编辑文本";
    std::string std_text = "std::string 也能直接绑";
    std::string multiline = "多行输入第一行\n第二行";
    std::string password = "hunter2";
    int int_value = 42;
    float float_value = 3.14f;
    int drag_int = 128;
    float drag_float = 12.5f;
    float drag_speed = 0.5f;
    float drag_min = 0.0f;
    float drag_max = 100.0f;
    float vec3[3] = {0.25f, 0.60f, 0.90f};
    int slider_int = 50;
    float slider_float = 0.50f;
    float slider_angle = 1.0f;
    float color[4] = {0.00f, 0.48f, 0.80f, 1.00f};
    int combo = 0;
    int listbox = 1;

    // ---- 布局 ----
    float splitter = 0.45f;
    float nested_value = 10.0f;
    bool tree_selected = false;
    int table_row_selected = 1;

    // ---- 弹层 ----
    bool open_modal = false;
    int modal_choice = -1;
    bool open_popup = false;
    int menu_choice = -1;
    char modal_input[64] = "槽位 1";

    // ---- 拖放 / 虚拟滚动 / 多选 ----
    std::vector<std::string> dropped;
    std::string drag_tip = "(把左边的方块拖到右边)";
    int clipper_items = 100000;
    int clipper_first = 0;
    int clipper_last = 0;
    int clipper_drawn = 0;
    ImGuiSelectionBasicStorage selection;
    int selection_touched = -1;

    // ---- 绘图 ----
    float phase = 0.0f;
    bool draw_fill = true;
    bool draw_clip = true;

    // ---- 字体与样式 ----
    int font_index = 0;
    bool dynamic_size = false;
    float dynamic_font_size = 26.0f;
    float global_scale = 1.0f;
    float style_rounding = 3.0f;
    float style_border = 1.0f;
    float style_spacing = 6.0f;

    // ---- 日志 / 状态 / 工具 ----
    ImGuiTextBuffer log;
    ImGuiTextFilter filter;
    bool log_autoscroll = true;
    int log_sequence = 0;
    int storage_counter = 0;
    float texture_uv[2] = {0.0f, 1.0f};
    bool show_demo = false;
    bool show_metrics = false;
    bool show_style_editor = false;
    bool show_id_stack = false;
    bool show_debug_log = false;
    bool show_about = false;
    bool nav_keyboard = true;
    bool nav_gamepad = true;
    bool no_mouse_cursor_change = false;
    bool swap_gamepad_buttons = false;

    // ---- 纹理（图片绘制演示用，由 App 注入） ----
    ImTextureRef texture_ref;
    bool texture_valid = false;

    // ---- Tab 切换（由手柄 ZL/ZR 或键盘 Q/E 触发，用 ImGuiTabItemFlags_SetSelected） ----
    int active_tab = 0;
    int tab_switch = 0; // +1 = 下一页，-1 = 上一页

    // ---- 每帧统计 ----
    int frame_count = 0;
    float last_frame_ms = 0.0f;
    float frame_ms_avg = 0.0f;
    int draw_calls = 0;
    int vertices = 0;
    int indices = 0;
    int windows = 0;
    int callback_hits = 0;

    void AppendLog(const char* fmt, ...) IM_FMTARGS(2);
};

class TourApp : public gui_dev::App {
public:
    void Configure(BackendConfig& cfg, PlatformKind kind) const override;
    void OnStart(UiContext& ui) override;
    void OnFrame(UiContext& ui, float dt) override;
    void OnShutdown(UiContext& ui) override;

private:
    void ApplyStyleTweaks();
    // 把手柄状态喂给 ImGui 内置导航（NavEnableGamepad）
    void FeedGamepadToImGui(UiContext& ui);
    void DrawShell();

    State state_;
    TextureRef texture_;
    int frame_ = 0;
    float frame_ms_accum_ = 0.0f;
    int frame_ms_count_ = 0;
};

} // namespace gui_dev::tour
