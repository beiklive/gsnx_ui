// ============================================================================
// 课时 3 · 帧循环解剖
// ============================================================================
//
// 【目标】
//   把「一帧」拆开看清：每个阶段谁在跑、你必须做什么、为什么顺序不能乱。
//
// ---------------------------------------------------------------------------
// 一、一帧的五个动作（src/core/App.cpp 的循环体）
// ---------------------------------------------------------------------------
//   while (!backend->ShouldQuit()) {
//       ui.BeginFrame();          // ① 事件泵 + 输入采样 + backend.NewImGuiFrame()
//       ...你的 UI...             // ② 提交控件/绘制命令（立即模式：每帧都重新提交）
//       ui.EndFrame();            // ③ ImGui::Render() + 后端渲染 + Present
//   }
//
//   展开看就是：
//       ① SDL_PollEvent 循环   -> 把系统事件喂给 ImGui 后端
//          imgui_impl_sdl2_NewFrame() / _sdlrenderer2_NewFrame()  -> 准备 io
//          ImGui::NewFrame()    -> 开始新的一帧（此后才能调控件 API）
//       ② 你的场景/组件在这里画
//       ③ ImGui::Render()       -> 立即模式收尾：算出 ImDrawData（顶点/索引/命令）
//          后端把 ImDrawData 交给 SDL 渲染 -> Present
//
//   两个"必须"：
//     · NewFrame 与 Render 必须成对，且一帧只能一次 —— 漏了会断言，多了会错乱；
//     · 控件 API 只能夹在中间调用。在 NewFrame 之前调 → 崩；在 Render 之后调 → 丢这一帧。
//
// ---------------------------------------------------------------------------
// 二、为什么叫"立即模式"
// ---------------------------------------------------------------------------
//   ImGui **不保存 UI 树**。每帧你的代码都要重新"提交"一遍所有控件，它当场：
//     · 用布局游标算出每个 item 的矩形
//     · 把绘制命令写进 ImDrawList
//     · 交互状态靠 ID 在帧之间延续（这就是 ID 必须稳定的原因）
//   所以：控件函数返回的 "clicked" 只在本帧有效；不要把它存起来下帧再用。
//
// ---------------------------------------------------------------------------
// 三、dt 从哪来
// ---------------------------------------------------------------------------
//   后端每帧测时间差，写进 io.DeltaTime（本项目的 Sdl2Backend::NewImGuiFrame 做这件事）。
//   你通过 UiContext::DeltaTime() 拿到它，喂给动画（见课时 8）。
//   **不要用固定值当动画步长**：帧率在 60/120 之间摆动时，动画速度会跟着变。
//
// ---------------------------------------------------------------------------
// 四、一帧的开销花在哪
// ---------------------------------------------------------------------------
//   界面左下角给了实时的 ImDrawData 统计（命令数/顶点数/索引数）。
//   经验值：720p 下一个信息密度正常的页面 1~3k 顶点；上万就要查是不是画多了。
//   本课时的"人为增加开销"滑杆直接 busy-wait，让你看到主循环**没有帧率上限**
//   （详见课时 12：要锁帧得自己加，别指望 PRESENTVSYNC）。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 把滑杆拉到 20ms，观察 FPS 掉到 ~50：说明帧率由"真实耗时"决定（负载重就掉）。
//   2. 在 AppRunner::Run() 里把 `ui_->EndFrame()` 注释掉，跑一次——看断言信息
//      （这是理解 NewFrame/Render 成对的最快方式，看完记得改回来）。
//   3. 在场景的 OnRender 里调用两次 BeginPanel 而不 EndPanel，理解配对要求。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 说出 ①→③ 每个动作里发生了什么。
//   2. 为什么控件函数返回值只在本帧有效？
//   3. dt 从哪来、为什么动画必须用它？
//   4. 立即模式与"保留模式"（如 Qt/HTML DOM）在代码写法上的区别？
// ============================================================================

#include <chrono>
#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseLog.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {

class Lesson03FrameLoopScene final : public Scene {
public:
    const char* Name() const override { return "lesson03"; }

    void OnEnter(UiContext& ui) override {
        (void)ui;
        LogEvent("L3 OnEnter      帧循环课时开始");
    }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 3 · 帧循环解剖",
                                    "NewFrame -> 你的 UI -> Render/Present，一帧一次且必须成对")) {
            Components::EndPanel(ui);
            return;
        }

        const ImGuiIO& io = ImGui::GetIO();
        const ImDrawData* draw_data = ImGui::GetDrawData();

        Section("一、本帧实况（这就是「一帧」的全部信息）");
        KeyValue("帧号", "%d", ImGui::GetFrameCount());
        KeyValue("dt（本帧间隔）", "%.2f ms", static_cast<double>(io.DeltaTime * 1000.0f));
        KeyValue("FPS（平滑值）", "%.1f", static_cast<double>(io.Framerate));
        KeyValue("命令列表数", "%d", draw_data != nullptr ? draw_data->CmdListsCount : 0);
        KeyValue("顶点 / 索引", "%d / %d", draw_data != nullptr ? draw_data->TotalVtxCount : 0,
                 draw_data != nullptr ? draw_data->TotalIdxCount : 0);
        KeyValue("活动纹理数", "%d", ImGui::GetPlatformIO().Textures.Size);

        Section("二、一帧的五个动作");
        Code("while (!backend->ShouldQuit()) {\n"
             "    ui.BeginFrame();     // 1) PollEvent -> 喂给 ImGui 后端\n"
             "                         // 2) backend.NewImGuiFrame() -> ImGui::NewFrame()\n"
             "    ...你的 UI...        // 3) 提交控件与绘制命令（每帧重新提交！）\n"
             "    ui.EndFrame();       // 4) ImGui::Render() -> ImDrawData\n"
             "}                        // 5) 后端渲染并 Present");

        Section("三、立即模式的三个直接后果");
        Bullet("控件返回值（clicked/hovered）只在本帧有效，不要存起来下帧用");
        Bullet("每帧都要重新提交全部 UI —— 所以每帧的开销要可控（课时 12）");
        Bullet("交互状态靠 ID 跨帧延续 —— 所以 ID 必须稳定（课时 5/6）");

        Section("四、亲手感受：主循环没有帧率上限");
        ImGui::SliderFloat("人为增加本帧开销 (ms)", &extra_ms_, 0.0f, 30.0f, "%.1f ms");
        Note("拉高它 -> FPS 下降。说明帧率 = min(显示器/后端上限, 1 / 真实耗时)。");
        Note("本项目实测：轻负载 ~120 FPS，重负载刚好掉到 60（见课时 12 的测量）。");
        if (extra_ms_ > 0.1f) {
            // 用 std::chrono busy-wait（UI 层不依赖平台 API，所以不用 sleep/usleep）
            const auto begin = std::chrono::steady_clock::now();
            while (std::chrono::duration<float, std::milli>(
                       std::chrono::steady_clock::now() - begin)
                       .count() < extra_ms_) {
            }
        }

        Section("五、练习与验收（完整版见本文件头部注释）");
        Bullet("练习：把 EndFrame() 注释掉跑一次，看断言 —— 最直观地理解配对要求");
        Bullet("验收：①→③ 各自发生了什么？为什么返回值只在本帧有效？");

        EndLesson(ui);
    }

private:
    float extra_ms_ = 0.0f;
};

std::unique_ptr<Scene> CreateLesson03FrameLoop() { return std::make_unique<Lesson03FrameLoopScene>(); }

} // namespace gui_dev::course
