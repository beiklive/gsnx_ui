// ============================================================================
// 课时 7 · 状态管理
// ============================================================================
//
// 【目标】
//   知道状态该放哪、什么时候必须复位、怎么让 UI 与数据解耦。
//
// ---------------------------------------------------------------------------
// 一、状态放哪：三选一
// ---------------------------------------------------------------------------
//   ① 类成员（首选）
//        最直观、生命周期随对象、可被多实例共享（每个实例一份）。
//        本项目所有正式控件都用它：GameMenuButton::anim_ / plate_ …
//   ② ImGuiStorage（按 ID 存）
//        ImGui::GetStateStorage() -> GetFloatRef(id, default) / GetIntRef / GetVoidPtr
//        适合"控件函数是自由函数、不想改签名"的场景；键是控件 ID，天然按实例区分。
//   ③ 全局/静态（尽量别用）
//        多实例会互相串；跨页面会残留；测试困难。只适合真正的进程级单例。
//
//   放错地方的典型症状：开关 A 一开，开关 B 也跟着开（用了 static）；
//   或者"进页面一次、数值翻倍"（忘记复位）。
//
// ---------------------------------------------------------------------------
// 二、什么时候必须复位
// ---------------------------------------------------------------------------
//   页面/场景重新进入时，需要区分两类状态：
//     · 应当保留：用户的选择、滚动位置、当前页 —— "返回上一页再回来"要还在
//     · 应当复位：动画进度、一次性效果、按下状态、临时缓冲 —— 否则会出现
//                 "再次打开菜单时动画从上次的值接着跑"这种怪异观感
//   本项目的统一入口是 OnEnter/OnLeave（课时 2 已加进 Scene，GameMenuView 也有）：
//       void OnEnter(...) override {  // 只在这里 Reset 动画
//           for (auto& item : items_) item.Reset();
//       }
//
// ---------------------------------------------------------------------------
// 三、UI 与数据解耦：一切动作走接口
// ---------------------------------------------------------------------------
//   页面**不直接**调用模拟核心/存档系统，而是通过一个 Delegate 接口出去：
//       class SettingsDelegate { virtual void OnSettingChanged(...) = 0; };
//   好处：
//     · UI 可以先做（Mock 实现），核心接好再换真实实现（本项目的暂停菜单就是这样）；
//     · 便于测试：塞一个记录用的假实现就能验证"点一下发了什么"；
//     · 编译期隔离：UI 编译不依赖核心头文件。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 把本课时的"返回时保留滚动"改成"总是重置"，观察差别。
//   2. 给向导加第 3 页，并让第 1 页的选择在第 3 页显示出来（跨页状态）。
//   3. 把 ImGuiStorage 的那个计数器删掉，改成 static 全局，制造"两个实例互相串味"，
//      再改回成员变量 —— 体会为什么成员是首选。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 三种状态载体的取舍？
//   2. 哪些状态必须复位、哪些必须保留？为什么？
//   3. Delegate 解耦解决了什么问题？
// ============================================================================

#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseLog.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {
namespace {

// 数据出口：页面只认这个接口，不认具体实现（真实项目里由核心侧实现）
class SettingsSink {
public:
    virtual ~SettingsSink() = default;
    virtual void OnValueChanged(const char* page, const char* key, int value) = 0;
};

} // namespace

class Lesson07StateScene final : public Scene, public SettingsSink {
public:
    const char* Name() const override { return "lesson07"; }

    void OnEnter(UiContext& ui) override {
        (void)ui;
        // 复位"应当复位"的东西（动画/临时值）；"应当保留"的（选择/页码）不动
        frame_counted_ = 0;
        last_event_[0] = '\0';
        LogEvent("L7 OnEnter      复位临时状态，保留选择与页码");
    }

    void OnLeave(UiContext& ui) override {
        (void)ui;
        LogEvent("L7 OnLeave      页面销毁前的最后一站");
    }

