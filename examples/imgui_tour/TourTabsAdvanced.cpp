#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "examples/imgui_tour/ImGuiTourApp.h"
#include "examples/imgui_tour/TourTabs.h"
#include "ui/Icons.h"

namespace gui_dev::tour {
namespace {

// AddCallback 的函数签名是普通函数指针，所以计数器放文件级
int g_callback_hits = 0;

void Hint(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

bool DragDropSourceSquare(const char* label, ImU32 color, const char* payload) {
    const ImVec2 size(96.0f, ImGui::GetTextLineHeight() * 2.2f);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(label, size);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, origin + size, color, 6.0f);
    draw->AddRect(origin, origin + size, hovered ? IM_COL32(255, 255, 255, 220) : IM_COL32(0, 0, 0, 120), 6.0f);
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    draw->AddText(origin + (size - text_size) * 0.5f, IM_COL32(255, 255, 255, 255), label);

    bool began = false;
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        began = true;
        ImGui::SetDragDropPayload("TOUR_ITEM", payload, std::strlen(payload) + 1);
        ImGui::Text("拖拽中：%s", label);
        ImGui::EndDragDropSource();
    }
    return began;
}

} // namespace

// ================================================== 拖放 / 虚拟滚动 / 多选 ======

void DrawAdvancedTab(State& s) {
    if (!ImGui::BeginTable("##adv", 2, ImGuiTableFlags_SizingStretchSame)) {
        return;
    }
    ImGui::TableNextColumn();

    ImGui::SeparatorText("拖放：BeginDragDropSource / AcceptDragDropPayload");
    ImGui::TextDisabled("把方块拖到右边的目标区（手柄：按住 A 移动）");
    DragDropSourceSquare("存档 A", IM_COL32(0x00, 0x7A, 0xCC, 0xFF), "存档 A");
    ImGui::SameLine();
    DragDropSourceSquare("存档 B", IM_COL32(0x4E, 0xC9, 0xB0, 0xFF), "存档 B");
    ImGui::SameLine();
    DragDropSourceSquare("存档 C", IM_COL32(0xC5, 0x86, 0xC0, 0xFF), "存档 C");
    ImGui::TextDisabled("拖拽载荷类型：TOUR_ITEM（ImGuiPayload 里能拿到自定义结构）");

    const ImVec2 target_size(0.0f, ImGui::GetTextLineHeight() * 3.4f);
    ImGui::Button("拖放目标区", ImVec2(-1.0f, target_size.y));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TOUR_ITEM")) {
            const char* item = static_cast<const char*>(payload->Data);
            s.dropped.push_back(item);
            if (s.dropped.size() > 6) {
                s.dropped.erase(s.dropped.begin());
            }
            s.AppendLog("拖放接收：%s", item);
        }
        ImGui::EndDragDropTarget();
    }
    if (s.dropped.empty()) {
        ImGui::TextDisabled("（还没收到拖放）");
    } else {
        for (const std::string& item : s.dropped) {
            ImGui::BulletText("%s", item.c_str());
        }
    }

    ImGui::SeparatorText("虚拟滚动：ImGuiListClipper");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderInt("列表长度", &s.clipper_items, 100, 200000);
    ImGui::TextDisabled("本帧提交：%d ~ %d（共画 %d 行），其余行完全没有参与布局", s.clipper_first, s.clipper_last,
                        s.clipper_drawn);
    if (ImGui::BeginChild("##clipper", ImVec2(0.0f, ImGui::GetTextLineHeight() * 6.0f), ImGuiChildFlags_Borders)) {
        ImGuiListClipper clipper;
        clipper.Begin(s.clipper_items);
        int first = -1;
        int last = -1;
        int drawn = 0;
        while (clipper.Step()) {
            if (first < 0) {
                first = clipper.DisplayStart;
            }
            last = clipper.DisplayEnd;
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                ImGui::Text("条目 %06d · 只有可见行被提交给 ImGui", i);
                ++drawn;
            }
        }
        s.clipper_first = first;
        s.clipper_last = last;
        s.clipper_drawn = drawn;
    }
    ImGui::EndChild();

    ImGui::TableNextColumn();

    ImGui::SeparatorText("多选：BeginMultiSelect / ImGuiSelectionBasicStorage");
    ImGui::TextDisabled("1.92 新增的官方多选 API（方向键移动 + A 勾选；清空用 B/Esc）");
    static const char* const kItems[] = {"Game 01", "Game 02", "Game 03", "Game 04", "Game 05", "Game 06",
                                         "Game 07", "Game 08", "Game 09", "Game 10", "Game 11", "Game 12"};
    const int item_count = IM_ARRAYSIZE(kItems);

    if (ImGui::Button("清空选择")) {
        s.selection.Clear();
    }
    ImGui::SameLine();
    ImGui::Text("已选 %d 项", s.selection.Size);

    const ImGuiMultiSelectFlags ms_flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
    // 官方写法：Child 包住 MultiSelect 块（反过来会踩 EndTable 的状态断言）
    if (ImGui::BeginChild("##multisel", ImVec2(0.0f, ImGui::GetTextLineHeight() * 8.0f), ImGuiChildFlags_Borders)) {
        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags, s.selection.Size, item_count);
        s.selection.ApplyRequests(ms_io); // Begin 的请求（SetAll/Clear…）先落实
        for (int i = 0; i < item_count; ++i) {
            ImGui::PushID(i);
            const bool selected = s.selection.Contains(s.selection.GetStorageIdFromIndex(i));
            ImGui::SetNextItemSelectionUserData(i);
            if (ImGui::Selectable(kItems[i], selected)) {
                s.selection_touched = i;
            }
            ImGui::PopID();
        }
        ms_io = ImGui::EndMultiSelect();
        s.selection.ApplyRequests(ms_io); // End 的请求（点击/框选/键盘）在这落实
    }
    ImGui::EndChild();
    ImGui::TextDisabled("SelectionBasicStorage 里存的是 ImGuiID，遍历用 GetNextSelectedItem()");

    if (s.selection_touched >= 0) {
        ImGui::TextDisabled("最近点过：%s", kItems[s.selection_touched]);
    }

    ImGui::SeparatorText("为什么需要它");
    Hint("用 Clipper + MultiSelect，10 万条列表也能保持 60FPS：ImGui 本身不维护数据模型，"
         "这两个 helper 就是官方提供的「大数据量」方案。");
    ImGui::EndTable();
}

