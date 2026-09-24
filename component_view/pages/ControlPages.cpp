// 控件页工厂：Tab 的顺序就是下面的顺序。
#include "component_view/pages/ControlPage.h"

#include "ui/Icons.h"

namespace gui_dev::cv {

std::vector<std::unique_ptr<ControlPage>> CreateControlPages() {
    std::vector<std::unique_ptr<ControlPage>> pages;
    pages.push_back(std::make_unique<BoxPage>());
    pages.push_back(std::make_unique<LabelPage>());
    pages.push_back(std::make_unique<ButtonPage>());
    pages.push_back(std::make_unique<ImagePage>());
    pages.push_back(std::make_unique<ImageButtonPage>());
    pages.push_back(std::make_unique<ListPage>());
    pages.push_back(std::make_unique<ScrollPage>());
    pages.push_back(std::make_unique<TabPage>());
    pages.push_back(std::make_unique<CheckboxPage>());
    pages.push_back(std::make_unique<RadioPage>());
    pages.push_back(std::make_unique<SliderPage>());
    pages.push_back(std::make_unique<ProgressPage>());
    pages.push_back(std::make_unique<InputPage>());
    pages.push_back(std::make_unique<KeyboardPage>());
    pages.push_back(std::make_unique<DialogPage>());
    pages.push_back(std::make_unique<MenuPage>());
    return pages;
}

} // namespace gui_dev::cv
