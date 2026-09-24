// ============================================================================
// 课时 5 · 控件制作
// ============================================================================
//
// 【目标】
//   掌握自定义控件的四步法，并能独立做出一个新控件。
//
// ---------------------------------------------------------------------------
// 一、四步法（背下来）
// ---------------------------------------------------------------------------
//   ① 量尺寸：ImGui::CalcTextSize / 固定值 -> 算出控件矩形
//   ② 占位：  ImGui::InvisibleButton(id, size) —— 布局、命中、ID、裁剪一次搞定
//   ③ 自绘：  ImDrawList（AddRectFilled / AddConvexPolyFilled / AddText / PathStroke ...）
//   ④ 状态：  hover/active/动画存帧外（成员或 ImGuiStorage），动画吃 dt
//
//   为什么不用 ImGui::Button：它自带一套固定视觉（圆角+填充+内建文字），
//   改到"斜切/漫画风"要跟它的样式系统对着干；不如占位 + 全自绘，完全可控。
//
// ---------------------------------------------------------------------------
// 二、本课时要做的控件：分段选择器 SegmentedControl
// ---------------------------------------------------------------------------
//   需求（真实项目里很常见）：
//     · 3~5 段，互斥选中；当前段用红色块 + 白字，其余暗色
//     · 选中段有一个"滑块"从旧位置动画滑到新位置（吃 dt）
//     · 支持鼠标点选 + 键盘/手柄左右切换（聚焦时）
//     · 高 DPI / 不同字号下靠 CalcTextSize 自适应宽度，不写死
//   实现见下方 SegmentedControl()，按 ①~④ 的注释顺序读。
//
// ---------------------------------------------------------------------------
// 三、容易踩的坑（都是本项目的真实教训）
// ---------------------------------------------------------------------------
//   · 只 SetCursorScreenPos 然后画、不提交 item -> 后面的控件会叠上来；
//   · 循环里用同名控件而不 PushID -> 状态互相串（课时 10 会专门演示）；
//   · 每帧构造 std::string 拼文字 -> 每帧堆分配（课时 12）；
//   · 动画直接 +=0.1f -> 帧率变了速度就变（课时 8）；
//   · SetCursorPos 之后不补 item -> imgui 1.92 直接断言。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 给 SegmentedControl 加第 4 段，观察它自动等分（说明宽度是算出来的，不是写死的）。
//   2. 把滑块动画的 SmoothTo 换成 `+= 0.1f`，把窗口拉大/缩小，看动画速度变化。
//   3. 仿照它做第二个控件：三态开关（关/自动/开），复用同样的四步法。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 说出四步法每一步用的 API。
//   2. 为什么必须提交 item？不提交会怎样？
//   3. CalcTextSize 在自定义控件里起什么作用？
// ============================================================================

