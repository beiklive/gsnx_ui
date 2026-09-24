// ============================================================================
// 课时 10 · 容器与复合控件
// ============================================================================
//
// 【目标】
//   会做"自己的容器"（可折叠分组、滚动区、卡片），并正确处理多实例状态。
//
// ---------------------------------------------------------------------------
// 一、复合控件的组成
// ---------------------------------------------------------------------------
//   复合控件 = 自绘外壳（标题栏/边框/折叠箭头）
//            + 一个容器（BeginChild）承载内容
//            + 自己的状态（展开与否、滚动位置）
//   关键 API：
//     BeginChild(id, size, flags, window_flags) / EndChild
//        · 规则和 Begin/End **不同**：无论 BeginChild 返回 true/false，都必须 EndChild
//        · size = (0,0) 表示填满剩余空间；负高度表示"剩余 - N"
//        · child 内部是独立窗口：独立游标、独立滚动、独立裁剪
//     GetWindowPos/GetWindowSize/GetWindowDrawList  在 child 内部取它自己的几何
//
// ---------------------------------------------------------------------------
// 二、折叠是怎么做的
// ---------------------------------------------------------------------------
//   标题栏自己画（InvisibleButton 占位 + 自绘），点击切换 open；
//   内容区用 if (open) { BeginChild(...) ... EndChild(); } 决定是否渲染。
//   折叠状态属于"应当保留"的状态（返回页面还在），但要考虑：
//     · 多实例不能共用（见下）
//     · 是否记忆到配置文件（由业务决定，UI 只提供值）
//
// ---------------------------------------------------------------------------
// 三、多实例状态：本课时最重要的一节
// ---------------------------------------------------------------------------
//   下面有**两个**分组，它们共用同一份组件代码。请点开一个、再点开另一个观察：
//     · 正确做法：状态放在"每个实例一份"的地方（结构体成员 / ImGuiStorage 按 ID）
//     · 错误做法：状态用 static 局部变量 —— 两个分组会互相串味
//   本课时给了一个开关，让你**制造**这个 bug 并亲眼看到：
//     勾选"用 static 共享状态" -> 点 A 分组，B 分组跟着变。
//   本项目所有正式控件都用成员变量（见 GameMenuButton / GameMenuSaveSlot）。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 打开"用 static 共享状态"，点一个分组，观察另一个跟着变 —— 这就是串味。
//   2. 把分组内容的 BeginChild 高度从 80 改成 0（填满），感受差别。
//   3. 仿照它做一个"可折叠 + 右侧带数值"的设置分组（复合控件综合练习）。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 复合控件由哪三部分组成？
//   2. BeginChild/EndChild 与 Begin/End 的配对规则差别？
//   3. 为什么状态用 static 会串味？正确的放法是什么？
// ============================================================================

#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {
namespace {

// 复合控件：可折叠分组
//   state 由调用方提供（每个实例一份）—— 这是正确做法的关键
struct CollapsibleGroupState {
    bool open = true;
    float content_scroll = 0.0f;
};

// shared_state 仅用于演示"用 static 会串味"这个反例
bool CollapsibleGroup(const char* id, const char* title, CollapsibleGroupState& state,
                      bool use_shared_static, bool& shared_static_open, float content_height) {
    ImGui::PushID(id);

    // ---- 外壳：自绘标题栏（占位 + 自绘 + 命中）---------------------------
    const float header_h = 34.0f;
    const ImVec2 mn = ImGui::GetCursorScreenPos();
    const ImVec2 size(ImGui::GetContentRegionAvail().x, header_h);
    const bool clicked = ImGui::InvisibleButton("##header", size);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 mx(mn.x + size.x, mn.y + size.y);

    bool& open = use_shared_static ? shared_static_open : state.open;
    if (clicked) {
        open = !open;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(mn, mx, hovered ? IM_COL32(0x2A, 0x30, 0x38, 0xFF)
                                       : IM_COL32(0x1E, 0x22, 0x28, 0xFF));
    dl->AddRectFilled(mn, ImVec2(mn.x + 4.0f, mx.y), IM_COL32(0xE2, 0x1B, 0x25, 0xFF));
    dl->AddText(ImVec2(mn.x + 14.0f, mn.y + 8.0f), IM_COL32(0xF5, 0xF5, 0xF7, 0xFF), title);
    dl->AddText(ImVec2(mx.x - 22.0f, mn.y + 8.0f), IM_COL32(0x98, 0x98, 0xA1, 0xFF),
                open ? "v" : ">");

    // ---- 内容区：容器 ----------------------------------------------------
    if (open) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.10f, 0.12f, 1.0f));
        ImGui::BeginChild("##content", ImVec2(0.0f, content_height), ImGuiChildFlags_None,
                          ImGuiWindowFlags_None);
        for (int i = 0; i < 6; ++i) {
            ImGui::Text("内容行 %d —— 这个子区域有自己的滚动条", i + 1);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::PopID();
    return clicked;
}

} // namespace

