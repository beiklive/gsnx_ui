// ============================================================================
// 课时 12 · 性能与质量
// ============================================================================
//
// 【目标】
//   知道 UI 的开销花在哪、怎么量、怎么保证"不卡"。
//
// ---------------------------------------------------------------------------
// 一、先量，再优化：三个可直接读到的指标
// ---------------------------------------------------------------------------
//   ImGui::GetDrawData()->TotalVtxCount / TotalIdxCount / CmdListsCount
//       —— 这一帧 UI 提交了多少几何；经验值：720p 信息页 1~3k 顶点。
//   io.DeltaTime / io.Framerate  —— 帧间隔与平滑 FPS。
//   自己的计时（std::chrono）      —— 定位是 UI 慢还是后端慢。
//   本课时的"压力测试"滑杆会画出 N 行，让你看着顶点数线性增长：
//   心里要有一条线 —— 顶点数是**可预测**的（每行固定几十个顶点），
//   所以"卡"通常是两种情况：提交了过多行（该做虚拟滚动，见课时 13），
//   或者每帧分配/字符串拼接把 CPU 拖垮。
//
// ---------------------------------------------------------------------------
// 二、四条硬规则（本项目所有 UI 代码都守）
// ---------------------------------------------------------------------------
//   R1 不每帧堆分配
//        错：std::string s = "第" + std::to_string(i) + "行";
//        对：char buf[32]; snprintf(buf, sizeof(buf), "第 %d 行", i);
//        本项目连日志都是固定 char[] 环形缓冲（CourseLog）。
//   R2 不在绘制里做重活
//        文件扫描、图片解码、存档读写都不许放在 OnRender（放到异步/下一帧）。
//   R3 顶点数可预算
//        列表用虚拟滚动，只画可见行；避免"画了 5000 行只为看不见"。
//   R4 不要依赖 vsync 锁帧
//        本项目实测：显示器 60Hz，但轻负载下实际跑 ~120 FPS（PRESENTVSYNC 没锁住）。
//        要严格锁帧得自己按目标帧率限时。动画本身吃 dt，所以帧率波动不影响观感。
//
// ---------------------------------------------------------------------------
// 三、怎么写"能验证"的 UI 代码
// ---------------------------------------------------------------------------
//   本项目一路用的三种验证手段，建议照搬：
//     1) 冒烟退出钩子：GUI_DEV_EXIT_AFTER=<帧数>，让程序**正常退出**，
//        这样才能验证析构路径（用 timeout 杀进程是走不到析构的 —— 我们踩过）。
//     2) 抓帧比对：临时在 EndRenderFrame 里 SDL_RenderReadPixels 存 BMP，
//        改完 UI 抓一张图核对（本项目所有视觉改动都这么验）。
//     3) 自检日志：字形覆盖率、纹理加载、缺字体等都打到 stderr，
//        一眼能看出"资源没加载"而不是靠肉眼猜。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 把压力测试拉到 2000 行，记录顶点数；再想想如果只画可见行能省多少。
//   2. 在本课时里加一个"每帧拼 std::string"的开关（画同样内容但用 string），
//      用 std::chrono 量 OnRender 耗时，对比两种写法的差距。
//   3. 给课程 Demo 加一条 stderr 自检（例如打印当前画布与缩放），再跑一次看输出。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 三个性能指标分别怎么读？
//   2. 四条规则的每一条对应的具体做法？
//   3. 为什么"用 timeout 杀进程"验证不了退出路径？
// ============================================================================

#include <chrono>
#include <cstdio>
#include <string>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {

class Lesson12PerfScene final : public Scene {
public:
    const char* Name() const override { return "lesson12"; }