// ============================================================ 绘图 ============

void DrawDrawingTab(State& s) {
    // AddCallback 的回调在渲染阶段执行，这里把上一帧的次数取回来显示
    s.callback_hits = g_callback_hits;
    if (!ImGui::BeginTable("##draw", 2, ImGuiTableFlags_SizingStretchSame)) {
        return;
    }
    ImGui::TableNextColumn();

    ImGui::SeparatorText("ImDrawList 图元");
    ImGui::Checkbox("填充多边形", &s.draw_fill);
    ImGui::SameLine();
    ImGui::Checkbox("裁剪演示", &s.draw_clip);

    const ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * 16.0f);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##canvas", canvas_size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 canvas_max = origin + canvas_size;

    draw->AddRectFilled(origin, canvas_max, IM_COL32(0x18, 0x18, 0x1A, 0xFF), 6.0f);
    draw->AddRect(origin, canvas_max, IM_COL32(0x3C, 0x3C, 0x3C, 0xFF), 6.0f);

    // 1) 线 / 矩形 / 圆角
    draw->AddLine(origin + ImVec2(12, 14), origin + ImVec2(120, 14), IM_COL32(0x56, 0x9C, 0xD6, 0xFF), 2.0f);
    draw->AddRect(origin + ImVec2(12, 26), origin + ImVec2(120, 52), IM_COL32(0xCE, 0x91, 0x78, 0xFF), 4.0f, 0, 2.0f);
    draw->AddRectFilled(origin + ImVec2(12, 62), origin + ImVec2(120, 88), IM_COL32(0x2D, 0x2D, 0x30, 0xFF), 10.0f);
    // 2) 渐变（四色）
    draw->AddRectFilledMultiColor(origin + ImVec2(12, 98), origin + ImVec2(120, 124), IM_COL32(0x00, 0x7A, 0xCC, 0xFF),
                                 IM_COL32(0x4E, 0xC9, 0xB0, 0xFF), IM_COL32(0xC5, 0x86, 0xC0, 0xFF),
                                 IM_COL32(0xCE, 0x91, 0x78, 0xFF));
    // 3) 圆 / 三角 / 多边形
    draw->AddCircleFilled(origin + ImVec2(158, 38), 22.0f, IM_COL32(0x00, 0x7A, 0xCC, 0xFF), 32);
    draw->AddCircle(origin + ImVec2(158, 38), 22.0f, IM_COL32(0xFF, 0xFF, 0xFF, 0x88), 32, 2.0f);
    draw->AddTriangleFilled(origin + ImVec2(136, 96), origin + ImVec2(180, 96), origin + ImVec2(158, 66),
                            IM_COL32(0xDC, 0xDC, 0xAA, 0xFF));
    const ImVec2 hex[] = {ImVec2(210, 28), ImVec2(232, 40), ImVec2(232, 64), ImVec2(210, 76),
                          ImVec2(188, 64), ImVec2(188, 40)};
    if (s.draw_fill) {
        draw->AddConvexPolyFilled(hex, 6, IM_COL32(0xC5, 0x86, 0xC0, 0xFF));
    }
    draw->AddPolyline(hex, 6, IM_COL32(0xFF, 0xFF, 0xFF, 0xCC), ImDrawFlags_Closed, 2.0f);

    // 4) 贝塞尔 / Path API
    draw->AddBezierCubic(origin + ImVec2(12, 150), origin + ImVec2(60, 120), origin + ImVec2(120, 180),
                         origin + ImVec2(170, 145), IM_COL32(0x4E, 0xC9, 0xB0, 0xFF), 2.0f, 24);
    draw->PathLineTo(origin + ImVec2(200, 150));
    draw->PathBezierCubicCurveTo(origin + ImVec2(240, 120), origin + ImVec2(280, 180), origin + ImVec2(320, 145));
    draw->PathStroke(IM_COL32(0x56, 0x9C, 0xD6, 0xFF), 0, 2.0f);

    // 5) 动画（用 dt 驱动，不用帧计数）
    const float t = std::fmod(s.phase, 3.0f) / 3.0f;
    const float radius = 6.0f + 4.0f * std::sin(t * 6.2831853f);
    draw->AddCircleFilled(origin + ImVec2(360.0f, 40.0f), radius, IM_COL32(0xF1, 0x4C, 0x4C, 0xFF), 24);
    draw->AddText(origin + ImVec2(340.0f, 60.0f), IM_COL32(0x85, 0x85, 0x85, 0xFF), "动画");

    // 6) 裁剪：PushClipRect 限制绘制区域
    if (s.draw_clip) {
        draw->PushClipRect(origin + ImVec2(340, 90), origin + ImVec2(470, 130), true);
        for (int i = 0; i < 6; ++i) {
            draw->AddRectFilled(origin + ImVec2(344.0f + i * 22.0f, 94.0f), origin + ImVec2(392.0f + i * 22.0f, 126.0f),
                                IM_COL32(0x00, 0x7A, 0xCC, 0xC0), 3.0f);
        }
        draw->PopClipRect();
        draw->AddText(origin + ImVec2(340.0f, 134.0f), IM_COL32(0x85, 0x85, 0x85, 0xFF), "PushClipRect");
    }

    // 7) 渲染期回调：AddCallback 在真正提交渲染时被调用
    draw->AddCallback(
        [](const ImDrawList*, const ImDrawCmd*) {
            // 注意：这里在渲染阶段执行，不能调用 ImGui 的绘制函数
            ++g_callback_hits;
        },
        nullptr);
    draw->AddText(origin + ImVec2(12.0f, canvas_size.y - 20.0f), IM_COL32(0x85, 0x85, 0x85, 0xFF),
                  "AddCallback：渲染阶段回调（次数见右侧）");

    ImGui::TableNextColumn();

    ImGui::SeparatorText("纹理绘制");
    if (s.texture_valid) {
        const float tex_w = ImGui::GetContentRegionAvail().x;
        ImGui::TextDisabled("AddImage（原样）");
        ImGui::Image(s.texture_ref, ImVec2(tex_w, 20.0f));
        ImGui::TextDisabled("AddImageRounded + 动态 UV + 色调");
        ImGui::SliderFloat2("UV", s.texture_uv, 0.0f, 8.0f, "%.2f");
        const ImVec2 origin2 = ImGui::GetCursorScreenPos();
        const ImVec2 size2(tex_w, ImGui::GetTextLineHeight() * 4.0f);
        ImGui::InvisibleButton("##tex", size2);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddImageRounded(s.texture_ref, origin2, origin2 + size2, ImVec2(0, 0),
                            ImVec2(1.0f, ImMax(s.texture_uv[1], 0.05f)), IM_COL32(0x4E, 0xC9, 0xB0, 0xFF),
                            ImGui::GetTextLineHeight() * 1.6f);
        ImGui::TextDisabled("（UV 竖直方向重复贴图，圆角由 AddImageRounded 直接生成）");
    } else {
        ImGui::TextDisabled("贴图未加载：img/border_gradient.png");
    }

    ImGui::SeparatorText("文本绘制（AddText 系列）");
    {
        const ImVec2 origin3 = ImGui::GetCursorScreenPos();
        const ImVec2 size3(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * 4.6f);
        ImGui::InvisibleButton("##text", size3);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(origin3, origin3 + size3, IM_COL32(0x20, 0x20, 0x22, 0xFF), 4.0f);
        dl->AddText(origin3 + ImVec2(8, 4), IM_COL32(0xD4, 0xD4, 0xD4, 0xFF), "AddText：普通一行");
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.4f, origin3 + ImVec2(8, 24),
                    IM_COL32(0x00, 0x7A, 0xCC, 0xFF), "AddText：指定字体 + 字号");
        // CPU 裁剪：超出 clip 的部分不生成顶点
        const ImVec4 clip(origin3.x + 8.0f, origin3.y + 50.0f, origin3.x + 190.0f, origin3.y + 72.0f);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), origin3 + ImVec2(8, 52), IM_COL32(0xCE, 0x91, 0x78, 0xFF),
                    "AddText 的 cpu_fine_clip_rect：这一行会在 190px 处被截断（不生成顶点）", nullptr, 0.0f, &clip);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), origin3 + ImVec2(8, 74), IM_COL32(0x9A, 0xA1, 0xAC, 0xFF),
                    "带 wrap_width 的自动换行也可以用它绘制");
    }

    ImGui::SeparatorText("曲线图");
    {
        float samples[64];
        for (int i = 0; i < 64; ++i) {
            samples[i] = 0.5f + 0.5f * std::sin(s.phase * 2.0f + static_cast<float>(i) * 0.24f);
        }
        ImGui::PlotLines("##plot", samples, 64, 0, "sin 采样", 0.0f, 1.0f, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 4.0f));
        float bars[12];
        for (int i = 0; i < 12; ++i) {
            bars[i] = 0.5f + 0.5f * std::sin(s.phase + static_cast<float>(i) * 0.7f);
        }
        ImGui::PlotHistogram("##hist", bars, 12, 0, "直方图", 0.0f, 1.0f,
                             ImVec2(-1.0f, ImGui::GetTextLineHeight() * 3.5f));
        ImGui::TextDisabled("渲染期回调次数：%d", s.callback_hits);
    }
    ImGui::EndTable();
}

} // namespace gui_dev::tour
