// ============================================================================
// 课时 4 · UI 层级与布局
// ============================================================================
//
// 【目标】
//   建立"窗口 > 子区域 > 元素"三层空间感，并掌握立即模式的布局游标。
//
// ---------------------------------------------------------------------------
// 一、三层空间
// ---------------------------------------------------------------------------
//   Window（窗口）    有位置/尺寸/滚动/裁剪；Begin/End 成对
//    └ Child（子区域） 窗口内部的一块独立区域：独立滚动、独立布局、独立裁剪
//       └ Item（元素）  每个 Text/Button/你的自绘控件都是 item，占一块矩形
//
//   本项目的页面骨架就是这么摞的（src/ui/Components.cpp 的 BeginPanel）：
//       根画布窗口（铺满屏幕、无装饰）
//        ├ 页头 Child（固定高 66）
//        ├ 内容 Child（负高度 = 填满剩余，自动滚动）
//        └ 页脚 Child（绝对定位到底部）
//
// ---------------------------------------------------------------------------
// 二、布局游标：立即模式的排版核心
// ---------------------------------------------------------------------------
//   每个窗口/子区域内部有一个"游标"（CursorPos），item 提交后游标会自动前进：
//       提交 item  ->  游标右移/下移到该 item 之后（含 ItemSpacing）
//   你手动干预的手段只有几个：
//       SameLine()               不换行，继续往右排
//       Spacing() / Dummy()      让游标前进一段（Dummy 还能占位参与命中测试）
//       SetCursorPos / SetCursorScreenPos  直接把游标挪到指定位置
//       NewLine()                换到下一行
//       Indent/Unindent          整体缩进
//
//   两个高频坑：
//     · SetCursorPos 之后**必须再提交一个 item**（比如 Dummy(0,0)），
//       否则 imgui 1.92 会断言 ErrorCheckUsingSetCursorPosToExtendParentBoundaries；
//     · 只 SetCursorPos 然后自绘、不提交 item，布局不会前进 —— 后面的控件会叠上来。
//
// ---------------------------------------------------------------------------
// 三、常用布局 API
// ---------------------------------------------------------------------------
//   GetCursorScreenPos()      取当前游标的屏幕坐标（自绘控件必用）
//   GetContentRegionAvail()   当前区域内还剩多少空间（响应式布局的基础）
//   GetItemRectMin/Max()      刚提交的 item 的矩形（自绘/命中测试用）
//   SetNextItemWidth()        控制下一个 item 的宽度
//   BeginTable/EndTable       表格布局（列对齐比手算坐标省心）
//   PushItemWidth / PushStyleVar(ItemSpacing, WindowPadding …)  临时改布局参数
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 把本课时"SameLine 演示"里的 SameLine() 注释掉，看三个方块如何换行。
//   2. 把 Dummy 大小从 (0,20) 改成 (40,20)，观察它如何把游标推得更远。
//   3. 在"嵌套子区域"那段里，把 BeginChild 的高度改成 0（默认填满），
//      再体会"子区域自己滚动"是什么意思。
// ---------------------------------------------------------------------------
// 【验收】
//   1. Window / Child / Item 的区别？各自什么时候用？
//   2. 提交一个 item 后游标怎么变？SameLine 做了什么？
//   3. 为什么 SetCursorPos 之后必须补一个 item？
//   4. 自己画出 BeginPanel 的三段结构并说明每段的高度怎么决定。
// ============================================================================

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/Theme.h"
#include "ui/UiContext.h"

