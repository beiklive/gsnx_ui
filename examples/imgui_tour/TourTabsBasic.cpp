#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <cstdio>

#include "examples/imgui_tour/TourTabs.h"

namespace gui_dev::tour {
namespace {

constexpr float kHint = 0.62f; // 次要说明文字的不透明度

void Hint(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

// 带 "(?)" 悬停说明的标题
void SectionWithHelp(const char* title, const char* help) {
    ImGui::SeparatorText(title);
    if (help == nullptr) {
        return;
    }
    ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 24.0f);
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
        ImGui::TextUnformatted(help);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

} // namespace

// ============================================================ 总览 ============

void DrawOverviewTab(State& s) {
    if (!ImGui::BeginTable("##ov", 2, ImGuiTableFlags_SizingStretchSame)) {
        return;
    }
    ImGui::TableNextColumn();

    SectionWithHelp("文本与标记", "Text / TextColored / TextDisabled / LabelText / BulletText / SeparatorText");
    ImGui::Text("Text 普通文本");
    ImGui::TextColored(ImVec4(0.30f, 0.79f, 0.69f, 1.0f), "TextColored 着色文本");
    ImGui::TextDisabled("TextDisabled 次要文本");
    ImGui::LabelText("LabelText（标签右对齐）", "值 %.1f", s.progress * 100.0f);
    ImGui::BulletText("BulletText 列表项");
    ImGui::TextWrapped("TextWrapped 会自动按宽度折行，中文英文混排都可以正常断行。");

    SectionWithHelp("按钮", "Button / SmallButton / ArrowButton / ColorButton / BeginDisabled / InvisibleButton");
    if (ImGui::Button("Button")) {
        s.AppendLog("Button 被点击");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Small")) {
        s.AppendLog("SmallButton 被点击");
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
        s.AppendLog("ArrowButton 左");
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
        s.AppendLog("ArrowButton 右");
    }
    ImGui::SameLine();
    ImGui::ColorButton("##swatch", ImVec4(s.color[0], s.color[1], s.color[2], s.color[3]),
                       ImGuiColorEditFlags_NoTooltip, ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));

    ImGui::Checkbox("禁用区块 (BeginDisabled)", &s.disabled_block);
    ImGui::BeginDisabled(s.disabled_block);
    if (ImGui::Button("被禁用的按钮")) {
        s.AppendLog("不该发生");
    }
    ImGui::SameLine();
    ImGui::SliderFloat("禁用滑条", &s.slider_float, 0.0f, 1.0f);
    ImGui::EndDisabled();

    // InvisibleButton + 自绘：ImGui 里自定义控件的标准起点
    ImGui::TextUnformatted("InvisibleButton + ImDrawList 自绘：");
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size(150.0f, ImGui::GetFrameHeight());
    const bool invisible_clicked = ImGui::InvisibleButton("##invisible", size);
    const bool invisible_hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, origin + size, invisible_hovered ? IM_COL32(0x11, 0x77, 0xBB, 0xFF)
                                                                 : IM_COL32(0x2D, 0x2D, 0x30, 0xFF),
                        4.0f);
    draw->AddRect(origin, origin + size, IM_COL32(0x54, 0x54, 0x58, 0xFF), 4.0f);
    draw->AddText(origin + ImVec2(12.0f, 4.0f), IM_COL32(0xD4, 0xD4, 0xD4, 0xFF), "自绘按钮");
    if (invisible_clicked) {
        s.AppendLog("InvisibleButton 被点击");
    }

    ImGui::TableNextColumn();

    SectionWithHelp("进度与数值", "ProgressBar（含 overlay 文本）");
    ImGui::ProgressBar(s.progress, ImVec2(-1.0f, 0.0f));
    ImGui::SliderFloat("progress", &s.progress, 0.0f, 1.0f, "%.2f");

    SectionWithHelp("选择类", "Checkbox / RadioButton / Selectable");
    ImGui::Checkbox("选项 A", &s.check_a);
    ImGui::SameLine();
    ImGui::Checkbox("选项 B", &s.check_b);
    ImGui::SameLine();
    ImGui::Checkbox("选项 C", &s.check_c);
    ImGui::RadioButton("单选 1", &s.radio, 0);
    ImGui::SameLine();
    ImGui::RadioButton("单选 2", &s.radio, 1);
    ImGui::SameLine();
    ImGui::RadioButton("单选 3", &s.radio, 2);
    for (int i = 0; i < 3; ++i) {
        char label[32];
        std::snprintf(label, sizeof(label), "Selectable 行 %d", i + 1);
        if (ImGui::Selectable(label, s.selectable == i)) {
            s.selectable = i;
            s.AppendLog("Selectable 选中第 %d 行", i + 1);
        }
    }

