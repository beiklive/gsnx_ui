// ============================================================================
// 课时 6 · 事件响应
// ============================================================================
//
// 【目标】
//   搞清输入是怎么从系统事件走到你的控件的，并做到"三路输入一个状态"。
//
// ---------------------------------------------------------------------------
// 一、输入是怎么进来的（链路）
// ---------------------------------------------------------------------------
//   系统事件（SDL_Event）
//     -> 后端 PollEvents：喂给 imgui_impl_sdl2，同时翻译成我们自己的抽象动作
//     -> ImGui 侧：鼠标/键盘进入 io（io.MousePos / io.MouseDown / io.KeysData...）
//     -> 你的控件：IsItemHovered / IsItemActive / IsItemClicked / IsKeyPressed ...
//     -> 我们自己的侧：gui_dev::InputAction（Up/Down/Confirm/Cancel/Menu/L/R/ZL/ZR）
//
//   为什么要多一层抽象动作：
//     · 业务代码不认 SDL 的键码/手柄按钮，换平台（Switch/桌面）不用改；
//     · 手柄的 ZL/ZR 在 SDL 里是**模拟轴**而不是按钮，映射差异被后端吃掉（阈值 0.5）；
//     · "菜单快捷键"这类语义（ZL+ZR）在动作层做边沿检测，比在按键层做干净。
//
// ---------------------------------------------------------------------------
// 二、控件怎么知道自己被点了
// ---------------------------------------------------------------------------
//   悬停：IsItemHovered()          —— 会考虑遮挡（被弹窗盖住时不算 hovered）
//   按住：IsItemActive()           —— 从按下到松开期间恒为 true，拖动逻辑用它
//   点击：IsItemClicked()          —— 本帧按下
//   拖出取消：IsItemDeactivatedAfterEdit() / IsItemDeactivated()
//   键盘：IsItemFocused() + IsKeyPressed(ImGuiKey_*)  —— 导航焦点到达该 item 时
//   手柄：本项目把它翻译成方向键/确认键（Scene::OnInput），所以控件不用关心手柄
//
//   **焦点两套模型**（重要）：
//     · ImGui nav 焦点：靠 Tab/方向键在 item 之间移动，适合"键盘/鼠标"场景；
//     · 显式索引焦点：`int focus = 0;` 自己维护，适合手柄场景（本项目暂停菜单用这套）。
//   两者可以共存，但一个页面里最好只让一套主导，否则会出现"两个高亮"。
//
// ---------------------------------------------------------------------------
// 三、输入消费：别让输入泄漏
// ---------------------------------------------------------------------------
//   io.WantCaptureMouse / io.WantCaptureKeyboard：ImGui 是否用掉了这一帧的输入。
//   在模拟器里，菜单打开时**不能**再把按键喂给游戏核心，否则会一边操作菜单一边操作游戏。
//   本项目做法：菜单可见时（Host::Visible）游戏侧不再读输入。
//
// ---------------------------------------------------------------------------
// 四、热键要做边沿检测
// ---------------------------------------------------------------------------
//   错误写法：`if (pad.Held(ZL) && pad.Held(ZR)) 打开菜单;`  -> 按住不放会连开连关
//   正确写法：记住上一帧的"同时按住"状态，只在 false->true 那一次触发：
//       const bool both = pad.Held(ZL) && pad.Held(ZR);
//       if (both && !latch) { 触发一次 }
//       latch = both;
//   本课时的两个演示（热键计数、长按连发）都是这个套路。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 把热键的 latch 去掉（else 分支里直接触发），按住 Z+C 看计数器疯涨。
//   2. 把长按连发的"首次延迟"从 0.35s 改成 0，感受手感差异（游戏机 UI 都需要首次延迟）。
//   3. 给三条输入路径加上"最后来源"显示（已实现），再想：如果两路同时改焦点会怎样？
// ---------------------------------------------------------------------------
// 【验收】
//   1. 画出 系统事件 -> 抽象动作 -> 控件 的链路。
//   2. IsItemHovered 与 IsItemActive 的区别？拖动逻辑该用哪个？
//   3. ImGui nav 焦点与显式索引焦点各自的适用场景？
//   4. 为什么热键必须做边沿检测？
// ============================================================================

#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/Icons.h"
#include "ui/UiContext.h"

