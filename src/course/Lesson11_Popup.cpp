// ============================================================================
// 课时 11 · 弹层与拖放
// ============================================================================
//
// 【目标】
//   会做模态确认框、右键菜单、拖拽排序 —— 交互复杂但 ImGui 已经给了骨架。
//
// ---------------------------------------------------------------------------
// 一、弹层的三种形态
// ---------------------------------------------------------------------------
//   ① 模态对话框   BeginPopupModal：挡住下层交互，必须显式关闭
//                    典型用法：if (open_requested) ImGui::OpenPopup("id");
//                              if (ImGui::BeginPopupModal("id", ...)) { ...; EndPopup(); }
//   ② 普通弹窗     BeginPopup / BeginPopupContextItem：点外部自动关
//   ③ 悬浮提示     SetItemTooltip / BeginTooltip：只读展示
//
//   注意：弹层本质是**另一个窗口**，会绘制在所有普通窗口之上；
//        所以不要试图在弹层里复用"当前窗口"的绘制列表假设。
//        本项目暂停菜单的对话框是自绘实现（不依赖 ImGui 弹窗），原因是它要参与
//        自己的视图栈与动画；这里讲的 ImGui 弹窗适合"设置页确认"这类轻量场景。
//
// ---------------------------------------------------------------------------
// 二、拖放（Drag&Drop）
// ---------------------------------------------------------------------------
//   ImGui 的拖放是"源 + 目标"配对：
//     源：   if (ImGui::BeginDragDropSource()) {
//                ImGui::SetDragDropPayload("MY_TYPE", &payload, sizeof(payload));
//                ImGui::Text("正在拖：%s", name);      // 拖动时跟随鼠标的提示
//                ImGui::EndDragDropSource();
//            }
//     目标： if (ImGui::BeginDragDropTarget()) {
//                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("MY_TYPE"))
//                    DoReorder(*(const int*)p->Data, my_index);
//                ImGui::EndDragDropTarget();
//            }
//   要点：
//     · payload 的类型字符串是双方约定的"协议"，不匹配就不会接受；
//     · 拖放目标必须在**每个可能的目标**上写，通常放在循环里逐行判定；
//     · 拖放期间的视觉反馈（高亮目标行）要自己做，否则用户不知道会插到哪。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 在列表末尾加一个"拖到这里放到最后"的区域（BeginDragDropTarget 放在循环外）。
//   2. 把模态框的确认按钮换成自绘控件（复用课时 5 的四步法）。
//   3. 给右键菜单加"删除"并让列表项真的消失（注意：循环里删除要小心索引）。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 模态框与普通弹窗的区别？谁会挡住下层交互？
//   2. 拖放的"源/目标"各自要写什么？payload 类型字符串的作用？
//   3. 为什么本项目暂停菜单的对话框是自绘而不是 ImGui 弹窗？
// ============================================================================

