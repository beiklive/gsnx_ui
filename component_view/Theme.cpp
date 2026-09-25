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

void SetMode(ThemeMode mode) {
    theme_mode = mode;
    if (mode == ThemeMode::Light) {
        // VSCode Light+：白底 + 浅灰控件 + 深色文字
        kBgEditor = rgb(255, 255, 255);
        kBgSideBar = rgb(243, 243, 245);
        kBgPanel = rgb(243, 243, 245);
        kBgActivity = rgb(236, 236, 238);
        kBgWidget = rgb(232, 232, 235);
        kBgWidgetHi = rgb(220, 220, 224);
        kBgInput = rgb(255, 255, 255);
        kBorder = rgb(215, 215, 220);
        kBorderStrong = rgb(160, 160, 166);
        kTextPrimary = rgb(31, 31, 35);
        kTextBright = rgb(0, 0, 0);
        kTextMuted = rgb(106, 106, 112);
        kTextDisabled = rgb(150, 150, 156);
        kAccent = rgb(0, 95, 184);
        kAccentHover = rgb(16, 110, 190);
        kButton = rgb(0, 95, 184);
        kButtonActive = rgb(0, 75, 150);
        kSelection = rgb(173, 214, 255);
        kControlBorder = rgb(203, 203, 209);
        kSwitchOff = rgb(190, 190, 196);
        kSwitchKnob = rgb(255, 255, 255);
        kCapsuleFill = rgb(0, 0, 0);   // 浅色底上改成黑色半透明
        kCapsuleStroke = rgb(0, 0, 0);
        kCapsuleShadow = rgba(0, 0, 0, 48);
    } else {
        // VSCode Dark+：深灰底 + 深色控件 + 浅色文字（组件约定的灰白边框也在这里）
        kBgEditor = rgb(30, 30, 30);
        kBgSideBar = rgb(37, 37, 38);
        kBgPanel = rgb(37, 37, 38);
        kBgActivity = rgb(51, 51, 51);
        kBgWidget = rgb(45, 45, 48);
        kBgWidgetHi = rgb(55, 55, 58);
        kBgInput = rgb(60, 60, 60);
        kBorder = rgb(60, 60, 60);
        kBorderStrong = rgb(84, 84, 88);
        kTextPrimary = rgb(212, 212, 212);
        kTextBright = rgb(255, 255, 255);
        kTextMuted = rgb(133, 133, 133);
        kTextDisabled = rgb(106, 106, 106);
        kAccent = rgb(0, 122, 204);
        kAccentHover = rgb(17, 119, 187);
        kButton = rgb(14, 99, 156);
        kButtonActive = rgb(10, 75, 119);
        kSelection = rgb(38, 79, 120);
        kControlBorder = rgb(190, 190, 195);
        kSwitchOff = rgb(88, 88, 92);
        kSwitchKnob = rgb(255, 255, 255);
        kCapsuleFill = rgb(255, 255, 255); // 白色半透明（GBAStation 那套）
        kCapsuleStroke = rgb(255, 255, 255);
        kCapsuleShadow = rgba(0, 0, 0, 72);
    }
    kTrackFill = kAccent;
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
    c[ImGuiCol_Text] = kTextPrimary;
    c[ImGuiCol_TextDisabled] = kTextMuted;
    c[ImGuiCol_WindowBg] = kBgEditor;
    c[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_PopupBg] = kBgSideBar;
    c[ImGuiCol_Border] = kBorder;
    c[ImGuiCol_FrameBg] = kBgWidget;
    c[ImGuiCol_FrameBgHovered] = kBgWidgetHi;
    c[ImGuiCol_FrameBgActive] = kBgInput;
    c[ImGuiCol_TitleBg] = kBgActivity;
    c[ImGuiCol_TitleBgActive] = kBgActivity;
    c[ImGuiCol_Button] = kButton;
    c[ImGuiCol_ButtonHovered] = kAccentHover;
    c[ImGuiCol_ButtonActive] = kButtonActive;
    c[ImGuiCol_Header] = kSelection;
    c[ImGuiCol_HeaderHovered] = kAccent;
    c[ImGuiCol_HeaderActive] = kAccentHover;
    c[ImGuiCol_CheckMark] = kAccent;
    c[ImGuiCol_SliderGrab] = kAccent;
    c[ImGuiCol_SliderGrabActive] = kAccentHover;
    c[ImGuiCol_Separator] = kBorder;
}

} // namespace gui_dev::cv::Theme
