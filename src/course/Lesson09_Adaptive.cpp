// ============================================================================
// 课时 9 · 布局自适应
// ============================================================================
//
// 【目标】
//   一套 UI 代码同时适配 720p 手持 / 1080p 底座 / 非 16:9 窗口，且不难看。
//
// ---------------------------------------------------------------------------
// 一、两层问题，分别解决
// ---------------------------------------------------------------------------
//   ① 大小：屏幕像素数不同 -> **等比缩放**（见课时 1 的 scale = min(h/720, w/1280)）
//      做法：UI 全部按 1280x720 的设计空间写，后端把几何与字号一起放大。
//   ② 比例：画布不是 16:9（桌面窗口可任意拉）-> **布局自适应**
//      做法：面板宽高、栅格列数、垂直居中这些"结构"不能写死，要按可用空间算。
//
//   只做①不做②的后果：4:3 窗口下菜单占 65% 宽、槽位卡被挤扁。
//   只做②不做①的后果：1080p 上字变大而面板不变，文字溢出（本项目早期真踩过）。
//
// ---------------------------------------------------------------------------
// 二、本项目怎么做的（src/gamemenu/GameMenuTheme.cpp 的 ResolveGameMenuLayout）
// ---------------------------------------------------------------------------
//   输入：主题（基准值）+ 当前画布矩形
//   输出：GameMenuLayout { panel_width, panel_height, panel_x, panel_y,
//                          slot_columns, slot_rows, wide }
//   规则（全部是"钳制 + 比例"，不是硬编码）：
//     · 面板宽 = 画布宽 * 0.41，夹在 [440, 640]，且给左侧游戏画面至少留 52%
//     · 面板高 = min(可用高度, 700)；多余空间上下均分（高画布垂直居中）
//     · 槽位栅格：内容区宽 > 540 用 3 列 2 行，否则 2 列 3 行
//     · 上下键步长跟着列数走（否则焦点会跳错行）
//
// ---------------------------------------------------------------------------
// 三、通用做法（不限于本项目的菜单）
// ---------------------------------------------------------------------------
//   · 先问"这块区域有多少空间"：GetContentRegionAvail()
//   · 再决定结构：Compact() ? 单列 : 双列（本项目 Components::Compact 用宽度阈值）
//   · 尺寸用比例 + 上下限，不要用绝对值：min(max(预算*比例, 下限), 上限)
//   · 只在"画布尺寸变化"时重算布局（本项目的 DisplayGeneration()），不要每帧算
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 把面板宽的 0.41 改成 0.6，看 1280x720 与 960x720 下的差别（一个难看、一个更难）。
//   2. 把 panel_max_height 从 700 改成 2000，看高画布下面板被拉成什么样。
//   3. 新增一个预设分辨率（例如 1024x600），观察槽位列数的切换点。
// ---------------------------------------------------------------------------
// 【验收】
//   1. "缩放"与"自适应"各解决什么问题？
//   2. 为什么面板宽要给游戏画面留一半？
//   3. 为什么布局只在画布变化时重算，而不是每帧算？
// ============================================================================

#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "gamemenu/GameMenuTheme.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {
namespace {

struct ResolutionPreset {
    const char* name;
    float width;
    float height;
};

constexpr ResolutionPreset kPresets[] = {
    {"Switch 手持 1280x720", 1280.0f, 720.0f},
    {"Switch 底座 1920x1080", 1920.0f, 1080.0f},
    {"4:3 窗口 960x720", 960.0f, 720.0f},
    {"宽屏 1600x720", 1600.0f, 720.0f},
    {"小窗 640x480", 640.0f, 480.0f},
};

} // namespace