class Lesson10ContainerScene final : public Scene {
public:
    const char* Name() const override { return "lesson10"; }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 10 · 容器与复合控件",
                                    "自绘外壳 + BeginChild 内容 + 每实例状态")) {
            Components::EndPanel(ui);
            return;
        }

        Section("一、复合控件 = 外壳 + 容器 + 状态");
        Code("外壳    InvisibleButton 占位 + 自绘标题栏/边框/箭头\n"
             "容器    if (open) { BeginChild(id, size); ...; EndChild(); }\n"
             "状态    bool open;  每个实例一份（成员/ImGuiStorage）\n"
             "\n"
             "注意：BeginChild/EndChild 与 Begin/End 规则不同 ——\n"
             "     无论 BeginChild 返回 true/false，都必须调用 EndChild。");

        // ---- 二、两个实例 --------------------------------------------------
        Section("二、两个分组实例：点开来对比");
        ImGui::Checkbox("用 static 共享状态（制造串味 bug）", &use_shared_static_);
        ImGui::SameLine();
        ImGui::TextDisabled(use_shared_static_ ? "现在点 A，B 会跟着变" : "正常：各自独立");
        ImGui::Spacing();

        CollapsibleGroup("groupA", "分组 A · 视频", group_a_, use_shared_static_,
                         shared_static_open_, 80.0f);
        CollapsibleGroup("groupB", "分组 B · 音频", group_b_, use_shared_static_,
                         shared_static_open_, 80.0f);

        KeyValue("A.open（成员）", "%s", group_a_.open ? "开" : "关");
        KeyValue("B.open（成员）", "%s", group_b_.open ? "开" : "关");
        KeyValue("共享 static", "%s", shared_static_open_ ? "开" : "关");

        // ---- 三、为什么 ------------------------------------------------
        Section("三、为什么会串味");
        Code("// 错误：static 局部变量 —— 所有实例共用一份\n"
             "bool& open() { static bool v = true; return v; }\n"
             "\n"
             "// 正确 A：结构体成员（首选）\n"
             "struct State { bool open = true; };\n"
             "\n"
             "// 正确 B：按 ID 存（自由函数控件）\n"
             "bool* open = &ImGui::GetStateStorage()->GetIntRef(ImGui::GetID(\"open\"), 1);");

        Section("四、三件容易忘的事");
        Bullet("BeginChild 里外是两套游标：里面的 GetCursorScreenPos 是 child 自己的");
        Bullet("BeginChild 的尺寸参数：0 = 填满剩余；负数 = 剩余 - N（本项目页脚就是这么留位的）");
        Bullet("折叠状态下**不要**提交内容 —— 否则布局会留出空白（除非你就是要保留高度）");

        Section("五、练习与验收");
        Bullet("练习：勾上共享状态，点 A 看 B 跟随 —— 制造并理解串味");
        Bullet("练习：做一个「可折叠 + 右侧带数值」的设置分组");
        Bullet("验收：复合控件三部分？BeginChild 配对规则？为什么 static 会串味？");

        EndLesson(ui);
    }

private:
    CollapsibleGroupState group_a_{};
    CollapsibleGroupState group_b_{};
    bool use_shared_static_ = false;
    bool shared_static_open_ = true;
};

std::unique_ptr<Scene> CreateLesson10Container() { return std::make_unique<Lesson10ContainerScene>(); }

} // namespace gui_dev::course
