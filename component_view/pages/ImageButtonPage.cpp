#include "component_view/pages/ControlPage.h"

#include <cstdio>

#include "component_view/Global.h"
#include "component_view/components/Box.h"
#include "component_view/components/ImageButton.h"
#include "component_view/components/Label.h"
#include "component_view/pages/PageHelpers.h"
#include "ui/Icons.h"
#include "ui/Texture.h"
#include "ui/UiContext.h"

namespace gui_dev::cv {
namespace {
TextureRef g_cover_texture;
}

void ImageButtonPage::Build(Widget* host, UiContext& ui) {
    if (!g_cover_texture.Valid()) {
        g_cover_texture = TextureRef(ui.GetBackend(), "img/border_gradient.png");
    }
    const ImVec2 native = g_cover_texture.Size();

    struct Cover {
        const char* title;
        Icons::Material icon;
        ImU32 tint;
        const char* badge;
    };
    const Cover covers[] = {
        {"Super Mario World", Icons::Material::Games, Theme::kAccent, "NEW"},
        {"Zelda · Link to the Past", Icons::Material::Memory, Theme::kTeal, ""},
        {"Metroid Fusion", Icons::Material::Storage, Theme::kOrange, "2P"},
        {"Kirby's Dream Land", Icons::Material::Favorite, Theme::kPurple, "★"},
    };

    Box* row = helpers::Row(host, 18.0f);
    row->SetSize(0.0f, 196.0f);

    covers_.clear();
    for (const Cover& cover : covers) {
        ImageButton* button = row->Emplace<ImageButton>(
            g_cover_texture.Valid() ? g_cover_texture.ImGuiRef() : ImTextureRef(), native);
        button->SetName(std::string("cover:") + cover.title);
        button->SetSize(152.9f, 149.8f);
        button->corner_radius = Theme::kRadiusLarge;
        button->fit = ImageButton::Fit::Stretch;
        // 贴图是 512x4 的渐变条：UV 在竖直方向重复，铺成彩色条带
        button->SetUV(ImVec2(0.0f, 0.0f), ImVec2(1.0f, 44.0f));
        button->tint_normal = Theme::Alpha(cover.tint, 0.92f);
        button->tint_selected = cover.tint;
        button->SetPlaceholder(Theme::kBgWidget, Icons::Glyph(cover.icon));
        button->SetCaption(cover.title);
        if (cover.badge[0] != '\0') {
            button->SetBadge(cover.badge);
        }
        connect(button, &ImageButton::clicked, this, [this, name = std::string(cover.title)] {
            selected_ = 0;
            for (std::size_t i = 0; i < covers_.size(); ++i) {
                if (covers_[i]->name == "cover:" + name) {
                    selected_ = static_cast<int>(i);
                }
            }
        });
        covers_.push_back(button);
    }

    Box* footer = helpers::Row(host, 16.0f);
    footer->SetSize(0.0f, 40.0f);
    footer->Emplace<Label>("A 选中 · 焦点时放大 + 描边动画 · 角标可显示 NEW / 2P / ★", Theme::kFontSmall,
                           Theme::kTextMuted);
    status_ = footer->AddLabel("", Theme::kFontSmall, Theme::kTeal);
}

void ImageButtonPage::OnUpdate(float dt) {
    (void)dt;
    for (std::size_t i = 0; i < covers_.size(); ++i) {
        covers_[i]->selected = (static_cast<int>(i) == selected_);
    }
    if (status_ != nullptr && !covers_.empty()) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "已选中 %d/%d：%s", selected_ + 1, static_cast<int>(covers_.size()),
                      covers_[static_cast<std::size_t>(selected_)]->caption.c_str());
        status_->text = buffer;
    }
}

std::vector<std::pair<Icons::Button, std::string>> ImageButtonPage::Navigation() const {
    return {{Icons::Button::Left, "切换封面"},
            {Icons::Button::A, "选中"},
            {Icons::Button::Right, "切换封面"},
            {Icons::Button::B, "返回标签"}};
}

void ImageButtonPage::FillProperties(std::vector<PropSection>& out) const {
    PushSection(out, "Images", {
                               Row("Normal Image", "border_gradient (512x4)"),
                               Row("Focus/Pressed/Selected", "可分别指定贴图"),
                               Row("Disabled Image", "可分别指定贴图"),
                               Row("Fit", "cover"),
                           });
    PushSection(out, "Overlay", {
                                Row("Caption", "封面标题（底部渐变条）"),
                                Row("Badge", "NEW / 2P / ★"),
                                Row("Selected 标记", "左侧强调色带", true),
                            });
    PushSection(out, "Focus", {
                               Row("Focus Scale", "1.06", true),
                               Row("Press Scale", "0.96"),
                               Row("Focus Border", "2px 强调色 + 描边动画", true),
                               Row("Current", std::to_string(selected_ + 1), true),
                           });
    PushSection(out, "Navigation", {
                                     Row("← →", "在封面之间移动"),
                                     Row("A", "选中当前封面"),
                                     Row("B", "回到左侧标签列"),
                                 });
}

} // namespace gui_dev::cv
