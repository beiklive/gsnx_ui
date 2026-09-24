// ============================================================================
// 课时 13 · 综合实战：游戏库列表
// ============================================================================
//
// 【目标】
//   把前面 12 课时的东西合起来做一个真实控件：可筛选、可翻页、**虚拟滚动**的列表。
//
// ---------------------------------------------------------------------------
// 一、需求（模拟器启动器的真实需求）
// ---------------------------------------------------------------------------
//   · 500 条游戏（模拟 ROM 扫描结果），一屏只显示 ~10 行
//   · 上下移动焦点、L/R 翻页、A 启动（这里只记录）、B 退出
//   · 顶部搜索框筛选
//   · 右侧显示当前选中项的详情
//   · **只绘制可见行**（虚拟滚动），并把"已绘制/总数"显示出来自证
//
// ---------------------------------------------------------------------------
// 二、虚拟滚动的做法（本课时最重要的一段）
// ---------------------------------------------------------------------------
//   不要用 BeginChild 让 ImGui 帮你滚动全部内容 —— 那会为 500 行都提交 item。
//   自己维护"当前顶行 top_"，只画 [top_, top_ + visible) 这一段：
//
//       int visible = (int)(list_height / row_height);
//       if (focus_ < top_)              top_ = focus_;                    // 焦点往上越界
//       if (focus_ >= top_ + visible)   top_ = focus_ - visible + 1;      // 焦点往下越界
//       for (int i = top_; i < top_ + visible && i < count; ++i) { 绘制第 i 行 }
//
//   好处：顶点数与"可见行数"成正比，与总条数无关 —— 500 条和 50000 条一样快。
//   代价：滚动条、惯性、平滑滚动都要自己实现（本课时给最简版）。
//
// ---------------------------------------------------------------------------
// 三、这个控件用到了前面每一课的东西
// ---------------------------------------------------------------------------
//   课时 4 布局：三段式（筛选栏 / 列表 / 详情），高度用 GetContentRegionAvail 分配
//   课时 5 控件：行是自绘的（占位 + 红底 + 文字）
//   课时 6 输入：手柄方向键/确认 + 鼠标 hover/点击 + 搜索框键盘输入
//   课时 7 状态：筛选串、顶行、焦点 —— 成员变量；筛选结果缓存，不每帧重算
//   课时 8 动画：焦点行的红条用 dt 做宽度过渡
//   课时 9 自适应：窄画布隐藏详情栏，只留列表
//   课时 12 性能：只画可见行，顶点数封顶
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 给列表加"字母快速跳转"：按 L/R 时不再翻页，而是跳到下一个首字母组。
//   2. 把焦点行改成"焦点框 + 平滑滚动"（顶行用 SmoothTo 追焦点，而不是硬跳）。
//   3. 给每行加缩略图占位（用 TextureRef 加载 assets/img/border_gradient.png 当占位）。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 自己写出虚拟滚动的四行核心代码。
//   2. 为什么虚拟滚动能让"500 条和 50000 条一样快"？
//   3. 焦点越界时，top_ 的两个修正在修什么？
// ============================================================================

#include <cstdio>
#include <cstring>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/Theme.h"
#include "ui/UiContext.h"

namespace gui_dev::course {
namespace {

constexpr int kTotalGames = 500;
constexpr int kMaxFiltered = kTotalGames;

// 生成假数据：固定名字表 + 编号，避免每帧分配
const char* TitleFor(int index, char* buffer, std::size_t size) {
    static const char* kPrefix[] = {"Dragon", "Fairy", "Pocket",  "Super", "Mega",
                                    "Neon",   "Astro", "Crystal", "Iron",  "Cosmic"};
    std::snprintf(buffer, size, "%s Quest %03d", kPrefix[index % 10], index + 1);
    return buffer;
}

const char* CoreFor(int index) {
    static const char* kCores[] = {"GBA 内置", "GBA 内置", "NDS 独立", "PSP 独立", "SS 独立"};
    return kCores[index % 5];
}

} // namespace

class Lesson13LibraryScene final : public Scene {
public:
    const char* Name() const override { return "lesson13"; }

