#include "demo/widgets/WidgetDemo.h"

#include <cmath>
#include <cstdio>

#include <imgui.h>
#include <imgui_internal.h> // 只有 Lesson 7 的高级用法需要（ButtonBehavior）

#include "ui/Components.h"
#include "ui/Theme.h"
#include "ui/UiContext.h"

namespace gui_dev::demo {
namespace {

constexpr float kPi = 3.14159265358979323846f;

// ---- 小工具 ----------------------------------------------------------------

ImU32 Mix(ImU32 a, ImU32 b, float t) {
    const auto ch = [t](ImU32 x, ImU32 y) {
        const float p = static_cast<float>((x >> IM_COL32_R_SHIFT) & 0xFF);
        const float q = static_cast<float>((y >> IM_COL32_R_SHIFT) & 0xFF);
        return static_cast<ImU32>(p + (q - p) * t);
    };
    return IM_COL32(ch(a, b), ch(a << 8, b << 8), ch(a << 16, b << 16),
                    (a >> IM_COL32_A_SHIFT) & 0xFF);
}

void LessonText(const char* api, const char* body) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.68f, 0.85f, 1.0f));
    ImGui::TextUnformatted(api);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.64f, 0.68f, 1.0f));
    ImGui::TextWrapped("%s", body);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

// ============================================================ Lesson 1 =======
// 最小自定义控件：一个带边框的彩色标签。
//
// 用了什么：
//   ImGui::CalcTextSize(label)          先算内容尺寸，才能定控件尺寸
//   ImGui::GetCursorScreenPos()         当前布局游标所在的屏幕坐标（左上角）
//   ImGui::InvisibleButton(id, size)    占位 + 命中测试；返回"本帧被点击"
//   ImGui::IsItemHovered()              刚才提交的那个 item 是否悬浮
//   ImGui::GetWindowDrawList()          当前窗口的绘制列表
//   ImDrawList::AddRectFilled/AddRect/AddText
//
// 为什么这样写：InvisibleButton 把布局游标推进 size，同时登记了一个可交互 item，
// 于是 ImGui 帮你处理了裁剪、ID 冲突检测、鼠标遮挡判断（被弹窗挡住时不会误触发）。
// 你只管在 [mn, mx] 这个矩形里画。
bool SimpleTag(const char* id, const char* label, ImU32 color) {
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const ImVec2 size(text_size.x + 28.0f, text_size.y + 14.0f);

    const ImVec2 mn = ImGui::GetCursorScreenPos(); // 必须在 InvisibleButton 之前取
    const bool clicked = ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 mx(mn.x + size.x, mn.y + size.y);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(mn, mx, hovered ? Mix(color, IM_COL32_WHITE, 0.25f) : color, 5.0f);
    dl->AddRect(mn, mx, IM_COL32(255, 255, 255, 200), 5.0f, 0, 1.5f);
    dl->AddText(ImVec2(mn.x + 14.0f, mn.y + 7.0f), IM_COL32_WHITE, label);
    return clicked;
}

// ============================================================ Lesson 2 =======
// 加交互状态 + 帧率无关动画：一颗会呼吸、按下会缩的圆点。
//
// 用了什么：
//   ImGui::IsItemHovered()   悬浮
//   ImGui::IsItemActive()    按住期间恒为 true（ImGui 帮你管理"谁在被按"）
//   ImGui::IsItemClicked()   本帧按下
//   ImGui::GetIO().DeltaTime 帧间隔 —— 控件内部动画就用它，不用外部传
//   状态存哪：这里是函数内 static；多实例场景请改用成员或 ImGuiStorage（见 Lesson 3）
//
// 坑：动画绝不能写成 current += 0.1f（帧率一变速度就变）。要用指数趋近或定时长推进。
bool PulseDot(const char* id, float radius) {
    static bool held = false;
    static float press = 0.0f; // 0..1 按压量

    const ImVec2 mn = ImGui::GetCursorScreenPos();
    const ImVec2 size(radius * 2.0f + 12.0f, radius * 2.0f + 12.0f);
    const ImVec2 center(mn.x + size.x * 0.5f, mn.y + size.y * 0.5f);

    const bool clicked = ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    if (active != held) {
        held = active;
    }

    // 帧率无关：1 - exp(-speed * dt)
    const float dt = ImGui::GetIO().DeltaTime;
    const float target = held ? 1.0f : 0.0f;
    press += (target - press) * (1.0f - std::exp(-18.0f * dt));

    const float pulse = 0.5f + 0.5f * std::sin(ImGui::GetTime() * 3.0f); // 呼吸
    const float r = radius * (1.0f - 0.18f * press) * (1.0f + 0.06f * pulse);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(center, r, held ? IM_COL32(0xE2, 0x1B, 0x25, 0xFF) : IM_COL32(0x4F, 0xA3, 0xFF, 0xFF));
    dl->AddCircle(center, r + 3.0f, IM_COL32(255, 255, 255, hovered ? 170 : 60), 0, 2.0f);
    // 按住时的外扩波纹
    if (press > 0.02f) {
        dl->AddCircle(center, r + 6.0f + 16.0f * press, IM_COL32(0xFF, 255, 255, static_cast<int>(90 * (1.0f - press))), 0,
                      3.0f);
    }
    return clicked;
}