#include <cstdio>

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {

class Lesson11PopupScene final : public Scene {
public:
    const char* Name() const override { return "lesson11"; }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 11 · 弹层与拖放",
                                    "模态框 / 右键菜单 / 拖拽排序")) {
            Components::EndPanel(ui);
            return;
        }

        Section("一、模态对话框");
        if (ImGui::Button("打开模态确认框")) {
            ImGui::OpenPopup("##confirm_modal");
        }
        if (ImGui::BeginPopupModal("##confirm_modal", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("确定要执行这个危险操作吗？");
            ImGui::Separator();
            if (ImGui::Button("确定", ImVec2(120.0f, 0.0f))) {
                ++confirmed_count_;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(120.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("确认次数：%d", confirmed_count_);
        Code("if (need_open) ImGui::OpenPopup(\"id\");\n"
             "if (ImGui::BeginPopupModal(\"id\", nullptr, ...)) {\n"
             "    ...内容...\n"
             "    if (确定) ImGui::CloseCurrentPopup();\n"
             "    ImGui::EndPopup();\n"
             "}");

        // ---- 二、右键菜单 --------------------------------------------------
        Section("二、右键菜单（点外部自动关）");
        ImGui::TextUnformatted("在这里点右键：");
        ImGui::Button("我身上有右键菜单", ImVec2(0.0f, 44.0f));
        if (ImGui::BeginPopupContextItem("##ctx")) {
            if (ImGui::MenuItem("重命名")) {
                std::snprintf(last_action_, sizeof(last_action_), "重命名");
            }
            if (ImGui::MenuItem("复制")) {
                std::snprintf(last_action_, sizeof(last_action_), "复制");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("删除", nullptr, false, false)) {
                // 演示"禁用项"：最后一个参数 false 会把它变灰不可点
            }
            ImGui::EndPopup();
        }
        if (last_action_[0] != '\0') {
            ImGui::SameLine();
            ImGui::TextDisabled("上次动作：%s", last_action_);
        }

        // ---- 三、拖拽排序 --------------------------------------------------
        Section("三、拖拽排序（拖动下面的行，会插到鼠标所在的行之前）");
        Note("拖动时目标行会高亮 —— 这个反馈要自己做，否则用户不知道会插到哪。");
        const float row_h = 34.0f;
        for (int i = 0; i < kItemCount; ++i) {
            ImGui::PushID(i);
            const ImVec2 mn = ImGui::GetCursorScreenPos();
            const ImVec2 size(ImGui::GetContentRegionAvail().x, row_h);
            ImGui::InvisibleButton("##row", size);
            const bool hovered = ImGui::IsItemHovered();

            // 拖放源
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("COURSE_ROW", &i, sizeof(int));
                ImGui::Text("拖动第 %d 行：%s", i + 1, kItems[i]);
                ImGui::EndDragDropSource();
            }
            // 拖放目标
            bool drop_target = false;
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COURSE_ROW")) {
                    const int from = *static_cast<const int*>(payload->Data);
                    Reorder(from, i);
                }
                drop_target = true;
                ImGui::EndDragDropTarget();
            }
            ImGui::PopID();

            const ImVec2 mx(mn.x + size.x, mn.y + size.y);
            ImGui::GetWindowDrawList()->AddRectFilled(
                mn, mx,
                drop_target ? IM_COL32(0x4F, 0xA3, 0xFF, 0xFF)
                            : (hovered ? IM_COL32(0x2A, 0x30, 0x38, 0xFF)
                                       : IM_COL32(0x1A, 0x1D, 0x22, 0xFF)),
                4.0f);
            ImGui::GetWindowDrawList()->AddText(ImVec2(mn.x + 12.0f, mn.y + 8.0f),
                                                IM_COL32(0xF5, 0xF5, 0xF7, 0xFF), kItems[i]);
        }
        ImGui::TextDisabled("当前顺序：%s | %s | %s | %s | %s", kItems[0], kItems[1], kItems[2],
                            kItems[3], kItems[4]);

        Section("四、练习与验收");
        Bullet("练习：加一个「拖到这里放到最后」的目标区域（放在循环外）");
        Bullet("练习：把模态框的确认按钮改成自绘控件（复用课时 5）");
        Bullet("验收：模态框与普通弹窗区别？拖放源/目标各写什么？");

        EndLesson(ui);
    }

private:
    void Reorder(int from, int to) {
        if (from == to || from < 0 || from >= kItemCount || to < 0 || to >= kItemCount) {
            return;
        }
        const char* moved = kItems[from];
        if (from < to) {
            for (int i = from; i < to; ++i) {
                kItems[i] = kItems[i + 1];
            }
        } else {
            for (int i = from; i > to; --i) {
                kItems[i] = kItems[i - 1];
            }
        }
        kItems[to] = moved;
        std::snprintf(last_action_, sizeof(last_action_), "移动到第 %d 位", to + 1);
    }

    static constexpr int kItemCount = 5;
    const char* kItems[kItemCount] = {"存档槽 1", "存档槽 2", "存档槽 3", "存档槽 4", "存档槽 5"};
    int confirmed_count_ = 0;
    char last_action_[64] = {};
};

std::unique_ptr<Scene> CreateLesson11Popup() { return std::make_unique<Lesson11PopupScene>(); }

} // namespace gui_dev::course