    SectionWithHelp("悬停提示", "SetTooltip / BeginTooltip（含延迟与自定义内容）");
    ImGui::Checkbox("启用提示", &s.tooltip_enabled);
    ImGui::SameLine();
    ImGui::Button("悬停我");
    if (s.tooltip_enabled && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("ImGui::SetTooltip：跟随鼠标的即时提示");
    }
    ImGui::SameLine();
    ImGui::Button("结构化提示");
    if (s.tooltip_enabled && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("BeginTooltip 里可以放任意控件：");
        ImGui::Separator();
        ImGui::ProgressBar(s.progress, ImVec2(160.0f, 0.0f));
        ImGui::BulletText("延迟 %.2fs 后出现", ImGui::GetIO().MouseDoubleClickTime);
        ImGui::EndTooltip();
    }
    SectionWithHelp("样式压栈与布局查询", "PushStyleVar / PushStyleColor / GetContentRegionAvail / CalcTextSize");
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.79f, 0.30f, 0.30f, 1.0f));
    ImGui::Button("PushStyleVar + PushStyleColor");
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 text_size = ImGui::CalcTextSize("文字");
    ImGui::TextDisabled("可用区域 %.0f x %.0f；两字宽 %.1f px", avail.x, avail.y, text_size.x);
    Hint("提示：鼠标/键盘/手柄都能驱动这些控件（手柄走 ImGui 内置导航）。");

    ImGui::EndTable();
}

// ============================================================ 输入 ============