// ============================================================ Lesson 3 =======
// 每实例独立状态 + 拖动：自定义 Toggle 开关。
//
// 用了什么：
//   ImGui::GetStateStorage()  按 ID 存浮点/整数的官方"控件本地状态"容器（不用全局变量）
//   ImGuiStorage::GetFloatRef(key, default)  返回引用，可直接 +=
//   ImGui::IsMouseDragging(0)  判断拖动中
//
// 坑：控件在循环里出现多次时，必须保证 ID 唯一 —— 见 Lesson 6 的 PushID。
bool ToggleSwitch(const char* id, bool* value, float* anim) {
    const float w = 84.0f;
    const float h = 34.0f;
    const ImVec2 mn = ImGui::GetCursorScreenPos();
    const ImVec2 size(w, h);

    ImGui::InvisibleButton(id, size);
    const bool active = ImGui::IsItemActive();

    // 点击切换 + 按住拖动也能改（把 x 位置映射成 0/1）
    if (ImGui::IsItemClicked()) {
        *value = !*value;
    }
    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const float t = (ImGui::GetMousePos().x - mn.x) / w;
        *value = t > 0.5f;
    }

    // 用 ID 关联的本地状态做动画（等价于"控件自己的成员变量"）
    const float dt = ImGui::GetIO().DeltaTime;
    *anim += ((*value ? 1.0f : 0.0f) - *anim) * (1.0f - std::exp(-16.0f * dt));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 mx(mn.x + w, mn.y + h);
    const float rounding = h * 0.5f;
    dl->AddRectFilled(mn, mx, IM_COL32(0x22, 0x26, 0x2C, 0xFF), rounding);

    // 打开时从左侧"填充"过来的红色区域
    if (*anim > 0.01f) {
        const ImVec2 fill_mx(mn.x + w * *anim, mx.y);
        dl->AddRectFilled(mn, fill_mx, IM_COL32(0xE2, 0x1B, 0x25, 0xFF), rounding);
    }
    dl->AddRect(mn, mx, IM_COL32(255, 255, 255, active ? 230 : 110), rounding, 0, 2.0f);

    // 圆形滑块
    const float knob_x = mn.x + rounding + (w - h) * *anim;
    const ImVec2 knob(knob_x, mn.y + rounding);
    dl->AddCircleFilled(knob, rounding - 4.0f, IM_COL32(0xF5, 0xF5, 0xF7, 0xFF));
    dl->AddCircle(knob, rounding - 4.0f, IM_COL32(0x00, 0x00, 0x00, 0x60), 0, 1.0f);

    // 文字（跟着滑块走，避免和滑块重叠）
    const char* text = *value ? "ON" : "OFF";
    const ImVec2 text_size = ImGui::CalcTextSize(text);
    const float text_x = *value ? mn.x + 12.0f : mx.x - text_size.x - 12.0f;
    dl->AddText(ImVec2(text_x, mn.y + (h - text_size.y) * 0.5f), IM_COL32_WHITE, text);
    return active;
}