    // SettingsSink：所有修改都从这里出去
    void OnValueChanged(const char* page, const char* key, int value) override {
        std::snprintf(last_event_, sizeof(last_event_), "%s / %s = %d", page, key, value);
    }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 7 · 状态管理",
                                    "状态放哪 / 何时复位 / 怎么与数据解耦")) {
            Components::EndPanel(ui);
            return;
        }
        ++frame_counted_;
        if (frame_counted_ == 1) {
            LogEvent("L7 首帧渲染     （OnEnter 已经跑过了）");
        }

        // ---- 一、当前状态一览（成员状态）-----------------------------------
        Section("一、本页面的状态（成员变量的真实值）");
        KeyValue("page_（页码，应保留）", "%d", page_);
        KeyValue("choice_（选择，应保留）", "%d", choice_);
        KeyValue("frame_counted_（临时，已复位）", "%d", frame_counted_);
        Note("切到别的课时再切回来：前两个保留、第三个从 0 重新数 —— 这就是要区分的两类状态。");

        // ---- 二、ImGuiStorage ---------------------------------------------
        Section("二、ImGuiStorage：按 ID 存控件本地状态（自由函数控件用它）");
        const ImGuiID storage_key = ImGui::GetID("##storage_counter");
        int* counter = ImGui::GetStateStorage()->GetIntRef(storage_key, 0);
        if (ImGui::Button("Storage 计数 +1")) {
            ++(*counter);
        }
        ImGui::SameLine();
        ImGui::Text("storage 计数 = %d", *counter);
        Note("键是控件 ID，天然按实例区分；无需改控件函数签名。");

        // ---- 三、两类状态的复位演示 ----------------------------------------
        Section("三、复位策略：动画/临时值 vs 用户选择");
        ImGui::Checkbox("返回时保留页码与选择（推荐）", &keep_on_leave_);
        if (!keep_on_leave_) {
            // 模拟"总是重置"的行为：每帧都复位 —— 用户会觉得页面"记不住"
            page_ = 0;
            choice_ = 0;
        }

        // ---- 四、向导页 ----------------------------------------------------
        Section("四、向导（两页 + 返回时如何取舍）");
        if (page_ == 0) {
            Note("第 1 页：选择模式");
            static const char* kModes[] = {"标准", "省电", "高性能"};
            const int before = choice_;
            for (int i = 0; i < 3; ++i) {
                ImGui::RadioButton(kModes[i], &choice_, i);
                if (i < 2) {
                    ImGui::SameLine();
                }
            }
            if (before != choice_) {
                OnValueChanged("第1页", "模式", choice_);
            }
            if (ImGui::Button("下一步 ->")) {
                page_ = 1;
            }
        } else {
            Note("第 2 页：确认（回到第 1 页会看到选择被保留）");
            static const char* kModes[] = {"标准", "省电", "高性能"};
            ImGui::Text("你选择的模式：%s", kModes[choice_ < 3 ? choice_ : 0]);
            if (ImGui::Button("<- 返回")) {
                page_ = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("完成")) {
                OnValueChanged("第2页", "确认", choice_);
                page_ = 0;
            }
        }
        ImGui::TextDisabled("最后一次数据变更：%s", last_event_[0] != '\0' ? last_event_ : "（无）");

        // ---- 五、解耦说明 --------------------------------------------------
        Section("五、UI 与数据解耦：一切动作走接口");
        Code("class SettingsSink {            // 页面只认这个接口\n"
             "public:\n"
             "    virtual void OnValueChanged(const char*, const char*, int) = 0;\n"
             "};\n"
             "// 页面里：if (变化) sink.OnValueChanged(\"第1页\", \"模式\", v);\n"
             "// demo 里是 Mock；接真实核心时只换实现，页面一行不改");

        Section("六、练习与验收");
        Bullet("练习：勾掉上面的复选框，感受「每帧复位」是什么体验");
        Bullet("练习：把 storage 计数换成 static 全局，制造两个实例串味");
        Bullet("验收：三种状态载体取舍？哪些状态必须复位？Delegate 解耦解决了什么？");

        EndLesson(ui);
    }

private:
    int page_ = 0;          // 应保留
    int choice_ = 0;        // 应保留
    int frame_counted_ = 0; // 应复位
    bool keep_on_leave_ = true;
    char last_event_[96] = {};
};

std::unique_ptr<Scene> CreateLesson07State() { return std::make_unique<Lesson07StateScene>(); }

} // namespace gui_dev::course
