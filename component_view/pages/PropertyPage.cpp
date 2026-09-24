#include "component_view/pages/PropertyPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/Button.h"
#include "component_view/components/Label.h"
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

constexpr float kColumnWidth = 600.0f;
constexpr float kColumnGap = 32.0f;
constexpr float kRightColumnX = kColumnWidth + kColumnGap;

} // namespace

Box* PropertyPage::Section(const char* title, const ImVec2& position, const ImVec2& size) {
    Box* section = Root().Emplace<Box>(std::string("prop_section:") + title);
    section->Surface();
    section->SetPosition(position.x, position.y);
    section->SetSize(size.x, size.y);
    section->layout = LayoutMode::Vertical;
    section->gap = ImVec2(0.0f, 10.0f);
    section->padding = EdgeInsets::All(16.0f);
    section->align_x = Align::Stretch;
    section->AddLabel(title, Theme::kFontHeader, Theme::kTextBright);
    return section;
}

// 居中文案的方块，用来演示 position / size / 圆角 / 边框 / 阴影。
Box* PropertyPage::CenteredBox(Widget* parent, const char* caption, const ImVec2& size, ImU32 background,
                               ImU32 border_color) {
    Box* box = parent->Emplace<Box>(std::string("demo:") + caption);
    box->SetSize(size.x, size.y);
    box->SetBackground(background);
    box->SetBorder(1.0f, border_color);
    box->SetRadius(Theme::kRadius);
    box->layout = LayoutMode::Vertical;
    box->align_x = Align::Center;
    box->align_y = Align::Center;
    box->padding = EdgeInsets::All(6.0f);
    box->AddLabel(caption, Theme::kFontTiny, Theme::kTextPrimary)
        ->SetAlign(TextAlign::Center, VerticalAlign::Middle);
    return box;
}