void DrawInputsTab(State& s) {
    if (!ImGui::BeginTable("##in", 3, ImGuiTableFlags_SizingStretchSame)) {
        return;
    }
    ImGui::TableNextColumn();
    SectionWithHelp("文本输入", "InputText（char[] / std::string）、InputTextWithHint、密码、Multiline");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##char", s.text, IM_ARRAYSIZE(s.text));
    ImGui::TextDisabled("char[128]：%s", s.text);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##std", &s.std_text)) {
        s.AppendLog("std::string 输入变化：%s", s.std_text.c_str());
    }
    ImGui::TextDisabled("std::string（imgui_stdlib.h）");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##hint", "带 placeholder 的输入框", s.text, IM_ARRAYSIZE(s.text));
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##pwd", &s.password, ImGuiInputTextFlags_Password);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextMultiline("##multi", &s.multiline, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 3.2f));
    Hint("InputText 还有 InputTextFlags：CharsDecimal / EnterReturnsTrue / ReadOnly 等。");

    ImGui::TableNextColumn();
    SectionWithHelp("数值输入", "InputInt / InputFloat / DragInt / DragFloat（速度、范围、格式）");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputInt("InputInt", &s.int_value);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputFloat("InputFloat", &s.float_value, 0.1f, 1.0f, "%.3f");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::DragInt("DragInt", &s.drag_int, 1.0f, 0, 255);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::DragFloat("DragFloat", &s.drag_float, s.drag_speed, s.drag_min, s.drag_max, "%.2f");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::DragFloat3("DragFloat3", s.vec3, 0.01f, 0.0f, 1.0f);

    ImGui::SeparatorText("DragFloat 参数");
    ImGui::SliderFloat("速度", &s.drag_speed, 0.01f, 5.0f, "%.2f");
    ImGui::DragFloatRange2("范围", &s.drag_min, &s.drag_max, 1.0f, 0.0f, 200.0f);
    Hint("DragFloatRange2 一次拖两个值，很适合做区间选择。");

    ImGui::TableNextColumn();
    SectionWithHelp("滑条 / 下拉 / 颜色", "SliderInt、SliderFloat、VSliderFloat、SliderAngle、Combo、ListBox、ColorEdit");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderInt("SliderInt", &s.slider_int, 0, 100);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderFloat("SliderFloat", &s.slider_float, 0.0f, 1.0f, "%.3f");

    ImGui::BeginGroup();
    ImGui::VSliderFloat("##v", ImVec2(ImGui::GetFrameHeight(), ImGui::GetTextLineHeight() * 4.5f),
                        &s.slider_float, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::SliderAngle("SliderAngle", &s.slider_angle);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("Combo", s.combo == 0 ? "整数缩放" : (s.combo == 1 ? "线性过滤" : "CRT 扫描线"))) {
        const char* items[] = {"整数缩放", "线性过滤", "CRT 扫描线"};
        for (int i = 0; i < 3; ++i) {
            if (ImGui::Selectable(items[i], s.combo == i)) {
                s.combo = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndGroup();
    ImGui::EndGroup();

    const char* list_items[] = {"Game 01", "Game 02", "Game 03", "Game 04"};
    if (ImGui::BeginListBox("ListBox", ImVec2(-1.0f, ImGui::GetTextLineHeight() * 4.0f))) {
        for (int i = 0; i < 4; ++i) {
            if (ImGui::Selectable(list_items[i], s.listbox == i)) {
                s.listbox = i;
            }
        }
        ImGui::EndListBox();
    }

    ImGui::ColorEdit4("ColorEdit4", s.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorPicker4("ColorPicker4", s.color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar);
    ImGui::EndTable();
}

// ============================================================ 布局 ============

void DrawLayoutTab(State& s) {
    if (!ImGui::BeginTable("##lay", 2, ImGuiTableFlags_SizingStretchSame)) {
        return;
    }
    ImGui::TableNextColumn();

    SectionWithHelp("折叠 / 树", "CollapsingHeader / TreeNodeEx（含 Leaf、Selected、DefaultOpen）");
    if (ImGui::CollapsingHeader("CollapsingHeader（默认展开）", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted("折叠区里的内容");
    }
    if (ImGui::TreeNodeEx("TreeNodeEx 父节点", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TreeNodeEx("子节点 A", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        ImGui::TreeNodeEx("子节点 B（可选中）",
                          ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                              ImGuiTreeNodeFlags_SpanAvailWidth |
                              (s.tree_selected ? ImGuiTreeNodeFlags_Selected : 0));
        if (ImGui::IsItemClicked()) {
            s.tree_selected = !s.tree_selected;
            s.AppendLog("TreeNode 选中状态：%s", s.tree_selected ? "开" : "关");
        }
        ImGui::TreeNodeEx("子节点 C", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        ImGui::TreePop();
    }

    SectionWithHelp("同排 / 缩进 / 分组", "SameLine / Indent / Unindent / Dummy / BeginGroup / AlignTextToFramePadding");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("文本与控件对齐");
    ImGui::SameLine();
    ImGui::Button("同行按钮");
    ImGui::Indent();
    ImGui::TextUnformatted("Indent 缩进一级");
    ImGui::Unindent();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::BeginGroup();
    ImGui::Button("BeginGroup");
    ImGui::Button("整组一起布局");
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::TextWrapped("BeginGroup 之后，同排与尺寸计算都按整组算。");

    SectionWithHelp("嵌套 TabBar", "BeginTabBar（内层 Tab，不带窗口）");
    if (ImGui::BeginTabBar("##inner_tabs")) {
        if (ImGui::BeginTabItem("内层 A")) {
            ImGui::SliderFloat("内层值 A", &s.nested_value, 0.0f, 100.0f);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("内层 B")) {
            ImGui::SliderFloat("内层值 B", &s.nested_value, 0.0f, 100.0f);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("内层 C")) {
            ImGui::TextUnformatted("TabBar 可以嵌套，也可以在窗口里手写页签。");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::TableNextColumn();

    SectionWithHelp("子窗口", "BeginChild（有独立裁剪与滚动、可带边框/可调整大小）");
    if (ImGui::BeginChild("##child", ImVec2(0.0f, ImGui::GetTextLineHeight() * 4.6f), ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        for (int i = 0; i < 12; ++i) {
            ImGui::Text("子窗口内的第 %d 行（超出部分被裁剪）", i + 1);
        }
    }
    ImGui::EndChild();

    SectionWithHelp("表格", "BeginTable（表头 / 边框 / 斑马纹 / 可排序 / 可调整列宽）");
    struct Row {
        const char* name;
        int size;
        float score;
    };
    static Row rows[] = {{"Game 01", 2048, 7.5f}, {"Game 02", 4096, 9.1f}, {"Game 03", 1024, 6.2f},
                         {"Game 04", 8192, 8.8f}, {"Game 05", 3072, 5.4f}, {"Game 06", 5120, 9.9f}};
    const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Sortable |
                                        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##table", 3, table_flags, ImVec2(0.0f, ImGui::GetTextLineHeight() * 7.4f))) {
        ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("大小", ImGuiTableColumnFlags_PreferSortDescending);
        ImGui::TableSetupColumn("评分");
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            // 点表头排序：这里用最简单的冒泡，演示 SortSpecs 的用法
            if (specs->SpecsDirty) {
                const ImGuiTableColumnSortSpecs& spec = specs->Specs[0];
                for (int i = 0; i < IM_ARRAYSIZE(rows) - 1; ++i) {
                    for (int j = 0; j < IM_ARRAYSIZE(rows) - 1 - i; ++j) {
                        bool swap = false;
                        if (spec.ColumnIndex == 0) {
                            swap = std::string(rows[j].name) > std::string(rows[j + 1].name);
                        } else if (spec.ColumnIndex == 1) {
                            swap = rows[j].size > rows[j + 1].size;
                        } else {
                            swap = rows[j].score > rows[j + 1].score;
                        }
                        if (spec.SortDirection == ImGuiSortDirection_Descending) {
                            swap = !swap;
                        }
                        if (swap) {
                            const Row tmp = rows[j];
                            rows[j] = rows[j + 1];
                            rows[j + 1] = tmp;
                        }
                    }
                }
                specs->SpecsDirty = false;
            }
        }
        for (int i = 0; i < IM_ARRAYSIZE(rows); ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const bool selected = (s.table_row_selected == i);
            if (ImGui::Selectable(rows[i].name, selected, ImGuiSelectableFlags_SpanAllColumns)) {
                s.table_row_selected = i;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d KB", rows[i].size);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.1f", rows[i].score);
        }
        ImGui::EndTable();
    }

    SectionWithHelp("可拖拽分隔", "SplitterBehavior（imgui_internal.h）");
    const float avail_w = ImGui::GetContentRegionAvail().x;
    const ImVec2 splitter_pos = ImGui::GetCursorScreenPos();
    ImGui::BeginChild("##split_l", ImVec2(avail_w * s.splitter - 4.0f, ImGui::GetTextLineHeight() * 3.0f),
                      ImGuiChildFlags_Borders);
    ImGui::TextWrapped("左栏（宽度由分隔条决定）");
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::InvisibleButton("##splitter", ImVec2(8.0f, ImGui::GetTextLineHeight() * 3.0f));
    if (ImGui::IsItemActive() || ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (ImGui::IsItemActive()) {
        s.splitter += ImGui::GetIO().MouseDelta.x / ImMax(avail_w, 1.0f);
        s.splitter = ImClamp(s.splitter, 0.15f, 0.85f);
    }
    (void)splitter_pos;
    ImGui::SameLine();
    ImGui::BeginChild("##split_r", ImVec2(0.0f, ImGui::GetTextLineHeight() * 3.0f), ImGuiChildFlags_Borders);
    ImGui::TextWrapped("右栏：拖动中间的分隔条试试");
    ImGui::EndChild();

    ImGui::EndTable();
}

// ============================================================ 弹层 ============

void DrawPopupsTab(State& s) {
    if (!ImGui::BeginTable("##pop", 2, ImGuiTableFlags_SizingStretchSame)) {
        return;
    }
    ImGui::TableNextColumn();

    SectionWithHelp("普通弹层", "OpenPopup / BeginPopup / BeginPopupContextItem / BeginPopupContextWindow");
    if (ImGui::Button("打开 Popup")) {
        ImGui::OpenPopup("##tour_popup");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("（Popup 跟着窗口，不会阻塞其它操作）");
    if (ImGui::BeginPopup("##tour_popup")) {
        ImGui::TextUnformatted("Popup 内容");
        ImGui::Separator();
        if (ImGui::MenuItem("菜单项")) {
            s.AppendLog("Popup 菜单项被点击");
        }
        ImGui::EndPopup();
    }

    ImGui::Button("右键/长按这里");
    if (ImGui::BeginPopupContextItem("##ctx_item")) {
        ImGui::TextDisabled("BeginPopupContextItem");
        if (ImGui::MenuItem("上下文项 1")) {
            s.AppendLog("上下文项 1");
        }
        if (ImGui::MenuItem("上下文项 2")) {
            s.AppendLog("上下文项 2");
        }
        ImGui::EndPopup();
    }
    ImGui::TextWrapped("在这个文本区域上右键（手柄：长按 A）可以弹出上下文菜单。");
    if (ImGui::BeginPopupContextItem("##ctx_text")) {
        ImGui::TextDisabled("在文本上弹出的菜单");
        ImGui::MenuItem("复制", "Ctrl+C");
        ImGui::EndPopup();
    }

    SectionWithHelp("模态对话框", "OpenPopup + BeginPopupModal：独占输入、可带关闭按钮");
    if (ImGui::Button("打开 Modal")) {
        s.open_modal = true;
        ImGui::OpenPopup("##tour_modal");
    }
    ImGui::SameLine();
    if (ImGui::Button("打开确认框（3 按钮）")) {
        s.open_modal = true;
        ImGui::OpenPopup("##tour_modal3");
    }
    if (ImGui::BeginPopupModal("##tour_modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("是否把当前状态保存到槽位？");
        ImGui::Separator();
        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputText("槽位名", s.modal_input, IM_ARRAYSIZE(s.modal_input));
        ImGui::Spacing();
        if (ImGui::Button("取消", ImVec2(110.0f, 0.0f))) {
            s.modal_choice = 0;
            s.AppendLog("Modal：取消");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("确定", ImVec2(110.0f, 0.0f))) {
            s.modal_choice = 1;
            s.AppendLog("Modal：确定（%s）", s.modal_input);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("##tour_modal3", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("三个按钮的确认框：");
        ImGui::Spacing();
        if (ImGui::Button("否", ImVec2(90.0f, 0.0f))) {
            s.modal_choice = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("稍后", ImVec2(90.0f, 0.0f))) {
            s.modal_choice = 2;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("是", ImVec2(90.0f, 0.0f))) {
            s.modal_choice = 1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::TextDisabled("上次结果：%d（-1 = 还没选过）", s.modal_choice);

    ImGui::TableNextColumn();

    SectionWithHelp("菜单栏", "BeginMenuBar / BeginMenu / MenuItem（快捷键、选中、禁用、子菜单）");
    if (ImGui::BeginChild("##menubar_host", ImVec2(0.0f, ImGui::GetTextLineHeight() * 2.2f),
                          ImGuiChildFlags_Borders, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("文件")) {
                if (ImGui::MenuItem("新建")) {
                    s.menu_choice = 0;
                    s.AppendLog("菜单：新建");
                }
                if (ImGui::MenuItem("打开", "Ctrl+O")) {
                    s.menu_choice = 1;
                    s.AppendLog("菜单：打开");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("保存", "Ctrl+S", false, true)) {
                    s.menu_choice = 2;
                    s.AppendLog("菜单：保存");
                }
                if (ImGui::MenuItem("另存为", nullptr, false, false)) {
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("视图")) {
                ImGui::MenuItem("显示网格", nullptr, &s.draw_clip);
                ImGui::MenuItem("显示坐标", nullptr, &s.tooltip_enabled);
                if (ImGui::BeginMenu("主题")) {
                    if (ImGui::MenuItem("Dark")) {
                        ImGui::StyleColorsDark();
                    }
                    if (ImGui::MenuItem("Light")) {
                        ImGui::StyleColorsLight();
                    }
                    if (ImGui::MenuItem("Classic")) {
                        ImGui::StyleColorsClassic();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("帮助")) {
                ImGui::MenuItem("关于", nullptr, &s.show_about);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::TextDisabled("菜单栏里点「视图 → 主题」可以直接换 ImGui 内置配色。");
    }
    ImGui::EndChild();

    SectionWithHelp("提示气泡", "SetTooltip / BeginTooltip / IsItemHovered(DelayNormal)");
    ImGui::Button("普通提示");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("立即出现");
    }
    ImGui::SameLine();
    ImGui::Button("延迟提示");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("悬停 %.1fs 后出现", ImGui::GetIO().MouseDoubleClickTime);
    }
    ImGui::SameLine();
    ImGui::Button("富内容提示");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("提示里可以有：");
        ImGui::Separator();
        ImGui::ProgressBar(s.progress, ImVec2(140.0f, 0.0f));
        ImGui::ColorButton("##tt", ImVec4(s.color[0], s.color[1], s.color[2], 1.0f), 0, ImVec2(24.0f, 24.0f));
        ImGui::SameLine();
        ImGui::TextUnformatted("任意控件");
        ImGui::EndTooltip();
    }

    SectionWithHelp("模拟一个「设置页」", "把上面这些组合起来就是常见的设置界面");
    ImGui::Text("画面：%s", s.combo == 0 ? "整数缩放" : (s.combo == 1 ? "线性过滤" : "CRT 扫描线"));
    ImGui::Text("音量：%.0f%%", s.slider_float * 100.0f);
    ImGui::Text("主题色：#%02X%02X%02X", static_cast<int>(s.color[0] * 255.0f), static_cast<int>(s.color[1] * 255.0f),
                static_cast<int>(s.color[2] * 255.0f));
    ImGui::Text("上次菜单项：%d", s.menu_choice);
    Hint("Modal 打开时会独占输入（ImGui 会把其它窗口的鼠标/键盘输入挡住）。");

    ImGui::EndTable();
}

} // namespace gui_dev::tour