class Lesson09AdaptiveScene final : public Scene {
public:
    const char* Name() const override { return "lesson09"; }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 9 · 布局自适应",
                                    "缩放解决大小，布局解决比例")) {
            Components::EndPanel(ui);
            return;
        }

        Section("一、两层问题");
        Code("① 大小不同 -> 等比缩放：UI 按 1280x720 设计，后端整体放大\n"
             "     scale = min(drawable_h / 720, drawable_w / 1280)\n"
             "② 比例不同 -> 布局自适应：面板宽高/栅格列数/居中，按可用空间算\n"
             "     只做① -> 4:3 下菜单占 65% 宽；只做② -> 1080p 上文字溢出");

        // ---- 二、本帧真实画布 ----------------------------------------------
        int drawable_w = 0;
        int drawable_h = 0;
        ui.GetBackend().GetDrawableSize(drawable_w, drawable_h);
        Section("二、当前真实画布");
        KeyValue("drawable / scale", "%d x %d / %.3f", drawable_w, drawable_h,
                 static_cast<double>(ui.GetBackend().UiScale()));
        KeyValue("逻辑画布", "%.0f x %.0f", ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

        // ---- 三、预设分辨率下的解析结果 ------------------------------------
        Section("三、同一套布局规则，在不同画布下的解析结果");
        Note("下面调用的是生产代码 ResolveGameMenuLayout()，不是演示用的假数据。");
        if (ImGui::BeginTable("##layout_table", 6,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("画布");
            ImGui::TableSetupColumn("面板宽");
            ImGui::TableSetupColumn("面板高");
            ImGui::TableSetupColumn("面板位置");
            ImGui::TableSetupColumn("槽位栅格");
            ImGui::TableSetupColumn("宽画布");
            ImGui::TableHeadersRow();
            for (const ResolutionPreset& preset : kPresets) {
                const gamemenu::Rect screen =
                    gamemenu::MakeRect(0.0f, 0.0f, preset.width, preset.height);
                const gamemenu::GameMenuLayout layout =
                    gamemenu::ResolveGameMenuLayout(gamemenu::DefaultGameMenuTheme(), screen);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(preset.name);
                ImGui::TableNextColumn();
                ImGui::Text("%.0f", static_cast<double>(layout.panel_width));
                ImGui::TableNextColumn();
                ImGui::Text("%.0f", static_cast<double>(layout.panel_height));
                ImGui::TableNextColumn();
                ImGui::Text("x=%.0f y=%.0f", static_cast<double>(layout.panel_x),
                            static_cast<double>(layout.panel_y));
                ImGui::TableNextColumn();
                ImGui::Text("%d x %d", layout.slot_columns, layout.slot_rows);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(layout.wide ? "是（3 列）" : "否（2 列）");
            }
            ImGui::EndTable();
        }

        // ---- 四、缩略图：把解析结果画出来 ----------------------------------
        Section("四、缩略图（按比例画出画布与面板位置）");
        const float thumb_w = (ImGui::GetContentRegionAvail().x - 40.0f) / 3.0f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (int i = 0; i < static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0])); ++i) {
            const ResolutionPreset& preset = kPresets[i];
            const float aspect = preset.height / preset.width;
            const ImVec2 mn = ImGui::GetCursorScreenPos();
            const ImVec2 size(thumb_w, thumb_w * aspect);
            ImGui::Dummy(size);
            if ((i + 1) % 3 != 0) {
                ImGui::SameLine();
            }
            const ImVec2 mx(mn.x + size.x, mn.y + size.y);
            dl->AddRectFilled(mn, mx, IM_COL32(0x10, 0x12, 0x16, 0xFF), 4.0f);
            dl->AddRect(mn, mx, IM_COL32(0x44, 0x4A, 0x54, 0xFF), 4.0f, 0, 1.0f);

            const gamemenu::GameMenuLayout layout = gamemenu::ResolveGameMenuLayout(
                gamemenu::DefaultGameMenuTheme(),
                gamemenu::MakeRect(0.0f, 0.0f, preset.width, preset.height));
            const float sx = size.x / preset.width;
            const float sy = size.y / preset.height;
            const ImVec2 panel_mn(mn.x + layout.panel_x * sx, mn.y + layout.panel_y * sy);
            const ImVec2 panel_mx(panel_mn.x + layout.panel_width * sx,
                                  panel_mn.y + layout.panel_height * sy);
            dl->AddRectFilled(panel_mn, panel_mx, IM_COL32(0xE2, 0x1B, 0x25, 0xC0), 2.0f);
            // 槽位栅格示意
            const int cols = layout.slot_columns;
            const int rows = layout.slot_rows;
            const float cell_w = (panel_mx.x - panel_mn.x) / static_cast<float>(cols);
            const float cell_h = (panel_mx.y - panel_mn.y) / static_cast<float>(rows + 1);
            for (int c = 0; c < cols; ++c) {
                for (int r = 0; r < rows; ++r) {
                    const ImVec2 c_mn(panel_mn.x + cell_w * static_cast<float>(c) + 2.0f,
                                      panel_mn.y + cell_h * static_cast<float>(r + 1) + 2.0f);
                    const ImVec2 c_mx(c_mn.x + cell_w - 4.0f, c_mn.y + cell_h - 4.0f);
                    dl->AddRect(c_mn, c_mx, IM_COL32(0xFF, 0xFF, 0xFF, 0x50), 2.0f, 0, 1.0f);
                }
            }
            dl->AddText(ImVec2(mn.x + 6.0f, mn.y + 4.0f), IM_COL32(0xF5, 0xF5, 0xF7, 0xFF),
                        preset.name);
        }

        Section("五、通用做法");
        Bullet("先问空间：GetContentRegionAvail()，再决定结构（单列/双列）");
        Bullet("尺寸用比例 + 上下限：min(max(预算*比例, 下限), 上限)，不要写绝对值");
        Bullet("只在画布变化时重算（DisplayGeneration），不要每帧算");
        Bullet("焦点步长跟着列数走，否则上下键会跳错行");

        Section("六、练习与验收");
        Bullet("练习：把 0.41 改成 0.6、把最大高度改成 2000，观察劣化");
        Bullet("验收：缩放与自适应各解决什么？为什么面板宽要给游戏留一半？");

        EndLesson(ui);
    }
};

std::unique_ptr<Scene> CreateLesson09Adaptive() { return std::make_unique<Lesson09AdaptiveScene>(); }

} // namespace gui_dev::course
