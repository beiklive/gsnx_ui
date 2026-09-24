#include "ui/Theme.h"

namespace gui_dev::Theme {

void ApplyColors() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;

    c[ImGuiCol_Text]                  = ToVec4(kTextPrimary);
    c[ImGuiCol_TextDisabled]          = ToVec4(kTextSecondary);
    c[ImGuiCol_WindowBg]              = ToVec4(IM_COL32(0x11, 0x13, 0x16, 0xFF));
    c[ImGuiCol_ChildBg]               = ToVec4(IM_COL32(0x00, 0x00, 0x00, 0x00));
    c[ImGuiCol_PopupBg]               = ToVec4(IM_COL32(0x16, 0x19, 0x1E, 0xFC));
    c[ImGuiCol_Border]                = ToVec4(IM_COL32(0x2A, 0x2F, 0x36, 0xFF));
    c[ImGuiCol_FrameBg]               = ToVec4(IM_COL32(0x1E, 0x22, 0x28, 0xFF));
    c[ImGuiCol_FrameBgHovered]        = ToVec4(IM_COL32(0x27, 0x2D, 0x35, 0xFF));
    c[ImGuiCol_FrameBgActive]         = ToVec4(IM_COL32(0x2F, 0x37, 0x41, 0xFF));
    c[ImGuiCol_TitleBg]               = ToVec4(IM_COL32(0x11, 0x13, 0x16, 0xFF));
    c[ImGuiCol_TitleBgActive]         = ToVec4(IM_COL32(0x1A, 0x1D, 0x22, 0xFF));
    c[ImGuiCol_MenuBarBg]             = ToVec4(IM_COL32(0x16, 0x19, 0x1E, 0xFF));
    c[ImGuiCol_ScrollbarBg]           = ToVec4(IM_COL32(0x00, 0x00, 0x00, 0x00));
    c[ImGuiCol_ScrollbarGrab]         = ToVec4(IM_COL32(0x33, 0x39, 0x42, 0xFF));
    c[ImGuiCol_ScrollbarGrabHovered]  = ToVec4(IM_COL32(0x41, 0x49, 0x54, 0xFF));
    c[ImGuiCol_ScrollbarGrabActive]   = ToVec4(kAccentDim);
    c[ImGuiCol_CheckMark]             = ToVec4(kAccent);
    c[ImGuiCol_SliderGrab]            = ToVec4(kAccent);
    c[ImGuiCol_SliderGrabActive]      = ToVec4(kAccent);
    c[ImGuiCol_Button]                = ToVec4(IM_COL32(0x22, 0x27, 0x2E, 0xFF));
    c[ImGuiCol_ButtonHovered]         = ToVec4(kAccentDim);
    c[ImGuiCol_ButtonActive]          = ToVec4(kAccent);
    c[ImGuiCol_Header]                = ToVec4(IM_COL32(0x24, 0x2A, 0x32, 0xFF));
    c[ImGuiCol_HeaderHovered]         = ToVec4(IM_COL32(0x2E, 0x37, 0x42, 0xFF));
    c[ImGuiCol_HeaderActive]          = ToVec4(kAccentDim);
    c[ImGuiCol_Separator]             = ToVec4(IM_COL32(0x2A, 0x2F, 0x36, 0xFF));
    c[ImGuiCol_SeparatorHovered]      = ToVec4(kAccentDim);
    c[ImGuiCol_SeparatorActive]       = ToVec4(kAccent);
    c[ImGuiCol_ResizeGrip]            = ToVec4(IM_COL32(0x2A, 0x2F, 0x36, 0xFF));
    c[ImGuiCol_ResizeGripHovered]     = ToVec4(kAccentDim);
    c[ImGuiCol_ResizeGripActive]      = ToVec4(kAccent);
    c[ImGuiCol_Tab]                   = ToVec4(IM_COL32(0x1A, 0x1D, 0x22, 0xFF));
    c[ImGuiCol_TabHovered]            = ToVec4(kAccentDim);
    c[ImGuiCol_TabSelected]           = ToVec4(IM_COL32(0x25, 0x2C, 0x36, 0xFF));
    c[ImGuiCol_TabDimmed]             = ToVec4(IM_COL32(0x16, 0x19, 0x1E, 0xFF));
    c[ImGuiCol_TabDimmedSelected]     = ToVec4(IM_COL32(0x1E, 0x23, 0x2A, 0xFF));
    c[ImGuiCol_PlotLines]             = ToVec4(kTextSecondary);
    c[ImGuiCol_PlotLinesHovered]      = ToVec4(kAccent);
    c[ImGuiCol_PlotHistogram]         = ToVec4(kAccent);
    c[ImGuiCol_PlotHistogramHovered]  = ToVec4(kAccent);
    c[ImGuiCol_TableHeaderBg]         = ToVec4(IM_COL32(0x1A, 0x1D, 0x22, 0xFF));
    c[ImGuiCol_TableBorderStrong]     = ToVec4(IM_COL32(0x2A, 0x2F, 0x36, 0xFF));
    c[ImGuiCol_TableBorderLight]      = ToVec4(IM_COL32(0x20, 0x24, 0x2A, 0xFF));
    c[ImGuiCol_TableRowBg]            = ToVec4(IM_COL32(0x00, 0x00, 0x00, 0x00));
    c[ImGuiCol_TableRowBgAlt]         = ToVec4(IM_COL32(0xFF, 0xFF, 0xFF, 0x06));
    c[ImGuiCol_TextSelectedBg]        = ToVec4(IM_COL32(0x4F, 0xA3, 0xFF, 0x66));
    c[ImGuiCol_NavCursor]             = ToVec4(kAccent);
    c[ImGuiCol_ModalWindowDimBg]      = ToVec4(IM_COL32(0x00, 0x00, 0x00, 0x99));
}

void Apply() {
    ImGuiStyle& s = ImGui::GetStyle();

    ApplyColors();

    s.WindowPadding     = ImVec2(kGapLarge, kGapLarge);
    s.FramePadding      = ImVec2(kGap, kGapSmall);
    s.CellPadding       = ImVec2(kGapSmall, kGapSmall);
    s.ItemSpacing       = ImVec2(kGap, kGapSmall + 2.0f);
    s.ItemInnerSpacing  = ImVec2(kGapSmall, kGapSmall);
    s.ScrollbarSize     = 12.0f;
    s.GrabMinSize       = 12.0f;

    s.WindowBorderSize  = 0.0f;
    s.ChildBorderSize   = 0.0f;
    s.PopupBorderSize   = 1.0f;
    s.FrameBorderSize   = 0.0f;

    s.WindowRounding    = 0.0f;
    s.ChildRounding     = kPanelRounding;
    s.FrameRounding     = 4.0f;
    s.PopupRounding     = kPanelRounding;
    s.ScrollbarRounding = 8.0f;
    s.GrabRounding      = 4.0f;
    s.TabRounding       = 4.0f;

    // Switch 是手柄导航，不允许鼠标悬浮式交互成为唯一手段。
    s.WindowMenuButtonPosition = ImGuiDir_None;
}

} // namespace gui_dev::Theme