namespace gui_dev::course {
namespace {

const char* ActionName(InputAction action) {
    switch (action) {
    case InputAction::Up: return "Up";
    case InputAction::Down: return "Down";
    case InputAction::Left: return "Left";
    case InputAction::Right: return "Right";
    case InputAction::Confirm: return "Confirm(A)";
    case InputAction::Cancel: return "Cancel(B)";
    case InputAction::Menu: return "Menu(+)";
    case InputAction::PageLeft: return "L";
    case InputAction::PageRight: return "R";
    case InputAction::TriggerLeft: return "ZL";
    case InputAction::TriggerRight: return "ZR";
    default: return "-";
    }
}

} // namespace

class Lesson06EventsScene final : public Scene {
public:
    const char* Name() const override { return "lesson06"; }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 6 · 事件响应",
                                    "三路输入（鼠标/键盘/手柄）汇聚到同一个状态")) {
            Components::EndPanel(ui);
            return;
        }

        const PadState& pad = ui.Pad();

        // ---- 一、原始输入实况 ----------------------------------------------
        Section("一、原始输入实况：抽象动作层（按任意方向键/A/B/L/R/ZL/ZR 试试）");
        if (ImGui::BeginTable("##pad_table", 4, ImGuiTableFlags_SizingStretchSame)) {
            const InputAction actions[] = {
                InputAction::Up,       InputAction::Down,      InputAction::Left,
                InputAction::Right,    InputAction::Confirm,   InputAction::Cancel,
                InputAction::Menu,     InputAction::PageLeft,  InputAction::PageRight,
                InputAction::TriggerLeft, InputAction::TriggerRight, InputAction::None};
            for (int i = 0; actions[i] != InputAction::None; ++i) {
                ImGui::TableNextColumn();
                const bool held = pad.Held(actions[i]);
                const bool pressed = pad.Pressed(actions[i]);
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      held ? ImVec4(0.89f, 0.11f, 0.15f, 1.0f)
                                           : ImVec4(0.62f, 0.64f, 0.68f, 1.0f));
                ImGui::Text("%-10s %s%s", ActionName(actions[i]), held ? "按住" : "-",
                            pressed ? " 刚按下" : "");
                ImGui::PopStyleColor();
            }
            ImGui::EndTable();
        }

        // ---- 二、三条路径汇聚到一个状态 ------------------------------------
        Section("二、三路输入 -> 一个状态（这就是「控件响应」的本质）");
        Note("下面三行共用同一个 focus 变量：鼠标 hover/点击、手柄方向键、键盘导航。");
        ImGui::Text("focus = %d   最后来源：%s", focus_, source_);
        for (int i = 0; i < kRowCount; ++i) {
            ImGui::PushID(i);
            // 手柄路径：显式索引焦点（本项目推荐的手柄模型）
            if (pad.Pressed(InputAction::Down)) {
                focus_ = (focus_ + 1) % kRowCount;
                source_ = "手柄 ↓";
            }
            if (pad.Pressed(InputAction::Up)) {
                focus_ = (focus_ + kRowCount - 1) % kRowCount;
                source_ = "手柄 ↑";
            }
            // 鼠标路径：hover 吸焦点，点击激活
            const ImVec2 mn = ImGui::GetCursorScreenPos();
            const ImVec2 size(ImGui::GetContentRegionAvail().x, 34.0f);
            const bool clicked = ImGui::InvisibleButton("##row", size);
            const bool hovered = ImGui::IsItemHovered();
            if (hovered) {
                focus_ = i;
                source_ = "鼠标";
            }
            if (clicked) {
                ++activate_count_;
                source_ = "鼠标点击";
            }
            // 键盘路径：ImGui 导航焦点（Tab/方向键到达时）
            const bool nav_focused = ImGui::IsItemFocused();
            if (nav_focused && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
                ++activate_count_;
                source_ = "键盘 Enter";
            }
            ImGui::PopID();

            const bool selected = i == focus_;
            ImGui::GetWindowDrawList()->AddRectFilled(
                mn, ImVec2(mn.x + size.x, mn.y + size.y),
                selected ? IM_COL32(0xE2, 0x1B, 0x25, 0xFF) : IM_COL32(0x22, 0x26, 0x2C, 0xFF),
                selected ? 6.0f : 4.0f);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(mn.x + 14.0f, mn.y + 8.0f), IM_COL32(0xF5, 0xF5, 0xF7, 0xFF),
                selected ? "选中（红）" : "未选中");
        }
        ImGui::TextDisabled("激活次数：%d", activate_count_);

        // ---- 三、热键：边沿检测 --------------------------------------------
        Section("三、热键 ZL+ZR：边沿检测（按住不放只触发一次）");
        const bool both = pad.Held(InputAction::TriggerLeft) && pad.Held(InputAction::TriggerRight);
        if (both && !hotkey_latch_) {
            ++hotkey_count_;
        }
        hotkey_latch_ = both;
        ImGui::Text("ZL+ZR 触发次数：%d   当前同时按住：%s", hotkey_count_, both ? "是" : "否");
        Note("键盘上 ZL/ZR 对应 Z / C（后端把 SDL 的扳机轴按阈值采样成按钮）。");

        // ---- 四、长按连发 --------------------------------------------------
        Section("四、长按连发：首次延迟 + 重复间隔");
        if (pad.Held(InputAction::PageRight)) {
            hold_time_ += ui.DeltaTime();
            if (hold_time_ >= 0.35f + static_cast<float>(repeat_count_) * 0.08f) {
                ++repeat_count_;
            }
        } else {
            hold_time_ = 0.0f;
            repeat_count_ = 0;
        }
        ImGui::Text("按住 R 已 %.2fs，触发 %d 次", static_cast<double>(hold_time_), repeat_count_);
        Note("做法：前 0.35s 不重复（防误触），之后按间隔重复 —— 游戏机 UI 的标准手感。");

        // ---- 五、输入消费 --------------------------------------------------
        Section("五、输入消费：别让输入泄漏给游戏");
        const ImGuiIO& io = ImGui::GetIO();
        KeyValue("io.WantCaptureMouse", "%s", io.WantCaptureMouse ? "是（UI 吃掉了鼠标）" : "否");
        KeyValue("io.WantCaptureKeyboard", "%s", io.WantCaptureKeyboard ? "是" : "否");
        Note("模拟器里菜单可见时，游戏核心必须停止读输入，否则会「边操作菜单边操作游戏」。");
        Note("本项目在 Host 层做这件事：菜单可见 -> 游戏侧不再分发输入。");

        Section("六、练习与验收");
        Bullet("练习：去掉热键 latch，按住 Z+C 看计数疯涨");
        Bullet("验收：IsItemHovered 与 IsItemActive 区别？热键为什么要边沿检测？");

        EndLesson(ui);
    }

private:
    static constexpr int kRowCount = 4;
    int focus_ = 0;
    const char* source_ = "无";
    int activate_count_ = 0;
    int hotkey_count_ = 0;
    bool hotkey_latch_ = false;
    float hold_time_ = 0.0f;
    int repeat_count_ = 0;
};

std::unique_ptr<Scene> CreateLesson06Events() { return std::make_unique<Lesson06EventsScene>(); }

} // namespace gui_dev::course