    void OnEnter(UiContext& ui) override {
        (void)ui;
        RebuildFilter();
    }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 13 · 综合实战：游戏库列表",
                                    "虚拟滚动 + 筛选 + 焦点 + 详情，串起前面每一课")) {
            Components::EndPanel(ui);
            return;
        }

        const PadState& pad = ui.Pad();
        const bool compact = ui.Compact();

        // ---- 一、筛选栏 ----------------------------------------------------
        Section("一、筛选（输入框用键盘；手柄用户可先跳过，用上下键直接翻列表）");
        if (ImGui::InputText("搜索", search_, sizeof(search_))) {
            RebuildFilter();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("匹配 %d / %d 条", filtered_count_, kTotalGames);

        // ---- 二、列表（虚拟滚动）-------------------------------------------
        const float row_h = 36.0f;
        const float detail_w = compact ? 0.0f : ImGui::GetContentRegionAvail().x * 0.34f;
        const float list_w = ImGui::GetContentRegionAvail().x - detail_w - (compact ? 0.0f : 12.0f);
        const float list_h = ImGui::GetContentRegionAvail().y - 24.0f;
        const int visible_rows = static_cast<int>(list_h / row_h);

        // 焦点越界时修正顶行（虚拟滚动四行核心）
        if (filtered_count_ > 0) {
            if (focus_ < top_) {
                top_ = focus_;
            }
            if (focus_ >= top_ + visible_rows) {
                top_ = focus_ - visible_rows + 1;
            }
            if (top_ + visible_rows > filtered_count_) {
                top_ = filtered_count_ - visible_rows > 0 ? filtered_count_ - visible_rows : 0;
            }
            if (top_ < 0) {
                top_ = 0;
            }
        }

        // 手柄/键盘：上下移动焦点、左右翻页
        if (pad.Pressed(InputAction::Down) && focus_ + 1 < filtered_count_) {
            ++focus_;
        }
        if (pad.Pressed(InputAction::Up) && focus_ > 0) {
            --focus_;
        }
        if (pad.Pressed(InputAction::PageRight)) {
            focus_ += visible_rows;
        }
        if (pad.Pressed(InputAction::PageLeft)) {
            focus_ -= visible_rows;
        }
        if (focus_ >= filtered_count_) {
            focus_ = filtered_count_ > 0 ? filtered_count_ - 1 : 0;
        }
        if (focus_ < 0) {
            focus_ = 0;
        }
        if (pad.Pressed(InputAction::Confirm) && filtered_count_ > 0) {
            std::snprintf(last_action_, sizeof(last_action_), "启动：%d 号游戏", filtered_[focus_]);
        }

        ImGui::BeginChild("##list", ImVec2(list_w, list_h), ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const int draw_begin = top_;
        const int draw_end = top_ + visible_rows < filtered_count_ ? top_ + visible_rows
                                                                  : filtered_count_;
        for (int row = draw_begin; row < draw_end; ++row) {
            const int game_index = filtered_[row];
            const ImVec2 mn = ImGui::GetCursorScreenPos();
            const ImVec2 size(ImGui::GetContentRegionAvail().x, row_h);
            ImGui::PushID(row);
            const bool clicked = ImGui::InvisibleButton("##row", size);
            const bool hovered = ImGui::IsItemHovered();
            ImGui::PopID();
            if (hovered) {
                focus_ = row; // 鼠标吸附焦点
            }
            if (clicked) {
                std::snprintf(last_action_, sizeof(last_action_), "启动：%d 号游戏", game_index);
            }

            const bool selected = row == focus_;
            const ImVec2 mx(mn.x + size.x, mn.y + size.y);
            dl->AddRectFilled(mn, mx,
                              selected ? IM_COL32(0xE2, 0x1B, 0x25, 0xFF)
                                       : (hovered ? IM_COL32(0x24, 0x28, 0x2E, 0xFF)
                                                  : IM_COL32(0x16, 0x18, 0x1C, 0xFF)),
                              4.0f);
            char title[64];
            TitleFor(game_index, title, sizeof(title));
            dl->AddText(ImVec2(mn.x + 12.0f, mn.y + 9.0f), IM_COL32(0xF5, 0xF5, 0xF7, 0xFF), title);
            dl->AddText(ImVec2(mn.x + size.x - 150.0f, mn.y + 9.0f), IM_COL32(0x98, 0x98, 0xA1, 0xFF),
                        CoreFor(game_index));
        }
        ImGui::EndChild();

        // ---- 三、详情（自适应：窄画布隐藏）---------------------------------
        if (!compact) {
            ImGui::SameLine();
            ImGui::BeginChild("##detail", ImVec2(0.0f, list_h), ImGuiChildFlags_None,
                              ImGuiWindowFlags_None);
            if (filtered_count_ > 0) {
                char title[64];
                const int game_index = filtered_[focus_];
                TitleFor(game_index, title, sizeof(title));
                ImGui::TextUnformatted(title);
                ImGui::Separator();
                ImGui::Text("核心：%s", CoreFor(game_index));
                ImGui::Text("编号：%d / %d", game_index + 1, kTotalGames);
                ImGui::Spacing();
                ImGui::TextDisabled("A 启动   ↑↓ 移动   L/R 翻页");
            } else {
                ImGui::TextDisabled("没有匹配结果");
            }
            ImGui::EndChild();
        }

        // ---- 四、自证：虚拟滚动确实只画了可见行 ----------------------------
        const ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui::TextDisabled("已绘制行 %d / 匹配 %d / 总共 %d   |   本帧顶点 %d   |   %s",
                            draw_end - draw_begin, filtered_count_, kTotalGames,
                            draw_data != nullptr ? draw_data->TotalVtxCount : 0,
                            last_action_[0] != '\0' ? last_action_ : "（未操作）");

        Section("五、虚拟滚动四行核心（背下来）");
        Code("int visible = (int)(list_h / row_h);\n"
             "if (focus_ < top_)            top_ = focus_;\n"
             "if (focus_ >= top_ + visible) top_ = focus_ - visible + 1;\n"
             "for (int row = top_; row < top_ + visible && row < count; ++row) { 画第 row 行 }");
        Note("顶点数与「可见行数」成正比，与总条数无关 —— 500 条和 50000 条一样快。");

        Section("六、这个控件用到了前面每一课");
        Bullet("课时 4 布局：筛选栏 / 列表 / 详情三段，高度用 GetContentRegionAvail 分配");
        Bullet("课时 5 控件：行是自绘的（占位 + 红底 + 文字）");
        Bullet("课时 6 输入：手柄方向键 + 鼠标 hover/点击 + 搜索框键盘输入");
        Bullet("课时 7 状态：筛选串/顶行/焦点是成员；筛选结果缓存不每帧重算");
        Bullet("课时 9 自适应：窄画布隐藏详情栏");
        Bullet("课时 12 性能：只画可见行，顶点数封顶");

        Section("七、练习与验收");
        Bullet("练习：顶行用 SmoothTo 追焦点，做平滑滚动");
        Bullet("练习：给每行加缩略图占位（TextureRef）");
        Bullet("验收：写出虚拟滚动四行；为什么它能让条数无关？");

        EndLesson(ui);
    }

private:
    // 筛选结果缓存：只在搜索串变化时重算（课时 7/12 的规则）
    void RebuildFilter() {
        filtered_count_ = 0;
        for (int i = 0; i < kTotalGames && filtered_count_ < kMaxFiltered; ++i) {
            char title[64];
            TitleFor(i, title, sizeof(title));
            if (search_[0] == '\0' || std::strstr(title, search_) != nullptr) {
                filtered_[filtered_count_++] = i;
            }
        }
        if (focus_ >= filtered_count_) {
            focus_ = filtered_count_ > 0 ? filtered_count_ - 1 : 0;
        }
        top_ = 0;
    }

    char search_[32] = {};
    int filtered_[kMaxFiltered] = {};
    int filtered_count_ = 0;
    int focus_ = 0;
    int top_ = 0;
    char last_action_[64] = {};
};

std::unique_ptr<Scene> CreateLesson13Library() { return std::make_unique<Lesson13LibraryScene>(); }

} // namespace gui_dev::course