// ============================================================ Lesson 4 =======
// 拖动型控件：自定义滑块。
//
// 用了什么：
//   ImGui::IsItemActive()      判断"这个 item 正被按住" —— 拖动逻辑的入口
//   ImGui::GetMousePos()       鼠标位置，映射到轨道比例
//   ImGui::IsItemFocused()     键盘/手柄导航聚焦到这个 item（本项目导航由 Scene::OnInput 翻译）
//   ImGui::IsKeyPressed(...)   聚焦时用方向键微调
//
// 要点：
//   · 占位矩形要覆盖"可点击区域"（这里是整条轨道 + 若干余量），否则边缘点不到；
//   · 拖动中不要每帧重新读取点击，用 IsItemActive() 保持"拖拽所有权"；
//   · 键盘路径别忘了（本项目要求不依赖鼠标）。
bool CustomSlider(const char* id, float* value, float min_value, float max_value, const char* fmt) {
    const float w = 300.0f;   // 轨道宽度
    const float h = 30.0f;
    const float text_reserve = 76.0f; // 给右侧数值文字留位，否则会压到后面的内容
    const ImVec2 mn = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(id, ImVec2(w + text_reserve, h));
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    const bool focused = ImGui::IsItemFocused();

    const float track_x0 = mn.x + 12.0f;
    const float track_x1 = mn.x + w - 12.0f;
    if (active) {
        const float t = (ImGui::GetMousePos().x - track_x0) / (track_x1 - track_x0);
        *value = min_value + (max_value - min_value) * (t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t));
    }
    if (focused) {
        const float step = (max_value - min_value) * 0.05f;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
            *value -= step;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
            *value += step;
        }
        *value = *value < min_value ? min_value : (*value > max_value ? max_value : *value);
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float t = (*value - min_value) / (max_value - min_value);
    const float cy = mn.y + h * 0.5f;
    const float track_h = 8.0f;

    dl->AddRectFilled(ImVec2(track_x0, cy - track_h * 0.5f), ImVec2(track_x1, cy + track_h * 0.5f),
                      IM_COL32(0x22, 0x26, 0x2C, 0xFF), 4.0f);
    const float fill_x = track_x0 + (track_x1 - track_x0) * t;
    dl->AddRectFilled(ImVec2(track_x0, cy - track_h * 0.5f), ImVec2(fill_x, cy + track_h * 0.5f),
                      IM_COL32(0xE2, 0x1B, 0x25, 0xFF), 4.0f);

    const float knob_r = (active || hovered || focused) ? 11.0f : 9.0f;
    dl->AddCircleFilled(ImVec2(fill_x, cy), knob_r, IM_COL32(0xF5, 0xF5, 0xF7, 0xFF));
    dl->AddCircle(ImVec2(fill_x, cy), knob_r, IM_COL32(0x00, 0x00, 0x00, 0x80), 0, 1.5f);
    if (focused) {
        dl->AddCircle(ImVec2(fill_x, cy), knob_r + 4.0f, IM_COL32(0x4F, 0xA3, 0xFF, 0xFF), 0, 2.0f);
    }

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), fmt, static_cast<double>(*value));
    dl->AddText(ImVec2(track_x1 + 8.0f, cy - ImGui::GetTextLineHeight() * 0.5f), IM_COL32(0xEC, 0xEE, 0xF1, 0xFF),
                buffer);
    return active;
}

