// ============================================================================
// 课时 2 · 生命周期
// ============================================================================
//
// 【目标】
//   知道一个 App 从启动到退出会依次经过哪些阶段、每个阶段"什么东西已经可用"，
//   以及**析构顺序**为什么是这套框架里最容易崩的地方。
//
// ---------------------------------------------------------------------------
// 一、六个阶段（对应 src/core/App.cpp 的 AppRunner::Run）
// ---------------------------------------------------------------------------
//   ① Configure(cfg, platform)      还不存在窗口。只能改配置结构体，别碰 backend/ui。
//   ② backend->Init(cfg)            窗口 + 渲染器 + 输入就绪；SDL/平台服务可用。
//   ③ ImGui::CreateContext()        ImGui 上下文就绪（此时还没有后端渲染能力）。
//      backend->InitImGuiBackend()  两个 imgui_impl_* 就绪，可以调 ImGui API 了。
//      ui.RefreshIfDisplayChanged() 字体加载完成 —— 从这一刻起才画得出字。
//   ④ OnStart(ui)                   你的初始化：建场景、载资源。**每帧循环之前，只调一次**。
//   ⑤ 每帧：OnFrame -> Scenes().UpdateAll -> Scenes().RenderAll
//                                   dt 可用、绘制可用；不要在这里做重活（见课时 12）。
//   ⑥ 退出：OnShutdown -> 析构场景 -> ui.reset() -> 关 ImGui 后端/上下文 -> backend->Shutdown()
//
//   一句话记忆：**越靠后创建的东西，越靠前销毁**（栈式逆序）。
//
// ---------------------------------------------------------------------------
// 二、谁拥有谁（所有权图）
// ---------------------------------------------------------------------------
//   main()
//    └─ AppRunner（栈上临时对象）        拥有 backend_（unique_ptr<Backend>）与 ui_（unique_ptr<UiContext>）
//        └─ Backend（SDL2 后端）         拥有窗口/渲染器/pl 服务/romfs
//        └─ UiContext                    持有 Backend& 与 ImGui 生命周期
//        └─ App&  ← 你传入的（main 里的局部对象，比 AppRunner 活得久！）
//             └─ SceneStack              拥有所有 Scene（unique_ptr）
//                  └─ Scene               可能持有 TextureRef / 字体 / 大块数据
//
//   注意最后两层：**App 与 Scene 的生命周期比 Backend 长**。
//   这正是本项目真实踩过的坑 ↓
//
// ---------------------------------------------------------------------------
// 三、真实的崩溃案例（退出时 SIGSEGV）
// ---------------------------------------------------------------------------
//   现象：程序正常退出瞬间崩溃，栈顶是 TextureRef::~TextureRef -> Reset()
//         -> backend_->ReleaseTexture()，访问了 0x1088 这种野地址。
//
//   原因：main 里写的是 `AppRunner(app).Run();` —— AppRunner 是**临时对象**，
//         Run() 返回就析构，Backend 跟着没了；而 app 要到 main 结束才析构，
//         它持有的场景这时才析构，于是场景里的 TextureRef 回头调用了一个
//         **已经销毁的后端**。
//
//   两类修法（本项目都做了）：
//     1) 顺序修：AppRunner::Run() 在关后端之前显式 `app_.Scenes().Clear()`，
//        保证"持有后端资源的对象先死"。这是根本解法。
//     2) 兜底修：BackendLiveness（存活标记，见 src/platform/Backend.h）。
//        Backend 析构时把标记置 false，TextureRef 释放前先问一句：
//        后端还活着吗？不活就只打警告、不回调。于是"用错顺序"也不会崩。
//
//   教训：**任何持有"别人拥有的资源"的类，都要么明确生命周期约束，要么带存活标记。**
//
// ---------------------------------------------------------------------------
// 四、三条规则
// ---------------------------------------------------------------------------
//   R1  资源先于其依赖析构：先放掉引用（场景/纹理），再关被引用的东西（后端）。
//   R2  帧内不做生命周期操作：不要在 OnRender 里 push/pop 视图栈、不要 release 纹理 ——
//       容易在遍历中途销毁正在使用的对象。本项目用 RequestPush/RequestPop 延迟到下一帧。
//   R3  状态要能被重置：每个页面提供 OnEnter（重建/复位），别依赖"上次留下的值"。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 把本课时的 OnUpdate 日志从"每 60 帧一次"改成每帧一次，看日志被刷掉的速度，
//      理解为什么日志/调试输出本身也要限频。
//   2. 在 src/core/App.cpp 的 `app_.Scenes().Clear();` 那一行加注释说明它防的是什么，
//      然后**注释掉**它，跑一次 GUI_DEV_EXIT_AFTER=60 观察退出时的 stderr 警告
//      （BackendLiveness 兜住了崩溃，但会打印"纹理在后端销毁之后才释放"）。
//   3. 给自己的某个类加一个"存活标记"式保护，并写一条注释说明它防的顺序问题。
// ---------------------------------------------------------------------------
// 【验收】不看代码能回答：
//   1. 六个阶段的顺序？哪一阶段之后才画得出字？
//   2. 为什么 App/Scene 比 Backend 活得久？这会导致什么？
//   3. BackendLiveness 解决的是"顺序"问题还是"内存泄漏"问题？
//   4. 为什么改视图栈要延迟到下一帧？
// ============================================================================

