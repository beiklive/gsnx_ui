#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <cstdio>
#include <string>

#include "examples/imgui_tour/ImGuiTourApp.h"
#include "examples/imgui_tour/TourTabs.h"
#include "ui/Icons.h"

namespace gui_dev::tour {
namespace {

void Hint(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

// 把 ConfigFlags / BackendFlags 的位翻成可读文本
std::string ConfigFlagsText(ImGuiConfigFlags flags) {
    std::string out;
    auto add = [&](ImGuiConfigFlags bit, const char* name) {
        if (flags & bit) {
            if (!out.empty()) {
                out += " | ";
            }
            out += name;
        }
    };
    add(ImGuiConfigFlags_NavEnableKeyboard, "NavEnableKeyboard");
    add(ImGuiConfigFlags_NavEnableGamepad, "NavEnableGamepad");
    add(ImGuiConfigFlags_NoMouse, "NoMouse");
    add(ImGuiConfigFlags_NoMouseCursorChange, "NoMouseCursorChange");
    return out.empty() ? "(无)" : out;
}

std::string BackendFlagsText(ImGuiBackendFlags flags) {
    std::string out;
    auto add = [&](ImGuiBackendFlags bit, const char* name) {
        if (flags & bit) {
            if (!out.empty()) {
                out += " | ";
            }
            out += name;
        }
    };
    add(ImGuiBackendFlags_HasGamepad, "HasGamepad");
    add(ImGuiBackendFlags_HasMouseCursors, "HasMouseCursors");
    add(ImGuiBackendFlags_HasSetMousePos, "HasSetMousePos");
    add(ImGuiBackendFlags_RendererHasVtxOffset, "RendererHasVtxOffset");
    add(ImGuiBackendFlags_RendererHasTextures, "RendererHasTextures");
    return out.empty() ? "(无)" : out;
}

} // namespace

// ======================================================== 字体与样式 ==========

void DrawFontStyleTab(State& s) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    if (!ImGui::BeginTable("##font", 2, ImGuiTableFlags_SizingStretchSame)) {
        return;
    }
    ImGui::TableNextColumn();

    ImGui::SeparatorText("已加载的字体（ImFontAtlas）");
    ImGui::TextDisabled("字体数：%d；1.92 起字形按需光栅化，不需要手工 Build()", io.Fonts->Fonts.Size);
    if (s.font_index >= io.Fonts->Fonts.Size) {
        s.font_index = 0;
    }
    for (int i = 0; i < io.Fonts->Fonts.Size; ++i) {
        ImFont* font = io.Fonts->Fonts[i];
        ImGui::PushID(i);
        const bool selected = (s.font_index == i);
        const std::string label = std::string(font->GetDebugName() != nullptr ? font->GetDebugName() : "(未命名)") +
                                  "  ·  size " + std::to_string(static_cast<int>(font->LegacySize));
        if (ImGui::Selectable(label.c_str(), selected)) {
            s.font_index = i;
        }
        ImGui::PopID();
    }
    ImFont* active_font = io.Fonts->Fonts.Size > 0 ? io.Fonts->Fonts[s.font_index] : ImGui::GetFont();
    ImGui::Spacing();
    ImGui::TextDisabled("用选中的字体渲染：");
    ImGui::PushFont(active_font, 0.0f);
    ImGui::Text("字体样例：GUI_DEV 0123 中文渲染 ABC xyz");
    ImGui::Text("按键图标：%s%s%s%s  Material：%s%s%s", Icons::Glyph(Icons::Button::A), Icons::Glyph(Icons::Button::B),
                Icons::Glyph(Icons::Button::X), Icons::Glyph(Icons::Button::Y),
                Icons::Glyph(Icons::Material::Save), Icons::Glyph(Icons::Material::Settings),
                Icons::Glyph(Icons::Material::Memory));
    ImGui::PopFont();
    Hint("字体合并用 ImFontConfig::MergeMode + GlyphExcludeRanges（本项目用它避免 NintendoExt 私用区挡住图标）。");

