// ============================================================================
// 课时 1 · 建立窗口
// ============================================================================
//
// 【目标】
//   搞清一个 ImGui 窗口是怎么起来的；分清哪些是"框架给你的"，哪些是"你要写的"。
//
// ---------------------------------------------------------------------------
// 一、先破除一个误解：ImGui 不建窗口
// ---------------------------------------------------------------------------
//   Dear ImGui 是**纯绘制库**：它不知道 SDL / Cocoa / Win32 的存在，也不会
//   创建窗口、不会处理系统事件。它只做两件事：
//     · 每帧接收"控件调用"（Button/Text/你自绘的），算出顶点与绘制命令；
//     · 把绘制命令交给后端去渲染。
//   所以任何 ImGui 项目必然有三层：
//
//     ┌───────────────────────────────────────────────────────────┐
//     │ 你的 UI       每帧提交控件与绘制                           │  src/ui, src/gamemenu
//     ├───────────────────────────────────────────────────────────┤
//     │ ImGui 后端    imgui_impl_sdl2 / imgui_impl_sdlrenderer2    │  third_party/imgui/backends
//     │               （把系统事件喂给 io，把 DrawData 画出来）     │
//     ├───────────────────────────────────────────────────────────┤
//     │ 平台后端      窗口 + 渲染器 + 事件循环                     │  src/platform/
//     └───────────────────────────────────────────────────────────┘
//
//   本项目把"平台后端"抽象成 gui_dev::Backend 接口，好处是：
//   src/ui 与 src/gamemenu 里没有任何 SDL 头文件 —— 换后端（GLFW/deko3D/…)不影响 UI。
//
// ---------------------------------------------------------------------------
// 二、GUI_DEV 的启动链条（这就是本课时的"示例代码"）
// ---------------------------------------------------------------------------
//   main.cpp:
//       gui_dev::course::CourseApp app;
//       return gui_dev::AppRunner(app).Run();
//
//   AppRunner::Run() 内部顺序（对应 src/core/App.cpp）：
//       1. app.Configure(cfg, kPlatform)          ← 【你写】配窗口参数
//       2. backend = CreatePlatformBackend()      ← 平台后端（SDL2）
//          backend->Init(cfg)                     ← SDL_CreateWindow / SDL_CreateRenderer
//       3. ImGui::CreateContext()                 ← 建 ImGui 上下文
//          backend->InitImGuiBackend()            ← imgui_impl_sdl2 + sdlrenderer2 初始化
//          ui.RefreshIfDisplayChanged()           ← 加载字体
//          app.OnStart(ui)                        ← 【你写】初始场景
//       4. while (!backend->ShouldQuit()) { BeginFrame → UI → EndFrame }
//       5. app.OnShutdown → 析构场景 → 关 ImGui → 关后端
//
//   所以你真正要写的只有三处：Configure() / OnStart() / 每帧的 UI。
//
// ---------------------------------------------------------------------------
// 三、BackendConfig 里真正影响窗口的字段
// ---------------------------------------------------------------------------
//   title      → SDL_CreateWindow 的标题
//   width/height → 窗口的**逻辑尺寸**（不是像素！高 DPI 下像素可能翻倍）
//   vsync      → 渲染器是否带 SDL_RENDERER_PRESENTVSYNC
//   resizable  → SDL_WINDOW_RESIZABLE
//   high_dpi   → SDL_WINDOW_ALLOW_HIGHDPI（桌面有意义；Switch 忽略）
//
//   Switch 与桌面在这里的差异只有一个：Switch 用全屏、忽略 width/height，
//   分辨率交给 libnx；所以 Configure() 里对 Switch 关掉 vsync。
//
// ---------------------------------------------------------------------------
// 四、窗口尺寸 ≠ 可绘制区域（本课时最重要的概念）
// ---------------------------------------------------------------------------
//   桌面高 DPI（Retina）下：
//       窗口 1280x720 点  →  实际可绘制 2560x1440 像素
//   UI 如果直接按像素排版，在高 DPI 上会小一半、在 4:3 窗口上会挤出屏幕。
//   本项目的做法是"**设计空间**"：
//
//       scale   = min(drawable_h / 720, drawable_w / 1280)   // 更受限的一边
//       logical = drawable / scale                            // UI 就在这个空间里排版
//       渲染缩放交给 SDL（SDL_RenderSetScale），字体光栅化密度用 FramebufferScale
//
//   于是：720p 手持 → scale 1.0（UI 就是设计值）；1080p 底座 → 1.5；
//   4:3 窗口 → 逻辑画布变高（多出的高度由布局层自适应）。
//
//   本课时的界面上会实时显示这几个数，你可以改窗口尺寸观察它们的变化。
//
// ---------------------------------------------------------------------------
// 【练习】改 src/course/CourseApp.cpp 的 Configure()，逐条观察：
//   1. vsync = false            → 数字里的"帧率"会变化（可用 GUI_DEV_NO_VSYNC=1 对照）
//   2. width/height = 960, 720  → 逻辑画布变成 1280x960，scale 变 0.75（信息区可验证）
//   3. resizable = false        → 窗口拉不动了
//   4. title 改成你的名字       → 窗口标题栏变化
// ---------------------------------------------------------------------------
// 【验收】不看代码能回答：
//   1. ImGui 负责建窗口吗？谁负责？
//   2. AppRunner::Run() 里 Configure / OnStart / 每帧 UI 的调用顺序？
//   3. 为什么 UI 代码里不该出现 SDL_ 开头的调用？
//   4. drawable 和 logical 的区别？scale 怎么算出来的？
// ============================================================================

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

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