#include <cstdarg>
#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseLog.h"
#include "platform/Backend.h"
#include "ui/Components.h"
#include "ui/Icons.h"
#include "ui/Theme.h"
#include "ui/UiContext.h"

namespace gui_dev::course {
namespace {

void Section(const char* title) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.89f, 0.11f, 0.15f, 1.0f));
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void Note(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.64f, 0.68f, 1.0f));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

void Code(const char* lines) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
    int line_count = 1;
    for (const char* p = lines; *p != '\0'; ++p) {
        if (*p == '\n') {
            ++line_count;
        }
    }
    ImGui::BeginChild(lines, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() *
                                              static_cast<float>(line_count) + 8.0f),
                      ImGuiChildFlags_None, ImGuiWindowFlags_None);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.85f, 0.72f, 1.0f));
    ImGui::TextUnformatted(lines);
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace

// ---------------------------------------------------------------------------
// 课时 2 的场景：把生命周期"跑给你看"
//
// 四个钩子各自打日志 —— 这就是本课时最直观的部分：
//   OnEnter  只在进入时一次
//   OnUpdate 每帧（这里限频到每 60 帧一次，顺便示范"调试输出要限频"）
//   OnRender 每帧绘制（不写日志，否则会把日志刷掉）
//   OnLeave  离开时一次 —— 注意它写进的是**场景之外**的共享日志，否则你看不到
// ---------------------------------------------------------------------------
class Lesson02LifecycleScene final : public Scene {
public:
    const char* Name() const override { return "lesson02"; }

    void OnEnter(UiContext& ui) override {
        (void)ui;
        LogEvent("L2 OnEnter      场景创建（每进入一次记一次）");
    }

    void OnUpdate(UiContext& ui, float dt) override {
        (void)ui;
        (void)dt;
        ++frames_;
        if (frames_ == 1 || frames_ % 60 == 0) {
            LogEvent("L2 OnUpdate     第 %d 帧（限频：每 60 帧记一次）", frames_);
        }
    }

    void OnLeave(UiContext& ui) override {
        (void)ui;
        // 关键：写共享日志。如果写在本场景的成员里，会随场景一起析构、屏上永远看不到。
        LogEvent("L2 OnLeave      场景析构前（生命周期最后一站）");
    }

