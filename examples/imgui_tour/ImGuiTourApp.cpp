#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <cstdarg>
#include <cstdio>

#include "component_view/Theme.h"
#include "core/App.h"
#include "examples/imgui_tour/ImGuiTourApp.h"
#include "examples/imgui_tour/TourTabs.h"
#include "ui/Icons.h"
#include "ui/Scene.h"
#include "ui/UiContext.h"

namespace gui_dev::tour {
namespace {

// 只用来占住 Scene 栈顶（页面直接在 App::OnFrame 里画）
class HostScene : public gui_dev::Scene {
public:
    const char* Name() const override { return "imgui_tour"; }
    void OnRender(gui_dev::UiContext& ui) override { (void)ui; }
};

constexpr ImGuiWindowFlags kShellFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

// 抓帧/CI 用：GUI_DEV_TOUR_TAB=<n> 时，前几帧强制选中第 n 个 Tab
int ForcedTabIndex() {
    static const int index = [] {
        const char* value = std::getenv("GUI_DEV_TOUR_TAB");
        return value != nullptr ? std::atoi(value) : -1;
    }();
    return index;
}

constexpr int kTabCount = 8;

// force: 抓帧用（GUI_DEV_TOUR_TAB）；want: 手柄/键盘切页时本帧要选中的 Tab
void DrawOneTab(const char* name, void (*draw)(State&), State& state, int index, bool force, int want) {
    ImGuiTabItemFlags flags = 0;
    if ((force && ForcedTabIndex() == index) || want == index) {
        flags |= ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(name, nullptr, flags)) {
        state.active_tab = index;
        draw(state);
        ImGui::EndTabItem();
    }
}

} // namespace

// ---------------------------------------------------------------- 日志 ----

void State::AppendLog(const char* fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (log.empty()) {
        log.appendf("[%04d] %s\n", log_sequence++, buffer);
        return;
    }
    // 给相关行加编号，方便看滚动与过滤
    log.appendf("[%04d] %s\n", log_sequence++, buffer);
    if (log.size() > 16 * 1024) {
        // 控制日志体积：超过 16KB 丢掉前一半
        const char* begin = log.begin();
        const char* keep = begin + log.size() / 2;
        while (keep < log.end() && *keep != '\n') {
            ++keep;
        }
        std::string tail(keep + 1, log.end());
        log.clear();
        log.append(tail.c_str());
    }
}

// ------------------------------------------------------------ 外壳与生命周期 ----

void TourApp::Configure(BackendConfig& cfg, PlatformKind kind) const {
    (void)kind;
    cfg.title = "GUI_DEV · ImGui 能力导览";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.vsync = true;
    cfg.resizable = true;
    if (const char* size = std::getenv("GUI_DEV_WINDOW")) {
        int w = 0;
        int h = 0;
        if (std::sscanf(size, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
            cfg.width = w;
            cfg.height = h;
        }
    }
    if (std::getenv("GUI_DEV_NO_VSYNC") != nullptr) {
        cfg.vsync = false;
    }
#if defined(GUI_DEV_PLATFORM_switch)
    cfg.vsync = false;
#endif
}

void TourApp::ApplyStyleTweaks() {
    ImGuiStyle& style = ImGui::GetStyle();
    // 720p 手持尺度：直接改字号基准（后端每帧会把 FontScaleMain 重置为 1.0，
    // 所以这里用 FontSizeBase：它是「全局缩放前」的基准字号，控件高度也跟着它走）
    style.FontSizeBase = 17.0f;
    style.WindowPadding = ImVec2(8.0f, 6.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameRounding = 3.0f;
    style.ChildRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding = 4.0f;
    state_.style_rounding = style.FrameRounding;
    state_.style_border = style.FrameBorderSize;
    state_.style_spacing = style.ItemSpacing.x;
    state_.global_scale = style.FontScaleMain;
    state_.dynamic_font_size = style.FontSizeBase;
}

void TourApp::OnStart(UiContext& ui) {
    gui_dev::cv::Theme::ApplyToImGui(); // 与项目其它 demo 一致的 VSCode 深色底
    ApplyStyleTweaks();

    // 开启 ImGui 内置导航：键盘 + 手柄（手柄状态在 FeedGamepadToImGui 里喂进来）
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.ConfigNavMoveSetMousePos = false;
    io.ConfigNavCursorVisibleAlways = true;

    // 导览用的纹理（图片绘制演示）
    texture_ = TextureRef(ui.GetBackend(), "img/border_gradient.png");
    state_.texture_ref = texture_.ImGuiRef();
    state_.texture_valid = texture_.Valid();
    // 多选演示用的 ID 适配器
    state_.selection.AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage*, int idx) {
        return static_cast<ImGuiID>(idx + 1);
    };
    state_.AppendLog("ImGui %s 能力导览启动", IMGUI_VERSION);
    state_.AppendLog("窗口：%dx%d，字体缩放 %.2f", 1280, 720, state_.global_scale);
    state_.AppendLog("导航：键盘 + 手柄（NavEnableKeyboard / NavEnableGamepad）");

    Scenes().Reset(std::make_unique<HostScene>());
}

void TourApp::OnShutdown(UiContext& ui) {
    (void)ui;
    texture_.Reset();
}

// 手柄 → ImGui 内置导航的按键事件（ImGuiKey 里本来就有 Gamepad* 键）
void TourApp::FeedGamepadToImGui(UiContext& ui) {
    const PadState& pad = ui.Pad();
    ImGuiIO& io = ImGui::GetIO();
    auto feed = [&](InputAction action, ImGuiKey key) {
        const std::size_t index = static_cast<std::size_t>(action);
        io.AddKeyEvent(key, pad.held[index]);
    };
    feed(InputAction::Up, ImGuiKey_GamepadDpadUp);
    feed(InputAction::Down, ImGuiKey_GamepadDpadDown);
    feed(InputAction::Left, ImGuiKey_GamepadDpadLeft);
    feed(InputAction::Right, ImGuiKey_GamepadDpadRight);
    feed(InputAction::Confirm, ImGuiKey_GamepadFaceDown);  // A
    feed(InputAction::Cancel, ImGuiKey_GamepadFaceRight);  // B
    feed(InputAction::ActionX, ImGuiKey_GamepadFaceLeft);  // X
    feed(InputAction::ActionY, ImGuiKey_GamepadFaceUp);    // Y
    feed(InputAction::PageLeft, ImGuiKey_GamepadL1);
    feed(InputAction::PageRight, ImGuiKey_GamepadR1);
    feed(InputAction::TriggerLeft, ImGuiKey_GamepadL2);
    feed(InputAction::TriggerRight, ImGuiKey_GamepadR2);
    feed(InputAction::Menu, ImGuiKey_GamepadStart);
    feed(InputAction::Minus, ImGuiKey_GamepadBack);
}

void TourApp::DrawShell() {
    State& s = state_;
    const ImGuiIO& io = ImGui::GetIO();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float status_height = ImGui::GetFrameHeight() + style.ItemSpacing.y;

    ImGui::Text("Dear ImGui %s", IMGUI_VERSION);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
        ImGui::TextUnformatted("这个 demo 只展示 ImGui 自带的接口：Tab 分页，页面本身不滚动。");
        ImGui::TextUnformatted("键盘：方向键 / Tab 移动，Enter 激活；手柄：同样走 ImGui 内置导航。");
        ImGui::TextUnformatted("右侧是每帧的 IO 统计；每个 Tab 都有对应接口的名字标注。");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 240.0f);
    ImGui::TextDisabled("显示 %.0fx%.0f · 逻辑 %.0fx%.0f · 基准字号 %.0fpx · 光栅密度 %.1f", io.DisplaySize.x,
                        io.DisplaySize.y, ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Size.y,
                        style.FontSizeBase, io.DisplayFramebufferScale.x);
    ImGui::Separator();

    // Tab 主体：内容按可用高度分配，避免整页滚动
    if (ImGui::BeginChild("##tour_body", ImVec2(0.0f, -status_height), ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        if (ImGui::BeginTabBar("##tour_tabs",
                               ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_TabListPopupButton |
                                   ImGuiTabBarFlags_Reorderable)) {
            const bool force = s.frame_count < 4;
            const int want = s.tab_switch > 0 ? (s.active_tab + 1) % kTabCount
                                              : (s.tab_switch < 0 ? (s.active_tab + kTabCount - 1) % kTabCount : -1);
            DrawOneTab("总览", DrawOverviewTab, s, 0, force, want);
            DrawOneTab("输入", DrawInputsTab, s, 1, force, want);
            DrawOneTab("布局", DrawLayoutTab, s, 2, force, want);
            DrawOneTab("弹层", DrawPopupsTab, s, 3, force, want);
            DrawOneTab("高级", DrawAdvancedTab, s, 4, force, want);
            DrawOneTab("绘图", DrawDrawingTab, s, 5, force, want);
            DrawOneTab("字体样式", DrawFontStyleTab, s, 6, force, want);
            DrawOneTab("系统工具", DrawSystemTab, s, 7, force, want);
            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Text("FPS %.1f (%.2f ms) · DrawCall %d · Vtx %d · Idx %d · Window %d · Nav %s%s", io.Framerate,
                s.frame_ms_avg, s.draw_calls, s.vertices, s.indices, s.windows,
                io.NavActive ? "激活" : "空闲", io.NavVisible ? "·可见" : "");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 470.0f);
    ImGui::TextDisabled("←↑↓→ 移动 · A 激活 · B 取消 · ZL/ZR 或 Q/E 换 Tab · 按住 X + L1/R1 切窗口");
}

void TourApp::OnFrame(UiContext& ui, float dt) {
    State& s = state_;
    ++s.frame_count;
    s.phase += dt;
    s.last_frame_ms = dt * 1000.0f;
    frame_ms_accum_ += s.last_frame_ms;
    if (++frame_ms_count_ >= 30) {
        s.frame_ms_avg = frame_ms_accum_ / static_cast<float>(frame_ms_count_);
        frame_ms_accum_ = 0.0f;
        frame_ms_count_ = 0;
    }

    FeedGamepadToImGui(ui);

    // Tab 切换：手柄 ZL/ZR（TriggerLeft/Right）或键盘 Q/E（PageLeft/Right）
    {
        const PadState& pad = ui.Pad();
        state_.tab_switch = 0;
        if (pad.Pressed(InputAction::TriggerRight) || pad.Pressed(InputAction::PageRight)) {
            state_.tab_switch = 1;
        } else if (pad.Pressed(InputAction::TriggerLeft) || pad.Pressed(InputAction::PageLeft)) {
            state_.tab_switch = -1;
        }
    }

    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
    if (ImGui::Begin("##imgui_tour", nullptr, kShellFlags)) {
        DrawShell();
    }
    ImGui::End();
    ImGui::PopStyleVar();

    // 调试 / 工具窗口（ImGui 自带，按需打开）
    if (s.show_demo) {
        ImGui::ShowDemoWindow(&s.show_demo);
    }
    if (s.show_metrics) {
        ImGui::ShowMetricsWindow(&s.show_metrics);
    }
    if (s.show_style_editor) {
        ImGui::Begin("样式编辑器 (ShowStyleEditor)", &s.show_style_editor);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }
    if (s.show_id_stack) {
        ImGui::ShowIDStackToolWindow(&s.show_id_stack);
    }
    if (s.show_debug_log) {
        ImGui::ShowDebugLogWindow(&s.show_debug_log);
    }
    if (s.show_about) {
        ImGui::ShowAboutWindow(&s.show_about);
    }

    // 统计：UiContext 在 Render() 之后缓存了上一帧的 ImDrawData 统计
    s.draw_calls = ui.LastDrawCalls();
    s.vertices = ui.LastVertices();
    s.indices = ui.LastIndices();
    s.windows = io.MetricsRenderWindows;

    if (const char* value = std::getenv("GUI_DEV_EXIT_AFTER")) {
        if (++frame_ >= std::atoi(value)) {
            ui.GetBackend().RequestQuit();
        }
    }
}

} // namespace gui_dev::tour