// 小标题 + 正文：课程里反复用，所以抽出来（正式控件请参考 GameMenuButton 的写法）
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

// 代码片段：等宽观感靠灰底 + 缩进，不用额外字体（课程不引入新资源）
void Code(const char* lines) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
    const float line_count =
        1.0f + static_cast<float>(std::count(lines, lines + std::strlen(lines), '\n'));
    ImGui::BeginChild(lines, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * line_count + 8.0f),
                      ImGuiChildFlags_None, ImGuiWindowFlags_None);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.85f, 0.72f, 1.0f));
    ImGui::TextUnformatted(lines);
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void KeyValue(const char* key, const char* fmt, ...) {
    ImGui::TextUnformatted(key);
    ImGui::SameLine(230.0f);
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.93f, 0.93f, 0.96f, 1.0f));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

} // namespace

// ---------------------------------------------------------------------------
// 课时 1 的界面：把上面的讲解变成可以看、可以验证的东西
// ---------------------------------------------------------------------------
class Lesson01WindowScene final : public Scene {
public:
    const char* Name() const override { return "lesson01"; }

    // 生命周期钩子的最小用法（详见课时 2）：进入/离开各记一条到共享日志，
    // 这样切到课时 2 时能看到「课时 1 的 OnLeave」确实发生在「课时 2 的 OnEnter」之前。
    void OnEnter(UiContext& ui) override {
        (void)ui;
        LogEvent("L1 OnEnter      场景创建");
    }
    void OnLeave(UiContext& ui) override {
        (void)ui;
        LogEvent("L1 OnLeave      场景析构前");
    }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 1 · 建立窗口",
                                    "ImGui 不建窗口；窗口由平台后端提供，UI 只负责画")) {
            Components::EndPanel(ui);
            return;
        }

        // ---- 零、先看实况：本课时最值钱的部分 ------------------------------
        Section("一、当前窗口实况：窗口尺寸 ≠ 可绘制区域");
        int drawable_w = 0;
        int drawable_h = 0;
        ui.GetBackend().GetDrawableSize(drawable_w, drawable_h);
        const float scale = ui.GetBackend().UiScale();
        const ImVec2 logical = ImGui::GetIO().DisplaySize;

        KeyValue("平台 / 后端", "%s / SDL2 + SDL_Renderer2", PlatformName());
        KeyValue("drawable（真实像素）", "%d x %d", drawable_w, drawable_h);
        KeyValue("ui scale（设计→物理）", "%.3f", static_cast<double>(scale));
        KeyValue("logical（UI 排版空间）", "%.0f x %.0f", logical.x, logical.y);
        KeyValue("帧间隔 dt", "%.2f ms  (%.0f FPS)", ui.DeltaTime() * 1000.0f,
                 ui.DeltaTime() > 0.0f ? 1.0f / ui.DeltaTime() : 0.0f);
        KeyValue("手持平台（Switch）", "%s", kIsHandheld ? "是" : "否");
        Note("scale = min(drawable_h / 720, drawable_w / 1280)；logical = drawable / scale。");
        Note("把窗口拉成 960x720 再看：逻辑画布变成 1280x960（是变高，不是变窄）。");

        // ---- 二、三层结构 --------------------------------------------------
        Section("二、三层结构：谁负责什么");
        Note("ImGui 是纯绘制库：不建窗口、不处理系统事件。所以必须有平台后端。");
        Note("本项目把它抽象成 gui_dev::Backend，src/ui 里看不到任何 SDL 调用。");
        Code("你的 UI (src/ui, src/gamemenu)\n"
             "   |  每帧提交控件与绘制，不认平台\n"
             "   v\n"
             "ImGui 后端 (imgui_impl_sdl2 / _sdlrenderer2)\n"
             "   |  事件 -> io，DrawData -> 屏幕\n"
             "   v\n"
             "平台后端 (src/platform/backends/sdl2)\n"
             "       SDL_CreateWindow / SDL_CreateRenderer / 事件泵");

        // ---- 二、启动链条 --------------------------------------------------
        Section("三、启动链条：你只需要写 3 处");
        Note("main -> AppRunner(app).Run()，内部顺序如下（src/core/App.cpp）：");
        Code("1. app.Configure(cfg, kPlatform)      <-- 你写：配窗口\n"
             "2. backend->Init(cfg)                   SDL_CreateWindow / Renderer\n"
             "3. ImGui::CreateContext()\n"
             "   backend->InitImGuiBackend()          两个 imgui_impl_* 初始化\n"
             "   ui.RefreshIfDisplayChanged()          加载字体\n"
             "   app.OnStart(ui)                      <-- 你写：初始场景\n"
             "4. while (!ShouldQuit()) { BeginFrame -> UI -> EndFrame }\n"
             "5. OnShutdown -> 析构场景 -> 关 ImGui -> 关后端");

        // ---- 四、配置速查 --------------------------------------------------
        Section("四、BackendConfig：只有 6 个字段");
        ImGui::TextUnformatted("title          -> SDL_CreateWindow 标题");
        ImGui::TextUnformatted("width / height -> 窗口逻辑尺寸（不是像素）");
        ImGui::TextUnformatted("vsync          -> SDL_RENDERER_PRESENTVSYNC");
        ImGui::TextUnformatted("resizable      -> SDL_WINDOW_RESIZABLE");
        ImGui::TextUnformatted("high_dpi       -> SDL_WINDOW_ALLOW_HIGHDPI（Switch 忽略）");

        // ---- 五、练习 ------------------------------------------------------
        Section("五、练习（改 src/course/CourseApp.cpp）");
        ImGui::BulletText("把 vsync 改成 false，看信息区的 FPS 与 dt 变化");
        ImGui::BulletText("把 width/height 改成 960x720，验证 logical 变成 1280x960");
        ImGui::BulletText("把 resizable 改成 false，试着拖动窗口边缘");
        ImGui::BulletText("把 title 改成你的名字");

        // ---- 六、验收 ------------------------------------------------------
        Section("六、验收：不看代码能回答这四个问题");
        ImGui::BulletText("ImGui 负责建窗口吗？谁负责？");
        ImGui::BulletText("Configure / OnStart / 每帧 UI 的调用顺序？");
        ImGui::BulletText("为什么 UI 代码里不该出现 SDL_ 开头的调用？");
        ImGui::BulletText("drawable 与 logical 的区别，scale 怎么算的？");

        Components::EndPanel(ui, {
            {Icons::Glyph(Icons::Button::L), "上一课"},
            {Icons::Glyph(Icons::Button::R), "下一课"},
            {Icons::Glyph(Icons::Button::B), "退出"},
        });
    }
};

std::unique_ptr<Scene> CreateLesson01Window() {
    return std::make_unique<Lesson01WindowScene>();
}

} // namespace gui_dev::course