    void OnRender(UiContext& ui) override {
        const auto render_begin = std::chrono::steady_clock::now();

        if (!Components::BeginPanel(ui, "课时 12 · 性能与质量",
                                    "先量再优化：顶点数 / 帧时间 / 分配")) {
            Components::EndPanel(ui);
            return;
        }

        const ImGuiIO& io = ImGui::GetIO();
        const ImDrawData* draw_data = ImGui::GetDrawData();

        Section("一、量：本帧指标");
        KeyValue("顶点 / 索引", "%d / %d", draw_data != nullptr ? draw_data->TotalVtxCount : 0,
                 draw_data != nullptr ? draw_data->TotalIdxCount : 0);
        KeyValue("命令列表数", "%d", draw_data != nullptr ? draw_data->CmdListsCount : 0);
        KeyValue("帧间隔 / FPS", "%.2f ms / %.1f", static_cast<double>(io.DeltaTime * 1000.0f),
                 static_cast<double>(io.Framerate));
        KeyValue("本课时 OnRender 耗时", "%.3f ms", static_cast<double>(last_render_ms_));
        KeyValue("上帧总顶点（含面板）", "%d", last_frame_vertices_);

        Section("二、压力测试：顶点数随绘制行数线性增长");
        ImGui::SliderInt("额外绘制行数", &stress_rows_, 0, 2000, "%d 行");
        Note("每行约 20~30 个顶点，2000 行就是几万顶点 —— 这就是为什么要虚拟滚动。");
        if (stress_rows_ > 0) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float y = ImGui::GetCursorScreenPos().y;
            for (int i = 0; i < stress_rows_; ++i) {
                const ImVec2 mn(ImGui::GetCursorScreenPos().x, y + static_cast<float>(i) * 2.0f);
                dl->AddRectFilled(mn, ImVec2(mn.x + 200.0f, mn.y + 1.0f),
                                  IM_COL32(0x4F, 0xA3, 0xFF, 0x30));
            }
            ImGui::Dummy(ImVec2(200.0f, static_cast<float>(stress_rows_) * 2.0f));
        }

        Section("三、四条硬规则");
        Code("R1 不每帧堆分配\n"
             "   错：std::string s = \"第\" + std::to_string(i) + \"行\";\n"
             "   对：char buf[32]; snprintf(buf, sizeof(buf), \"第 %d 行\", i);\n"
             "R2 不在绘制里做重活（扫描/解码/读写都不许放 OnRender）\n"
             "R3 顶点数可预算（列表用虚拟滚动，只画可见行 —— 课时 13）\n"
             "R4 不要依赖 vsync 锁帧（本项目实测 60Hz 屏上跑 120 FPS）");

        Section("四、实测数据（本项目 mac 端）");
        KeyValue("320x180 窗口", "120.6 FPS / 8.29 ms");
        KeyValue("1280x720 窗口", "120.3 FPS / 8.32 ms");
        KeyValue("2560x1440 窗口", "60.0 FPS / 16.67 ms");
        KeyValue("关掉 vsync", "120.6 FPS（无差别）");
        Note("结论：主循环没有帧率上限，轻负载每刷新呈现两帧；负载重了自然回落到 60。");

        Section("五、怎么写能验证的 UI 代码");
        Bullet("冒烟退出：GUI_DEV_EXIT_AFTER=<帧数>（timeout 杀进程走不到析构）");
        Bullet("抓帧比对：临时 SDL_RenderReadPixels 存图，改完视觉抓一张核对");
        Bullet("自检日志：字形覆盖、纹理加载、缺失字体都打到 stderr");

        Section("六、练习与验收");
        Bullet("练习：把滑杆拉到 2000 行记录顶点数；再想只画可见行能省多少");
        Bullet("验收：三个指标怎么读？为什么 timeout 杀进程验证不了退出路径？");

        EndLesson(ui);

        const auto render_end = std::chrono::steady_clock::now();
        last_render_ms_ = std::chrono::duration<float, std::milli>(render_end - render_begin).count();
        last_frame_vertices_ = draw_data != nullptr ? draw_data->TotalVtxCount : 0;
    }

private:
    int stress_rows_ = 0;
    float last_render_ms_ = 0.0f;
    int last_frame_vertices_ = 0;
};

std::unique_ptr<Scene> CreateLesson12Perf() { return std::make_unique<Lesson12PerfScene>(); }

} // namespace gui_dev::course