// ============================================================ Lesson 5 =======
// 纯几何绘制 + 极坐标交互：旋钮。
//
// 用了什么：
//   ImDrawList::PathArcTo / PathStroke   画弧（起始角、半径、分段数）
//   ImDrawList::AddCircleFilled          圆心
//   std::atan2                           鼠标方向 -> 角度 -> 数值
//   ImDrawList::AddLine                  指针
//
// 要点：几何绘制要用「中心 + 半径 + 角度」，别用矩形思维；
//       角度到数值的映射注意 0 度位置与方向（屏幕 y 轴向下，sin 是反的）。
bool Knob(const char* id, float* value01, float radius) {
    const ImVec2 mn = ImGui::GetCursorScreenPos();
    const ImVec2 size(radius * 2.0f + 10.0f, radius * 2.0f + 10.0f);
    const ImVec2 center(mn.x + size.x * 0.5f, mn.y + size.y * 0.5f);

    ImGui::InvisibleButton(id, size);
    const bool active = ImGui::IsItemActive();

    if (active) {
        const ImVec2 delta(ImGui::GetMousePos().x - center.x, ImGui::GetMousePos().y - center.y);
        // 从正上方开始顺时针一圈：角度归一到 0..1
        float angle = std::atan2(delta.x, -delta.y); // 上=0，右=+90°
        if (angle < 0.0f) {
            angle += 2.0f * kPi;
        }
        *value01 = angle / (2.0f * kPi);
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float start = -kPi * 0.5f;                                  // 顶部
    const float end = start + 2.0f * kPi * (*value01);                // 顺时针到当前值
    constexpr int kSegments = 48;

    dl->AddCircleFilled(center, radius * 0.82f, IM_COL32(0x1A, 0x1D, 0x22, 0xFF));
    dl->PathClear();
    dl->PathArcTo(center, radius, -kPi * 0.5f, kPi * 1.5f, kSegments);
    dl->PathStroke(IM_COL32(0x33, 0x39, 0x42, 0xFF), 0, 8.0f);
    dl->PathClear();
    dl->PathArcTo(center, radius, start, end, kSegments);
    dl->PathStroke(IM_COL32(0xE2, 0x1B, 0x25, 0xFF), 0, 8.0f);

    const float pointer_angle = start + 2.0f * kPi * (*value01);
    const ImVec2 tip(center.x + std::cos(pointer_angle) * radius * 0.72f,
                     center.y + std::sin(pointer_angle) * radius * 0.72f);
    dl->AddLine(center, tip, IM_COL32(0xF5, 0xF5, 0xF7, 0xFF), 4.0f);
    dl->AddCircleFilled(center, 6.0f, IM_COL32(0xF5, 0xF5, 0xF7, 0xFF));

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(*value01 * 100.0f + 0.5f));
    const ImVec2 text_size = ImGui::CalcTextSize(buffer);
    dl->AddText(ImVec2(center.x - text_size.x * 0.5f, center.y + radius * 0.42f),
                IM_COL32(0x98, 0x98, 0xA1, 0xFF), buffer);
    return active;
}

// ============================================================ Lesson 6 =======
// 循环里的控件：ID 唯一化。
//
// 用了什么：
//   ImGui::PushID(i) / PopID()  把后续 ID 放进一个作用域，避免同名控件撞 ID
//   ImGui::SameLine()           让下一个 item 排在同一行
//
// 坑：ImGui 靠 ID 识别"同一个控件"（状态、动画、焦点都挂在 ID 上）。
//     在 for 循环里用同一个 label 而不 PushID，会出现"改一个动全部"或断言。
void LevelBar(int index, float* level) {
    ImGui::PushID(index);
    ImGui::InvisibleButton("##bar", ImVec2(260.0f, 18.0f));
    const bool active = ImGui::IsItemActive();
    if (active) {
        const ImVec2 mn = ImGui::GetItemRectMin(); // 已提交 item 的矩形，比记 origin 更省事
        const float t = (ImGui::GetMousePos().x - mn.x) / ImGui::GetItemRectSize().x;
        *level = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 mn = ImGui::GetItemRectMin();
    const ImVec2 mx = ImGui::GetItemRectMax();
    dl->AddRectFilled(mn, mx, IM_COL32(0x18, 0x1B, 0x20, 0xFF), 3.0f);
    dl->AddRectFilled(mn, ImVec2(mn.x + (mx.x - mn.x) * *level, mx.y),
                      IM_COL32(0x4F, 0xA3, 0xFF, 0xFF), 3.0f);
    dl->AddRect(mn, mx, IM_COL32(255, 255, 255, 60), 3.0f, 0, 1.0f);
    ImGui::PopID();
}

// ============================================================ Lesson 7 =======
// 容器型控件：自己开一个子区域，画标题栏 + 折叠。
//
// 用了什么：
//   ImGui::BeginChild / EndChild        独立滚动、独立布局的子窗口
//   ImGui::GetWindowPos/Size            子窗口自身几何
//   ImGui::GetWindowDrawList()
//   ImGui::InvisibleButton              自己实现标题栏按钮
//
// 注意：BeginChild/EndChild 与 Begin/End 规则不同 ——
//       无论 BeginChild 返回 true/false，都必须调用配对的 EndChild。
void CustomPanel(const char* id, const char* title, bool* open, float height) {
    ImVec2 origin = ImGui::GetCursorScreenPos();

    // 标题栏（自己占位 + 自绘）
    ImGui::PushID(id);
    ImGui::InvisibleButton("##header", ImVec2(ImGui::GetContentRegionAvail().x, 32.0f));
    const bool toggled = ImGui::IsItemClicked();
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 header_mn = ImGui::GetItemRectMin();
    const ImVec2 header_mx = ImGui::GetItemRectMax();
    if (toggled) {
        *open = !*open;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(header_mn, header_mx, hovered ? IM_COL32(0x2A, 0x30, 0x38, 0xFF)
                                                     : IM_COL32(0x1E, 0x22, 0x28, 0xFF));
    dl->AddRectFilled(header_mn, ImVec2(header_mn.x + 4.0f, header_mx.y), IM_COL32(0xE2, 0x1B, 0x25, 0xFF));
    dl->AddText(ImVec2(header_mn.x + 14.0f, header_mn.y + 7.0f), IM_COL32(0xF5, 0xF5, 0xF7, 0xFF), title);
    const char* arrow = *open ? "v" : ">";
    dl->AddText(ImVec2(header_mx.x - 22.0f, header_mn.y + 7.0f), IM_COL32(0x98, 0x98, 0xA1, 0xFF), arrow);
    ImGui::PopID();

    if (*open) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.10f, 0.12f, 1.0f));
        ImGui::BeginChild(id, ImVec2(0.0f, height), ImGuiChildFlags_None, ImGuiWindowFlags_None);
        ImGui::TextUnformatted("子区域内的内容，布局游标是独立的。");
        ImGui::TextUnformatted("可以放滚动内容、也可以嵌别处的控件。");
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    (void)origin;
}