#include <cmath>
#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {
namespace {

constexpr float kPi = 3.14159265358979323846f;

// 帧率无关的指数趋近（详见课时 8）
float SmoothTo(float current, float target, float speed, float dt) {
    return current + (target - current) * (1.0f - std::exp(-speed * dt));
}

// 复用本课时的状态：真实项目应放进类成员（这里放 static 只为让演示代码更短）
struct SegmentedState {
    float knob = 0.0f; // 滑块当前位置（动画值）
    bool was_focused = false;
};

// ===========================================================================
// 自定义控件：分段选择器（四步法示例）
//   items/count : 段名
//   value       : 当前选中段（读写）
//   返回        : 本帧是否被点击（点击已就地写入 *value）
// ===========================================================================
bool SegmentedControl(const char* id, const char* const* items, int count, int* value,
                      SegmentedState& state) {
    // ---- ① 量尺寸：每段宽度 = 文本宽 + 左右 padding，取最大值，保证等分且不挤字
    const ImGuiStyle& style = ImGui::GetStyle();
    const float text_height = ImGui::GetTextLineHeight();
    float segment_width = 0.0f;
    for (int i = 0; i < count; ++i) {
        const float w = ImGui::CalcTextSize(items[i]).x + style.FramePadding.x * 6.0f;
        if (w > segment_width) {
            segment_width = w;
        }
    }
    const float height = text_height + style.FramePadding.y * 4.0f;
    const ImVec2 size(segment_width * static_cast<float>(count), height);

    // ---- ② 占位：InvisibleButton 负责布局 + 命中 + ID
    const ImVec2 mn = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool focused = ImGui::IsItemFocused();
    const ImVec2 mx(mn.x + size.x, mn.y + size.y);

    // 鼠标点选：按 x 落在第几段
    if (clicked) {
        const float t = (ImGui::GetMousePos().x - mn.x) / size.x;
        int index = static_cast<int>(t * static_cast<float>(count));
        if (index < 0) {
            index = 0;
        }
        if (index >= count) {
            index = count - 1;
        }
        *value = index;
    }
    // 键盘/手柄（聚焦时）：本项目由 Scene::OnInput 把手柄方向键翻译成 ImGui 按键
    if (focused) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
            *value = (*value + count - 1) % count;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
            *value = (*value + 1) % count;
        }
    }

    // ---- ④ 状态：滑块位置做动画（帧率无关）
    const float dt = ImGui::GetIO().DeltaTime;
    state.knob = SmoothTo(state.knob, static_cast<float>(*value), 18.0f, dt);

    // ---- ③ 自绘
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(mn, mx, IM_COL32(0x18, 0x1B, 0x20, 0xFF), 6.0f);
    // 选中滑块（红色，位置来自动画值）
    const float seg_w = size.x / static_cast<float>(count);
    const ImVec2 knob_min(mn.x + seg_w * state.knob + 2.0f, mn.y + 2.0f);
    const ImVec2 knob_max(knob_min.x + seg_w - 4.0f, mx.y - 2.0f);
    dl->AddRectFilled(knob_min, knob_max, IM_COL32(0xE2, 0x1B, 0x25, 0xFF), 4.0f);
    // 分段文字
    for (int i = 0; i < count; ++i) {
        const ImVec2 text = ImGui::CalcTextSize(items[i]);
        const float cx = mn.x + seg_w * (static_cast<float>(i) + 0.5f) - text.x * 0.5f;
        const float cy = mn.y + (size.y - text.y) * 0.5f;
        const bool selected = i == *value;
        dl->AddText(ImVec2(cx, cy), selected ? IM_COL32(0xFF, 0xFF, 0xFF, 0xFF)
                                             : IM_COL32(0x98, 0x98, 0xA1, 0xFF),
                    items[i]);
    }
    // 边框：悬浮/聚焦时更亮
    const ImU32 border = focused ? IM_COL32(0x4F, 0xA3, 0xFF, 0xFF)
                                 : (hovered ? IM_COL32(0xFF, 0xFF, 0xFF, 0xB0)
                                            : IM_COL32(0xFF, 0xFF, 0xFF, 0x50));
    dl->AddRect(mn, mx, border, 6.0f, 0, focused ? 2.5f : 1.5f);
    state.was_focused = focused;
    return clicked;
}

// 第二个控件：环形进度（示范"几何绘制"）
void RingProgress(const char* id, float value01, float radius) {
    const ImVec2 mn = ImGui::GetCursorScreenPos();
    const ImVec2 size(radius * 2.0f + 8.0f, radius * 2.0f + 8.0f);
    ImGui::InvisibleButton(id, size); // 只占位，不交互
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 center(mn.x + size.x * 0.5f, mn.y + size.y * 0.5f);
    dl->PathClear();
    dl->PathArcTo(center, radius, -kPi * 0.5f, kPi * 1.5f, 48);
    dl->PathStroke(IM_COL32(0x33, 0x39, 0x42, 0xFF), 0, 6.0f);
    dl->PathClear();
    dl->PathArcTo(center, radius, -kPi * 0.5f, -kPi * 0.5f + 2.0f * kPi * value01, 48);
    dl->PathStroke(IM_COL32(0xE2, 0x1B, 0x25, 0xFF), 0, 6.0f);
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(value01 * 100.0f + 0.5f));
    const ImVec2 text = ImGui::CalcTextSize(buffer);
    dl->AddText(ImVec2(center.x - text.x * 0.5f, center.y - text.y * 0.5f),
                IM_COL32(0xF5, 0xF5, 0xF7, 0xFF), buffer);
}

} // namespace