namespace gui_dev::course {

class Lesson04HierarchyScene final : public Scene {
public:
    const char* Name() const override { return "lesson04"; }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 4 · UI 层级与布局",
                                    "Window > Child > Item，以及布局游标怎么动")) {
            Components::EndPanel(ui);
            return;
        }

        // ---- 一、三层空间 --------------------------------------------------
        Section("一、三层空间：Window > Child > Item");
        Code("Window  根画布：铺满屏幕、无装饰（Begin/End 成对）\n"
             " └ Child 页头/内容/页脚：各自独立布局、滚动、裁剪\n"
             "    └ Item 每个 Text / 自绘控件：占一块矩形，参与命中测试");

        // ---- 二、本帧的几何实况 --------------------------------------------
        Section("二、本帧几何实况（都是可以随时取的）");
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 window_pos = ImGui::GetWindowPos();
        const ImVec2 window_size = ImGui::GetWindowSize();
        KeyValue("窗口 pos / size", "(%.0f, %.0f) / %.0f x %.0f", window_pos.x, window_pos.y,
                 window_size.x, window_size.y);
        KeyValue("游标 screen pos", "(%.0f, %.0f)", cursor.x, cursor.y);
        KeyValue("内容区剩余", "%.0f x %.0f", avail.x, avail.y);
        Note("GetContentRegionAvail() 是响应式布局的基础：窄了就换布局，而不是写死尺寸。");

        // 把游标位置画出来（只画，不提交 item，所以不影响布局）
        ImGui::GetWindowDrawList()->AddCircleFilled(ImGui::GetCursorScreenPos(), 4.0f,
                                                    IM_COL32(0xE2, 0x1B, 0x25, 0xFF));
        ImGui::SameLine();
        Note("  <- 红点就是当前游标位置");

        // ---- 三、游标操作演示 ----------------------------------------------
        Section("三、游标操作：SameLine / Dummy / 换行");
        ImGui::Checkbox("启用 SameLine", &same_line_);
        ImGui::SameLine();
        ImGui::Checkbox("插入 Dummy(0,20)", &use_dummy_);
        ImGui::Spacing();
        for (int i = 0; i < 3; ++i) {
            ImGui::Button("方块"); // 用内建按钮只为演示排版，正式视觉要自绘（课时 5）
            if (same_line_) {
                ImGui::SameLine();
            }
        }
        if (use_dummy_) {
            ImGui::Dummy(ImVec2(0.0f, 20.0f)); // 推进游标，同时也是一个可命中的 item
        }
        Note("SameLine 关掉后三个方块会竖排；Dummy 把游标往下推 20px。");

        // ---- 四、嵌套子区域 ------------------------------------------------
        Section("四、嵌套子区域：独立滚动与裁剪");
        Note("下面这个子区域高度固定 90px，里面的内容超过高度时它自己滚动（滚轮试一下）。");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.08f, 0.10f, 1.0f));
        ImGui::BeginChild("##nested", ImVec2(0.0f, 90.0f), ImGuiChildFlags_None,
                          ImGuiWindowFlags_None);
        for (int i = 0; i < 8; ++i) {
            ImGui::Text("子区域第 %d 行 —— 布局游标在子区域内部，与外面互不干扰", i + 1);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        const ImVec2 nested_avail = ImGui::GetContentRegionAvail();
        KeyValue("回到外面后的剩余空间", "%.0f x %.0f", nested_avail.x, nested_avail.y);
        Note("子区域结束后游标回到外层，继续往下排 —— 这就是「栈式」的空间嵌套。");

        // ---- 五、表格布局 --------------------------------------------------
        Section("五、表格：列对齐不用手算坐标");
        if (ImGui::BeginTable("##demo_table", 3,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame |
                                  ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("项目");
            ImGui::TableSetupColumn("值");
            ImGui::TableSetupColumn("说明");
            ImGui::TableHeadersRow();
            for (int row = 0; path_fixed_layouts()[row].name != nullptr; ++row) {
                const FixedLayoutRow& item = path_fixed_layouts()[row];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(item.name);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(item.value);
                ImGui::TableNextColumn();
                ImGui::TextDisabled("%s", item.note);
            }
            ImGui::EndTable();
        }

        Section("六、练习与验收（完整版见本文件头部）");
        Bullet("练习：注释掉 SameLine 看换行；改 Dummy 尺寸看推进距离");
        Bullet("验收：Window/Child/Item 区别？SetCursorPos 后为什么必须补 item？");

        EndLesson(ui);
    }

private:
    struct FixedLayoutRow {
        const char* name;
        const char* value;
        const char* note;
    };
    static const FixedLayoutRow* path_fixed_layouts() {
        static const FixedLayoutRow rows[] = {
            {"页头高度", "66（Theme::kHeaderHeight）", "固定值，主题集中管理"},
            {"内容区高度", "-Gap - FooterHeight", "负高度 = 填满剩余空间"},
            {"页脚位置", "画布高 - 50", "绝对定位，保证贴底"},
            {nullptr, nullptr, nullptr},
        };
        return rows;
    }

    bool same_line_ = true;
    bool use_dummy_ = false;
};

std::unique_ptr<Scene> CreateLesson04Hierarchy() { return std::make_unique<Lesson04HierarchyScene>(); }

} // namespace gui_dev::course