    ImGui::SeparatorText("动态字号：PushFont(font, size)");
    ImGui::Checkbox("启用动态字号", &s.dynamic_size);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderFloat("字号", &s.dynamic_font_size, 8.0f, 48.0f, "%.0f px");
    if (s.dynamic_size) {
        ImGui::PushFont(active_font, s.dynamic_font_size);
        ImGui::Text("同一字体在不同字号下：在 720p 下按需光栅化，不会糊");
        ImGui::PopFont();
    }

    ImGui::TableNextColumn();

    ImGui::SeparatorText("字号基准：style.FontSizeBase / FontScaleMain");
    if (ImGui::SliderFloat("FontSizeBase", &s.dynamic_font_size, 12.0f, 30.0f, "%.0f px")) {
        style.FontSizeBase = s.dynamic_font_size;
    }
    ImGui::TextDisabled("FontSizeBase=%.0f（全局缩放前）· FontScaleMain=%.2f（后端每帧重置为 1.0）",
                        style.FontSizeBase, style.FontScaleMain);
    ImGui::TextDisabled("GetFontSize() = %.1f px", ImGui::GetFontSize());
    Hint("本项目 720p 手持基准：FontSizeBase = 17px；桌面大窗口可以调大。");

    ImGui::SeparatorText("运行时改样式（ImGuiStyle）");
    if (ImGui::SliderFloat("FrameRounding", &s.style_rounding, 0.0f, 12.0f, "%.1f")) {
        style.FrameRounding = s.style_rounding;
    }
    if (ImGui::SliderFloat("FrameBorderSize", &s.style_border, 0.0f, 3.0f, "%.1f")) {
        style.FrameBorderSize = s.style_border;
    }
    if (ImGui::SliderFloat("ItemSpacing.x", &s.style_spacing, 0.0f, 20.0f, "%.0f")) {
        style.ItemSpacing.x = s.style_spacing;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("恢复默认")) {
        ImGui::StyleColorsDark();
        style = ImGui::GetStyle();
    }

    ImGui::SeparatorText("内置样式与样式编辑器");
    if (ImGui::Button("Dark")) {
        ImGui::StyleColorsDark();
    }
    ImGui::SameLine();
    if (ImGui::Button("Light")) {
        ImGui::StyleColorsLight();
    }
    ImGui::SameLine();
    if (ImGui::Button("Classic")) {
        ImGui::StyleColorsClassic();
    }
    ImGui::SameLine();
    ImGui::Checkbox("打开 ShowStyleEditor", &s.show_style_editor);
    ImGui::TextDisabled("样式编辑器是 ImGui 自带的窗口，能实时改颜色/尺寸并导出 C++ 代码。");

    ImGui::SeparatorText("颜色表（style.Colors）");
    if (ImGui::BeginChild("##colors", ImVec2(0.0f, ImGui::GetTextLineHeight() * 5.0f), ImGuiChildFlags_Borders)) {
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            ImGui::ColorButton(ImGui::GetStyleColorName(i), style.Colors[i], ImGuiColorEditFlags_NoTooltip,
                               ImVec2(18.0f, 14.0f));
            ImGui::SameLine();
            ImGui::Text("%s", ImGui::GetStyleColorName(i));
        }
    }
    ImGui::EndChild();
    ImGui::EndTable();
}

// ========================================================== 系统工具 ==========