class Lesson05WidgetScene final : public Scene {
public:
    const char* Name() const override { return "lesson05"; }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 5 · 控件制作",
                                    "四步法：量尺寸 -> 占位 -> 自绘 -> 状态")) {
            Components::EndPanel(ui);
            return;
        }

        Section("一、四步法");
        Code("① 量尺寸  CalcTextSize / 固定值\n"
             "② 占位    InvisibleButton(id, size)   <- 布局+命中+ID+裁剪\n"
             "③ 自绘    GetWindowDrawList()->AddRectFilled/AddText/PathStroke...\n"
             "④ 状态    hover/active/动画存帧外；动画用 io.DeltaTime");
        Note("为什么不用 ImGui::Button：它自带固定视觉，改风格要跟它的样式系统对抗。");

        Section("二、示例控件：分段选择器（SegmentedControl）");
        Note("鼠标点选；用 Tab/方向键聚焦后可用 ←→ 切换（手柄走 Scene::OnInput 翻译）。");
        static const char* kItems[4] = {"关闭", "省电", "均衡", "性能"};
        int changed = selected_;
        SegmentedControl("##seg", kItems, 4, &selected_, segment_state_);
        if (changed != selected_) {
            Note("选中切换为：%s（滑块会滑过去，吃 dt）", kItems[selected_]);
        }
        ImGui::SameLine();
        RingProgress("##ring", static_cast<float>(selected_) / 3.0f, 26.0f);

        Section("三、拆解：每一步对应哪几行代码");
        Code("// ① 等分宽度 = 最长文本 + padding（不写死，字号变了自动适配）\n"
             "segment_width = CalcTextSize(items[i]).x + FramePadding.x*6\n"
             "\n"
             "// ② 占位：返回本帧是否被点击\n"
             "clicked = InvisibleButton(id, size)\n"
             "focused = IsItemFocused()\n"
             "\n"
             "// ③ 自绘：红滑块 + 四段文字 + 边框\n"
             "knob = lerp(旧值, 选中段, 动画)\n"
             "AddRectFilled(knob_min, knob_max, 红)\n"
             "AddText(每段居中, 选中白 / 未选灰)\n"
             "\n"
             "// ④ 状态：滑块位置做帧率无关插值\n"
             "state.knob = SmoothTo(state.knob, *value, 18, io.DeltaTime)");

        Section("四、坑（都是真实踩过的）");
        Bullet("只 SetCursorScreenPos 不提交 item -> 后面的控件叠上来");
        Bullet("循环里同名控件不 PushID -> 状态串味（课时 10 演示）");
        Bullet("每帧拼 std::string -> 每帧堆分配（课时 12）");
        Bullet("动画 +=0.1f -> 帧率变了速度就变（课时 8）");

        Section("五、练习与验收");
        Bullet("练习：加第 4 段（已加）看自动等分；把 SmoothTo 换成 +=0.1f 对比");
        Bullet("练习：仿照它做三态开关（关/自动/开）");
        Bullet("验收：四步法每步的 API？为什么必须占位？CalcTextSize 的作用？");

        EndLesson(ui);
    }

private:
    static SegmentedState segment_state_;
    static int selected_;
};

SegmentedState Lesson05WidgetScene::segment_state_{};
int Lesson05WidgetScene::selected_ = 2;

std::unique_ptr<Scene> CreateLesson05Widget() { return std::make_unique<Lesson05WidgetScene>(); }

} // namespace gui_dev::course
