#include "component_view/Theme.h"

#include "component_view/Types.h"

namespace gui_dev::cv::Theme {

ImU32 Alpha(ImU32 color, float alpha) {
    const float base = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF);
    const float out = Clampf(base * Clampf(alpha, 0.0f, 1.0f), 0.0f, 255.0f);
    return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(out) << IM_COL32_A_SHIFT);
}

ImU32 Mix(ImU32 a, ImU32 b, float t) {
    t = Clampf(t, 0.0f, 1.0f);
    const ImVec4 va = ImGui::ColorConvertU32ToFloat4(a);
    const ImVec4 vb = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(va.x + (vb.x - va.x) * t, va.y + (vb.y - va.y) * t,
                                                 va.z + (vb.z - va.z) * t, va.w + (vb.w - va.w) * t));
}

void ApplyToImGui() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = kRadius;
    style.ChildRounding = kRadius;
    style.FrameRounding = kRadiusSmall;
    style.PopupRounding = kRadiusSmall;
    style.ScrollbarRounding = kRadius;
    style.GrabRounding = kRadiusSmall;
    style.TabRounding = kRadiusSmall;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(kGap, kGap);

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(kTextPrimary);
    c[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(kTextMuted);
    c[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(kBgEditor);
    c[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(kBgSideBar);
    c[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(kBorder);
    c[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(kBgWidget);
    c[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(kBgWidgetHi);
    c[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(kBgInput);
    c[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(kBgActivity);
    c[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(kBgActivity);
    c[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(kButton);
    c[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(kAccentHover);
    c[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(kButtonActive);
    c[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(kSelection);
    c[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(kAccent);
    c[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(kAccentHover);
    c[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(kAccent);
    c[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(kAccent);
    c[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(kAccentHover);
    c[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(kBorder);
}

} // namespace gui_dev::cv::Theme