// ============================================================ Lesson 8 =======
// 进阶：直接用 imgui_internal.h 的 ButtonBehavior 自己实现一个按钮。
//
// 什么时候需要：
//   想要 ImGui 内建按钮的完整行为（键盘激活、重复、拖出取消、Tooltip、
//   双击、ItemFlags、Popup 等）但视觉全自定义时。
//   上面的 InvisibleButton 方案已经够 90% 场景；只有需要精细控制交互语义时
//   才下沉到 ButtonBehavior。
//
// 代价：imgui_internal.h 不是稳定 API，升级 imgui 时可能需要跟着改。
bool InternalButtonBehaviorDemo(int* click_count) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }
    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID("##internal_button");
    const ImVec2 size(180.0f, 40.0f);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(size, ImGui::GetStyle().FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

    bool hovered = false;
    bool held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

    ImDrawList* dl = window->DrawList;
    dl->AddRectFilled(bb.Min, bb.Max, held ? IM_COL32(0x7E, 0x0E, 0x15, 0xFF)
                                           : (hovered ? IM_COL32(0xE2, 0x1B, 0x25, 0xFF)
                                                      : IM_COL32(0x24, 0x28, 0x2E, 0xFF)),
                      6.0f);
    dl->AddRect(bb.Min, bb.Max, IM_COL32(0xE2, 0x1B, 0x25, 0xFF), 6.0f, 0, 2.0f);
    const char* text = "ButtonBehavior 按钮";
    const ImVec2 text_size = ImGui::CalcTextSize(text);
    dl->AddText(ImVec2(bb.Min.x + (size.x - text_size.x) * 0.5f, bb.Min.y + (size.y - text_size.y) * 0.5f),
                IM_COL32(0xF5, 0xF5, 0xF7, 0xFF), text);
    if (pressed) {
        ++(*click_count);
    }
    return pressed;
}

} // namespace

// ------------------------------------------------------------------ 场景 ----