void DrawSystemTab(State& s) {
    ImGuiIO& io = ImGui::GetIO();

    if (!ImGui::BeginTable("##sys", 2, ImGuiTableFlags_SizingStretchSame)) {
        return;
    }
    ImGui::TableNextColumn();

    ImGui::SeparatorText("IO 状态（ImGuiIO）");
    ImGui::Text("Framerate    : %.1f FPS (%.2f ms)", io.Framerate, io.DeltaTime * 1000.0f);
    ImGui::Text("DisplaySize  : %.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("FramebufferScale: %.2f x %.2f（字体光栅密度）", io.DisplayFramebufferScale.x,
                io.DisplayFramebufferScale.y);
    if (ImGui::IsMousePosValid()) {
        ImGui::Text("MousePos     : %.0f, %.0f（按键 %d%d%d）", io.MousePos.x, io.MousePos.y, io.MouseDown[0] ? 1 : 0,
                    io.MouseDown[1] ? 1 : 0, io.MouseDown[2] ? 1 : 0);
    } else {
        ImGui::Text("MousePos     : (无鼠标，IsMousePosValid() = false)");
    }
    ImGui::Text("Capture      : 鼠标 %s / 键盘 %s / 文本 %s", io.WantCaptureMouse ? "是" : "否",
                io.WantCaptureKeyboard ? "是" : "否", io.WantTextInput ? "是" : "否");
    ImGui::Text("Backend      : %s + %s", io.BackendPlatformName ? io.BackendPlatformName : "?",
                io.BackendRendererName ? io.BackendRendererName : "?");
    ImGui::TextWrapped("ConfigFlags  : %s", ConfigFlagsText(io.ConfigFlags).c_str());
    ImGui::TextWrapped("BackendFlags : %s", BackendFlagsText(io.BackendFlags).c_str());

    ImGui::SeparatorText("渲染统计（ImDrawData）");
    ImGui::Text("DrawCall %d · 顶点 %d · 索引 %d · 窗口 %d", s.draw_calls, s.vertices, s.indices, s.windows);
    ImGui::Text("渲染期回调次数：%d", s.callback_hits);

    ImGui::SeparatorText("按键状态（IsKeyDown / IsKeyPressed）");
    ImGui::Text("键盘 Space=%d Enter=%d Esc=%d", ImGui::IsKeyDown(ImGuiKey_Space) ? 1 : 0,
                ImGui::IsKeyDown(ImGuiKey_Enter) ? 1 : 0, ImGui::IsKeyDown(ImGuiKey_Escape) ? 1 : 0);
    ImGui::Text("手柄 Dpad=%d%d%d%d A=%d B=%d X=%d Y=%d L1=%d R1=%d", ImGui::IsKeyDown(ImGuiKey_GamepadDpadUp) ? 1 : 0,
                ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown) ? 1 : 0, ImGui::IsKeyDown(ImGuiKey_GamepadDpadLeft) ? 1 : 0,
                ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight) ? 1 : 0, ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown) ? 1 : 0,
                ImGui::IsKeyDown(ImGuiKey_GamepadFaceRight) ? 1 : 0, ImGui::IsKeyDown(ImGuiKey_GamepadFaceLeft) ? 1 : 0,
                ImGui::IsKeyDown(ImGuiKey_GamepadFaceUp) ? 1 : 0, ImGui::IsKeyDown(ImGuiKey_GamepadL1) ? 1 : 0,
                ImGui::IsKeyDown(ImGuiKey_GamepadR1) ? 1 : 0);
    ImGui::Checkbox("NavEnableKeyboard", &s.nav_keyboard);
    ImGui::SameLine();
    ImGui::Checkbox("NavEnableGamepad", &s.nav_gamepad);
    if (s.nav_keyboard) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    }
    if (s.nav_gamepad) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    }
    // NavId 在上下文里（imgui_internal.h），代表当前被导航高亮的 item
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    ImGui::Text("导航：NavActive=%d NavVisible=%d NavId=%u", io.NavActive ? 1 : 0, io.NavVisible ? 1 : 0,
                static_cast<unsigned>(ctx != nullptr ? ctx->NavId : 0));

    ImGui::SeparatorText("IO 配置项（io.Config*）");
    ImGui::Text("DragThreshold %.1f · 双击 %.2fs · 单击延迟 %.2fs（都在 io 上）", io.MouseDragThreshold,
                io.MouseDoubleClickTime, io.MouseSingleClickDelay);
    ImGui::Text("ConfigNavSwapGamepadButtons=%d · ConfigNavCursorVisibleAlways=%d",
                io.ConfigNavSwapGamepadButtons ? 1 : 0, io.ConfigNavCursorVisibleAlways ? 1 : 0);
    ImGui::Checkbox("NoMouseCursorChange", &s.no_mouse_cursor_change);
    ImGui::SameLine();
    if (s.no_mouse_cursor_change) {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    }
    ImGui::SameLine();
    ImGui::Checkbox("NavSwapGamepadButtons", &s.swap_gamepad_buttons);
    io.ConfigNavSwapGamepadButtons = s.swap_gamepad_buttons;
    Hint("这些开关都是运行时生效的，改完立刻能看到导航/光标行为变化。");

    ImGui::SeparatorText("ImGuiStorage（键值持久化）");
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID key = ImGui::GetID("tour_counter");
    s.storage_counter = storage->GetInt(key, 0);
    if (ImGui::Button("计数 +1")) {
        storage->SetInt(key, s.storage_counter + 1);
    }
    ImGui::SameLine();
    ImGui::Text("storage[\"tour_counter\"] = %d", s.storage_counter);

    ImGui::TableNextColumn();

    ImGui::SeparatorText("剪贴板");
    ImGui::TextWrapped("当前内容：%s", ImGui::GetClipboardText() != nullptr ? ImGui::GetClipboardText() : "(空)");
    if (ImGui::Button("写入剪贴板")) {
        ImGui::SetClipboardText("来自 ImGui::SetClipboardText");
    }
    ImGui::SameLine();
    if (ImGui::Button("复制到输入框")) {
        s.std_text = ImGui::GetClipboardText() != nullptr ? ImGui::GetClipboardText() : "";
    }

    ImGui::SeparatorText("ini 持久化");
    ImGui::TextDisabled("IniFilename：%s", io.IniFilename != nullptr ? io.IniFilename : "(禁用)");
    static std::string saved_ini;
    if (ImGui::Button("SaveIniSettingsToMemory")) {
        saved_ini = ImGui::SaveIniSettingsToMemory();
        s.AppendLog("ini 已序列化到内存（%d 字节）", static_cast<int>(saved_ini.size()));
    }
    ImGui::SameLine();
    if (ImGui::Button("LoadIniSettingsFromMemory") && !saved_ini.empty()) {
        ImGui::LoadIniSettingsFromMemory(saved_ini.c_str(), saved_ini.size());
        s.AppendLog("ini 已从内存恢复");
    }
    ImGui::TextDisabled("窗口位置/表头宽度这类状态就是靠它保存的（Settings 里存）。");

    ImGui::SeparatorText("日志 + 过滤（ImGuiTextBuffer / ImGuiTextFilter）");
    s.filter.Draw("过滤", -1.0f);
    ImGui::SameLine();
    if (ImGui::Button("加 20 行")) {
        for (int i = 0; i < 20; ++i) {
            s.AppendLog("模拟日志 %d：拖放/输入/导航事件", s.log_sequence);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("清空")) {
        s.log.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("自动滚动", &s.log_autoscroll);
    if (ImGui::BeginChild("##log", ImVec2(0.0f, ImGui::GetTextLineHeight() * 6.0f), ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        s.filter.Draw("##filter", -1.0f);
        ImGui::Separator();
        const char* begin = s.log.begin();
        const char* end = s.log.end();
        for (const char* line = begin; line < end;) {
            const char* line_end = line;
            while (line_end < end && *line_end != '\n') {
                ++line_end;
            }
            if (s.filter.PassFilter(line, line_end)) {
                ImGui::TextUnformatted(line, line_end);
            }
            line = line_end + 1;
        }
        if (s.log_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    ImGui::SeparatorText("ImGui 自带的调试 / 工具窗口");
    ImGui::Checkbox("ShowDemoWindow", &s.show_demo);
    ImGui::SameLine();
    ImGui::Checkbox("ShowMetricsWindow", &s.show_metrics);
    ImGui::SameLine();
    ImGui::Checkbox("ShowIDStackToolWindow", &s.show_id_stack);
    ImGui::Checkbox("ShowDebugLogWindow", &s.show_debug_log);
    ImGui::SameLine();
    ImGui::Checkbox("ShowStyleEditor", &s.show_style_editor);
    ImGui::SameLine();
    ImGui::Checkbox("ShowAboutWindow", &s.show_about);
    Hint("ShowDemoWindow 是官方全功能示例：想找某个控件怎么写，打开它对着看最快。");
    ImGui::EndTable();
}

} // namespace gui_dev::tour