void PropertyPage::OnBuild() {
    // ---- 页头 --------------------------------------------------------------
    Root().Emplace<Label>("component_view · 属性演示", Theme::kFontTitle, Theme::kTextBright)->SetPosition(0.0f, 0.0f);
    Root()
        .Emplace<Label>("Widget 基类属性：position / anchor / pivot / size / min_size / corner_radius / border / "
                        "shadow / z_order / opacity",
                        Theme::kFontSmall, Theme::kTextMuted)
        ->SetPosition(0.0f, 46.0f);

    // ---- anchor / pivot ----------------------------------------------------
    Box* anchor_section = Section("位置 · anchor / pivot", ImVec2(0.0f, 88.0f), ImVec2(kColumnWidth, 250.0f));
    Box* canvas = anchor_section->Emplace<Box>("anchor_canvas");
    canvas->SetSize(0.0f, 170.0f);
    canvas->SetBackground(Theme::kBgEditor);
    canvas->SetBorder(1.0f, Theme::kBorderStrong);
    canvas->SetRadius(Theme::kRadiusSmall);
    canvas->padding = EdgeInsets::All(0.0f);

    Box* top_left = CenteredBox(canvas, "anchor 0,0", ImVec2(150.0f, 40.0f), Theme::kSelection, Theme::kBorder);
    top_left->SetAnchor(0.0f, 0.0f);
    top_left->SetPosition(10.0f, 10.0f);

    Box* middle =
        CenteredBox(canvas, "anchor .5/.5 + pivot .5/.5", ImVec2(220.0f, 44.0f), Theme::kButton, Theme::kAccent);
    middle->SetAnchor(0.5f, 0.5f);
    middle->SetPivot(0.5f, 0.5f);

    Box* bottom_right = CenteredBox(canvas, "anchor 1,1 + pivot 1,1", ImVec2(180.0f, 40.0f),
                                    Theme::Alpha(Theme::kTeal, 0.9f), Theme::kTeal);
    bottom_right->SetAnchor(1.0f, 1.0f);
    bottom_right->SetPivot(1.0f, 1.0f);

    // ---- size / 自适应 -----------------------------------------------------
    Box* size_section = Section("尺寸 · 自适应与固定", ImVec2(kRightColumnX, 88.0f), ImVec2(kColumnWidth, 250.0f));
    Box* size_row = size_section->Emplace<Box>("size_row");
    size_row->layout = LayoutMode::Horizontal;
    size_row->gap = ImVec2(14.0f, 0.0f);
    size_row->align_y = Align::Center;
    size_row->size.y = 68.0f;

    Box* auto_box = size_row->Emplace<Box>("size_auto");
    auto_box->SetBackground(Theme::kBgWidget);
    auto_box->SetBorder(1.0f, Theme::kAccent);
    auto_box->SetRadius(Theme::kRadiusSmall);
    auto_box->layout = LayoutMode::Horizontal;
    auto_box->padding = EdgeInsets::Symmetric(14.0f, 12.0f);
    auto_box->AddLabel("size 0 → 由内容撑开", Theme::kFontSmall, Theme::kTextPrimary);

    CenteredBox(size_row, "固定 180x64", ImVec2(180.0f, 64.0f), Theme::kBgWidget, Theme::kBorder)
        ->SetRadius(Theme::kRadius);

    Box* min_box = size_row->Emplace<Box>("size_min");
    min_box->SetMinSize(200.0f, 64.0f);
    min_box->SetBackground(Theme::kBgWidget);
    min_box->SetBorder(1.0f, Theme::kBorder);
    min_box->SetRadius(Theme::kRadius);
    min_box->layout = LayoutMode::Vertical;
    min_box->align_x = Align::Center;
    min_box->align_y = Align::Center;
    min_box->padding = EdgeInsets::All(8.0f);
    min_box->AddLabel("min_size 200x64", Theme::kFontTiny, Theme::kTextPrimary)
        ->SetAlign(TextAlign::Center, VerticalAlign::Middle);

    Label* wrap = size_section->Emplace<Label>(
        "wrap_width = 540：文字超出宽度后自动折行，组件高度由内容测量得出，父容器不需要关心行数。", Theme::kFontSmall,
        Theme::kTextMuted);
    wrap->wrap_width = 540.0f;
    wrap->single_line = false;
    wrap->SetSize(540.0f, 0.0f);

    // ---- 圆角 / 边框 / 阴影 -------------------------------------------------
    Box* style_section = Section("圆角 / 边框 / 阴影 · 四角可以单独设置", ImVec2(0.0f, 348.0f),
                                 ImVec2(kColumnWidth, 190.0f));
    Box* style_row = style_section->Emplace<Box>("style_row");
    style_row->layout = LayoutMode::Horizontal;
    style_row->gap = ImVec2(10.0f, 0.0f);
    style_row->align_y = Align::Center;
    style_row->size.y = 110.0f;
    for (int i = 0; i < 4; ++i) {
        Box* item = style_row->Emplace<Box>("style_item");
        item->SetSize(128.0f, 110.0f);
        item->SetBackground(Theme::kBgWidget);
        item->layout = LayoutMode::Vertical;
        item->align_x = Align::Center;
        item->align_y = Align::Center;
        item->padding = EdgeInsets::All(8.0f);
        const char* caption = nullptr;
        switch (i) {
        case 0:
            caption = "radius 0";
            item->SetRadius(0.0f);
            item->SetBorder(1.0f, Theme::kBorder);
            break;
        case 1:
            caption = "radius 14";
            item->SetRadius(14.0f);
            item->SetBorder(1.0f, Theme::kBorder);
            break;
        case 2:
            caption = "border 3";
            item->SetRadius(Theme::kRadius);
            item->SetBorder(3.0f, Theme::kOrange);
            break;
        default:
            caption = "shadow blur 30";
            item->SetRadius(Theme::kRadius);
            item->SetBorder(1.0f, Theme::kBorder);
            item->shadow = ShadowStyle::Soft(30.0f);
            item->shadow.offset = ImVec2(0.0f, 8.0f);
            item->shadow.color = Theme::kShadow;
            break;
        }
        item->SetName(std::string("style_item_") + std::to_string(i));
        item->AddLabel(caption, Theme::kFontTiny, Theme::kTextPrimary)
            ->SetAlign(TextAlign::Center, VerticalAlign::Middle);
    }

    // ---- z_order -----------------------------------------------------------
    Box* z_section = Section("层叠 · z_order（插入顺序 + 显式层级）", ImVec2(kRightColumnX, 348.0f),
                             ImVec2(kColumnWidth, 190.0f));
    Box* z_canvas = z_section->Emplace<Box>("z_canvas");
    z_canvas->SetSize(0.0f, 110.0f);
    z_canvas->SetBackground(Theme::kBgEditor);
    z_canvas->SetBorder(1.0f, Theme::kBorderStrong);
    z_canvas->SetRadius(Theme::kRadiusSmall);
    z_canvas->padding = EdgeInsets::All(0.0f);

    struct Layer {
        const char* caption;
        ImU32 color;
        int z;
        ImVec2 position;
    };
    // 故意把 z 最大的先插入：绘制与命中的顺序只由 z_order 决定。
    const Layer layers[] = {
        {"z_order 2", Theme::Alpha(Theme::kPurple, 0.95f), 2, ImVec2(300.0f, 40.0f)},
        {"z_order 0", Theme::Alpha(Theme::kAccent, 0.95f), 0, ImVec2(40.0f, 20.0f)},
        {"z_order 1", Theme::Alpha(Theme::kTeal, 0.95f), 1, ImVec2(170.0f, 30.0f)},
    };
    for (const Layer& layer : layers) {
        Box* box = CenteredBox(z_canvas, layer.caption, ImVec2(180.0f, 54.0f), layer.color, Theme::kBorderStrong);
        box->SetPosition(layer.position.x, layer.position.y);
        box->SetZOrder(layer.z);
    }

    // ---- 交互 --------------------------------------------------------------
    Box* action_row = Root().Emplace<Box>("action_row");
    action_row->Surface();
    action_row->SetPosition(0.0f, 548.0f);
    action_row->SetSize(2.0f * kColumnWidth + kColumnGap, 78.0f);
    action_row->layout = LayoutMode::Horizontal;
    action_row->gap = ImVec2(16.0f, 0.0f);
    action_row->align_y = Align::Center;

    action_row->AddLabel("交互 · 事件回调", Theme::kFontHeader, Theme::kTextBright);

    auto add_button = [this, action_row](const char* name, const char* text, int variant) {
        Button* button = action_row->Emplace<Button>(text);
        button->SetName(name);
        button->FitContent(18.0f, 44.0f);
        if (variant == 1) {
            button->Secondary();
        } else if (variant == 2) {
            button->Ghost();
        }
        return button;
    };

    Button* plus = add_button("prop_plus", "点我 +1", 0);
    plus->SetIcon(Icons::Glyph(Icons::Button::A));
    plus->on_click = [this](Widget&) { ++count_; };

    Button* reset = add_button("prop_reset", "重置", 1);
    reset->on_click = [this](Widget&) { count_ = 0; };

    // 可聚焦的 Box 也能参与交互（不只有 Button）
    Box* card = action_row->Emplace<Box>("prop_card");
    card->SetSize(220.0f, 44.0f);
    card->SetBackground(Theme::kBgWidget);
    card->SetBorder(1.0f, Theme::kBorder);
    card->SetRadius(Theme::kRadiusSmall);
    card->layout = LayoutMode::Vertical;
    card->align_x = Align::Center;
    card->align_y = Align::Center;
    card->SetFocusable(true);
    card->AddLabel("可聚焦的 Box", Theme::kFontSmall, Theme::kTextPrimary)
        ->SetAlign(TextAlign::Center, VerticalAlign::Middle);
    card->on_click = [this](Widget&) { ++count_; };

    status_label_ = action_row->AddLabel("count = 0", Theme::kFontSmall, Theme::kTextMuted);

    AddHint(Icons::Button::A, "确定");
    AddHint(Icons::Button::B, "返回");
    AddHint(Icons::Button::Left, "移动焦点");
    AddHint(Icons::Button::L, "上一页");
    AddHint(Icons::Button::R, "下一页");
}

void PropertyPage::OnUpdate(float dt) {
    (void)dt;
    if (status_label_ == nullptr) {
        return;
    }
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "count = %d", count_);
    status_label_->text = buffer;
    status_label_->color = count_ > 0 ? Theme::kTeal : Theme::kTextMuted;
}

} // namespace gui_dev::cv