void WidgetDemoScene::OnRender(UiContext& ui) {
    if (!Components::BeginPanel(ui, "自定义控件 101", "InvisibleButton 占位 + DrawList 自绘")) {
        Components::EndPanel(ui);
        return;
    }

    LessonText("Lesson 1 · 最小控件",
               "CalcTextSize 定尺寸 -> GetCursorScreenPos 取原点 -> InvisibleButton 占位 -> "
               "在矩形里 AddRectFilled/AddText。点一下试试：");
    if (SimpleTag("##tag1", "点我", IM_COL32(0x2E, 0x6F, 0xB8, 0xFF))) {
        ui.Text("标签被点击了");
    }
    ImGui::SameLine();
    SimpleTag("##tag2", "只读标签", IM_COL32(0x44, 0x4A, 0x54, 0xFF));
    ImGui::Spacing();
    ImGui::Separator();

    LessonText("Lesson 2 · 交互状态 + dt 动画",
               "IsItemHovered / IsItemActive / IsItemClicked 拿到状态；动画用 io.DeltaTime 做指数趋近，"
               "保证 30/60/120 FPS 表现一致。按住看外扩波纹：");
    PulseDot("##dot1", 22.0f);
    ImGui::SameLine();
    PulseDot("##dot2", 22.0f);
    ImGui::SameLine();
    ui.TextDisabled("（鼠标悬停/按住；不依赖鼠标时用自己的输入层驱动同样状态）");
    ImGui::Separator();

    LessonText("Lesson 3 · 每实例状态 + 拖动",
               "ImGuiStorage 按 ID 存控件本地状态（等价于成员变量）；IsItemActive + IsMouseDragging "
               "实现拖动改值。点一下或按住左右拖：");
    for (int i = 0; i < 3; ++i) {
        ImGui::PushID(i);
        ToggleSwitch("##toggle", &toggle_value_[i], &toggle_state_[i]);
        ImGui::SameLine();
        ui.Text("开关 %d：%s", i + 1, toggle_value_[i] ? "开" : "关");
        ImGui::PopID();
    }
    ImGui::Separator();

    LessonText("Lesson 4 · 拖动型控件（滑块）",
               "IsItemActive 是拖动逻辑的入口；IsItemFocused + IsKeyPressed 补键盘路径；"
               "提示文本用 snprintf 写固定栈缓冲，避免每帧堆分配：");
    CustomSlider("##volume", &volume_, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ui.TextDisabled("拖动 / 方向键");
    ImGui::Separator();

    LessonText("Lesson 5 · 纯几何绘制（旋钮）",
               "PathArcTo + PathStroke 画弧，atan2 把鼠标方向变成角度，再映射成数值。"
               "注意屏幕 y 轴向下，sin 方向是反的：");
    Knob("##knob", &knob_, 46.0f);
    ImGui::SameLine();
    ui.Text("旋钮值：%.0f%%", knob_ * 100.0f);
    ImGui::Separator();

    LessonText("Lesson 6 · 循环里的控件（PushID）",
               "ImGui 靠 ID 识别控件：状态、动画、焦点都挂在 ID 上。循环里必须 PushID，"
               "否则会「改一个动全部」。GetItemRectMin/Max 拿已提交 item 的矩形最省事：");
    for (int i = 0; i < 4; ++i) {
        ui.Text("通道 %d", i + 1);
        ImGui::SameLine(120.0f);
        LevelBar(i, &pad_level_[i]);
    }
    ImGui::Separator();

    LessonText("Lesson 7 · 容器型控件（BeginChild）",
               "自己开子区域：标题栏用 InvisibleButton 自绘，内容用 BeginChild 获得独立布局/"
               "滚动。点标题栏折叠：");
    CustomPanel("##panel", "自定义面板", &panel_open_, 110.0f);
    ImGui::Separator();

    LessonText("Lesson 8 · 进阶（imgui_internal）",
               "需要内建按钮的完整语义（键盘激活、重复、拖出取消、Tooltip、双击）而视觉全自定义时，"
               "可以直接用 ButtonBehavior。代价：internal 头不是稳定 API：");
    static int click_count = 0;
    InternalButtonBehaviorDemo(&click_count);
    ImGui::SameLine();
    ui.Text("点击次数：%d", click_count);

    Components::EndPanel(ui, {});
}

void WidgetDemoApp::Configure(BackendConfig& cfg, PlatformKind kind) const {
    (void)kind;
    cfg.title = "GBAStation · 自定义控件教学";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.vsync = true;
    cfg.resizable = true;
#if defined(GUI_DEV_PLATFORM_switch)
    cfg.vsync = false;
#endif
    if (const char* window = std::getenv("GUI_DEV_WINDOW")) {
        int w = 0;
        int h = 0;
        if (std::sscanf(window, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
            cfg.width = w;
            cfg.height = h;
        }
    }
    if (std::getenv("GUI_DEV_NO_VSYNC")) {
        cfg.vsync = false;
    }
}

void WidgetDemoApp::OnStart(UiContext& ui) {
    (void)ui;
    Scenes().Reset(std::make_unique<WidgetDemoScene>());
}

void WidgetDemoApp::OnFrame(UiContext& ui, float dt) {
    (void)dt;
    // 冒烟测试：GUI_DEV_EXIT_AFTER=<帧数> 跑满后走正常退出（验证退出路径不崩）
    static const int exit_after = [] {
        const char* value = std::getenv("GUI_DEV_EXIT_AFTER");
        return value != nullptr ? std::atoi(value) : 0;
    }();
    if (exit_after > 0 && ++frame_ >= exit_after) {
        ui.GetBackend().RequestQuit();
    }
}

} // namespace gui_dev::demo