    void OnRender(UiContext& ui) override {
        if (entered_render_ == 0) {
            entered_render_ = 1;
            LogEvent("L2 OnRender     首次绘制（此后每帧都会来）");
        }

        if (!Components::BeginPanel(ui, "课时 2 · 生命周期",
                                    "六阶段顺序 + 所有权 + 为什么析构顺序最容易崩")) {
            Components::EndPanel(ui);
            return;
        }

        // ---- 一、共享生命周期日志（本课时的核心）---------------------------
        Section("一、生命周期实况（跨场景共享日志）");
        Note("按 L/R 切换课时，再回来看这段：会看到上一课的 OnLeave 与下一课的 OnEnter 的先后。");
        Note("OnLeave 之所以能看到，是因为日志存在场景之外 —— 写在场景成员里会随它一起析构。");
        ImGui::Spacing();

        const int line_count = LogLineCount();
        for (int i = 0; i < line_count; ++i) {
            const CourseLogLine& line = LogLineAt(i);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.58f, 0.64f, 1.0f));
            ImGui::Text("%6.2fs  f%-5d", static_cast<double>(line.time), line.frame);
            ImGui::PopStyleColor();
            ImGui::SameLine(150.0f);
            // 最新一行高亮
            if (i == line_count - 1) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.93f, 0.93f, 0.96f, 1.0f));
                ImGui::TextUnformatted(line.text);
                ImGui::PopStyleColor();
            } else {
                ImGui::TextDisabled("%s", line.text);
            }
        }

        // ---- 二、六阶段 ----------------------------------------------------
        Section("二、六个阶段：越晚创建，越早销毁");
        Code("1. Configure(cfg, platform)      窗口还不存在：只改配置\n"
             "2. backend->Init(cfg)            窗口/渲染器/输入就绪\n"
             "3. ImGui::CreateContext()\n"
             "   backend->InitImGuiBackend()   ImGui API 可用\n"
             "   ui.RefreshIfDisplayChanged()  字体就绪 <- 从这之后才画得出字\n"
             "4. OnStart(ui)                   你的初始化，只调一次\n"
             "5. 每帧 OnFrame -> Update -> Render\n"
             "6. OnShutdown -> 析构场景 -> 关 ImGui -> 关后端");

        // ---- 三、所有权 ----------------------------------------------------
        Section("三、谁拥有谁：注意最后两层的寿命更长");
        Code("main()\n"
             " └ AddRunner(临时)  拥有 backend_ / ui_\n"
             "    └ Backend        窗口 / 渲染器 / pl / romfs\n"
             "    └ UiContext      Backend& + ImGui 生命周期\n"
             "    └ App&            <- 你传入的，比 AppRunner 活得久\n"
             "       └ SceneStack  拥有所有 Scene\n"
             "          └ Scene     可能持有 TextureRef 等后端资源");
        Note("App / Scene 比 Backend 活得久 —— 这就是下面那个崩溃的前提。");

        // ---- 四、真实崩溃案例 ----------------------------------------------
        Section("四、真实案例：退出瞬间 SIGSEGV");
        Note("栈顶：TextureRef::~TextureRef -> backend_->ReleaseTexture()，访问野地址。");
        Note("原因：场景比后端活得久，析构时去回调了已经销毁的后端。");
        Code("// 两类修法（本项目都做了）\n"
             "// 1) 顺序修（根本）：关后端之前先拆掉持有资源的对象\n"
             "app_.Scenes().Clear();        // 在 ui_.reset()/backend->Shutdown() 之前\n"
             "\n"
             "// 2) 兜底修：存活标记\n"
             "if (backend_ != nullptr && liveness_.Alive()) {\n"
             "    backend_->ReleaseTexture(texture_);   // 后端还活着才回调\n"
             "} else {\n"
             "    fprintf(stderr, \"纹理在后端销毁之后才释放，已跳过\\n\");\n"
             "}");

        // ---- 五、三条规则 --------------------------------------------------
        Section("五、三条规则");
        ImGui::BulletText("R1 资源先于其依赖析构：先放引用，再关被引用者");
        ImGui::BulletText("R2 帧内不做生命周期操作：改视图栈/释放资源都延迟到下一帧");
        ImGui::BulletText("R3 状态要能被重置：页面提供 OnEnter 复位，不依赖上次残留");

        // ---- 六、练习 / 验收 ------------------------------------------------
        Section("六、练习与验收（详见 Lesson02_Lifecycle.cpp 文件头）");
        ImGui::BulletText("练习 2：注释掉 App.cpp 里的 Scenes().Clear()，跑 GUI_DEV_EXIT_AFTER=60");
        ImGui::BulletText("        观察 BackendLiveness 兜住崩溃时打印的 stderr 警告");
        ImGui::BulletText("验收：六个阶段顺序？哪一阶段之后才画得出字？");
        ImGui::BulletText("验收：BackendLiveness 解决的是顺序问题还是泄漏问题？");

        Components::EndPanel(ui, {
            {Icons::Glyph(Icons::Button::L), "上一课"},
            {Icons::Glyph(Icons::Button::R), "下一课"},
            {Icons::Glyph(Icons::Button::B), "退出"},
        });
    }

private:
    int frames_ = 0;
    int entered_render_ = 0;
};

std::unique_ptr<Scene> CreateLesson02Lifecycle() {
    return std::make_unique<Lesson02LifecycleScene>();
}

} // namespace gui_dev::course
