// 控件页接口：左侧 Tab 的每一项对应一个 ControlPage，
// 右侧展示区（showcase）+ 属性面板（Visual / Layout / State / Navigation）由它提供。
//
// 约定：**展示区里的控件必须真的能用手柄操作**，属性面板显示的是实时状态。
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "component_view/Types.h"
#include "component_view/Widget.h"
#include "platform/Input.h"
#include "ui/Icons.h"

namespace gui_dev {
class UiContext;
} // namespace gui_dev

namespace gui_dev::cv {

struct PropSection {
    std::string title;
    std::vector<PropRow> rows;
};

// 便捷构造：往 sections 里加一组属性
inline void PushSection(std::vector<PropSection>& out, std::string title,
                        std::initializer_list<PropRow> rows) {
    PropSection section;
    section.title = std::move(title);
    section.rows.assign(rows.begin(), rows.end());
    out.push_back(std::move(section));
}

inline PropRow Row(std::string name, std::string value, bool highlight = false) {
    PropRow row;
    row.name = std::move(name);
    row.value = std::move(value);
    row.highlight = highlight;
    return row;
}

class ControlPage {
public:
    virtual ~ControlPage() = default;

    // ---- 元信息 ------------------------------------------------------------
    virtual const char* Name() const = 0;    // Tab 标题（大写）
    virtual const char* Title() const = 0;   // 中文名
    virtual const char* Summary() const = 0; // 一句话说明
    virtual const char* Icon() const { return ""; }

    // ---- 生命周期 ----------------------------------------------------------
    // 搭 showcase：host 已经配好（垂直排列、居中、gap 16、padding 22）
    // ui 用来加载纹理（图片类控件需要），页面可以把 TextureRef 存成成员
    virtual void Build(Widget* host, UiContext& ui) = 0;
    virtual void OnUpdate(float dt) { (void)dt; }
    virtual void FillProperties(std::vector<PropSection>& out) const { (void)out; }
    // 页内按键（在焦点导航之前调用）；返回 true 表示已消费
    virtual bool OnAction(InputAction action) {
        (void)action;
        return false;
    }
    // 弹层（Dialog / 键盘弹层）挂到这里，需要自己控制 Overlay 的可见性
    virtual void BuildOverlay(Widget* overlay) { (void)overlay; }
    virtual void OnEnter() {}
    virtual void OnLeave() {}

    // 展示区容器（由 Shell 注入，页面可以拿它做焦点恢复等）
    void SetHost(Widget* host) { host_ = host; }
    Widget* host() const { return host_; }

    // 底部按键说明；默认给通用导航提示
    virtual std::vector<std::pair<Icons::Button, std::string>> Navigation() const {
        return {{Icons::Button::Up, "切换控件"}, {Icons::Button::A, "确定"}, {Icons::Button::B, "返回标签"}};
    }

private:
    Widget* host_ = nullptr;
};

// ---- 16 个控件页 ----------------------------------------------------------
class BoxPage : public ControlPage {
public:
    const char* Name() const override { return "BOX"; }
    const char* Title() const override { return "Box · 容器"; }
    const char* Summary() const override { return "布局 / 圆角 / 边框 / 阴影 / 溢出 / 焦点动画"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;

private:
    class Box* focus_card_ = nullptr;
    class Box* overflow_hidden_ = nullptr;
    float elapsed_ = 0.0f;
};

class LabelPage : public ControlPage {
public:
    const char* Name() const override { return "LABEL"; }
    const char* Title() const override { return "Label · 文本"; }
    const char* Summary() const override { return "单行 / 多行 / 换行 / 省略号 / 跑马灯 / 描边 / 焦点换文案"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;

private:
    class Label* marquee_ = nullptr;
    class Label* focus_label_ = nullptr;
    class Label* wrap_label_ = nullptr;
    float elapsed_ = 0.0f;
};

class ButtonPage : public ControlPage {
public:
    const char* Name() const override { return "BUTTON"; }
    const char* Title() const override { return "Button · 按钮"; }
    const char* Summary() const override { return "Normal / Focused / Pressed / Selected / Disabled + A/X/Y"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;

private:
    class Button* primary_ = nullptr;
    class Button* selected_ = nullptr;
    class Button* disabled_ = nullptr;
    class Label* status_ = nullptr;
    int confirm_count_ = 0;
    int aux_count_ = 0;
};

class ImagePage : public ControlPage {
public:
    const char* Name() const override { return "IMAGE"; }
    const char* Title() const override { return "Image · 图片"; }
    const char* Summary() const override { return "Contain / Cover / Stretch / UV / Tint / Flip / Rotation / 圆角"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;

private:
    class Image* rotating_ = nullptr;
    float angle_ = 0.0f;
    bool has_texture_ = false;
};

class ImageButtonPage : public ControlPage {
public:
    const char* Name() const override { return "IMAGE BUTTON"; }
    const char* Title() const override { return "ImageButton · 图片按钮"; }
    const char* Summary() const override { return "封面格：焦点放大 + 边框动画 + 角标 + 选中态"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;

private:
    std::vector<class ImageButton*> covers_;
    class Label* status_ = nullptr;
    int selected_ = 0;
};

class ListPage : public ControlPage {
public:
    const char* Name() const override { return "LIST"; }
    const char* Title() const override { return "List · 列表"; }
    const char* Summary() const override { return "垂直 / 水平 / 网格 / 循环 / 翻页 / 快速滚动 / 自动滚动"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;

private:
    class List* vertical_ = nullptr;
    class List* horizontal_ = nullptr;
    class Label* status_ = nullptr;
    int activated_ = -1;
};

class ScrollPage : public ControlPage {
public:
    const char* Name() const override { return "SCROLL"; }
    const char* Title() const override { return "Scroll · 滚动"; }
    const char* Summary() const override { return "焦点自动滚动 / 平滑 / 越界回弹 / 翻页 / 滚动条自动隐藏"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;

private:
    class ScrollBox* scroll_ = nullptr;
    class Label* status_ = nullptr;
    std::vector<class Button*> cards_;
};

class TabPage : public ControlPage {
public:
    const char* Name() const override { return "TAB"; }
    const char* Title() const override { return "Tab · 标签"; }
    const char* Summary() const override { return "水平 / 垂直 / 图标+文本 / 指示条动画 / LR 翻页"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;

private:
    class TabBar* horizontal_ = nullptr;
    class TabBar* vertical_ = nullptr;
    class Label* status_ = nullptr;
};

class CheckboxPage : public ControlPage {
public:
    const char* Name() const override { return "CHECKBOX"; }
    const char* Title() const override { return "Checkbox · 复选"; }
    const char* Summary() const override { return "A 切换 / 勾选动画 / 焦点换色 / Disabled"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;

private:
    std::vector<class Checkbox*> boxes_;
    class Checkbox* master_ = nullptr;
    class Label* status_ = nullptr;
    float elapsed_ = 0.0f;
};

class RadioPage : public ControlPage {
public:
    const char* Name() const override { return "RADIO"; }
    const char* Title() const override { return "Radio · 单选"; }
    const char* Summary() const override { return "单选组：↑↓ 移动高亮 / A 选定 / 圆点动画"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;

private:
    class RadioGroup* group_ = nullptr;
    class RadioGroup* horizontal_ = nullptr;
    class Label* status_ = nullptr;
};

class SliderPage : public ControlPage {
public:
    const char* Name() const override { return "SLIDER"; }
    const char* Title() const override { return "Slider · 滑条"; }
    const char* Summary() const override { return "←→ 微调 / L R 快调 / ZL ZR 大步 / B 取消回滚"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;

private:
    class Slider* volume_ = nullptr;
    class Slider* speed_ = nullptr;
    class Progress* meter_ = nullptr;
    class Label* status_ = nullptr;
};

class ProgressPage : public ControlPage {
public:
    const char* Name() const override { return "PROGRESS"; }
    const char* Title() const override { return "Progress · 进度"; }
    const char* Summary() const override { return "确定 / 不定态 / 水平 / 垂直 / 圆角 / 平滑动画"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;

private:
    std::vector<class Progress*> bars_;
    class Button* toggle_ = nullptr;
    class Label* status_ = nullptr;
    float value_ = 0.0f;
    float elapsed_ = 0.0f;
    bool running_ = true;
};

class InputPage : public ControlPage {
public:
    const char* Name() const override { return "INPUT"; }
    const char* Title() const override { return "Input · 输入框"; }
    const char* Summary() const override { return "A 打开虚拟键盘（不调用系统键盘）/ X 清空 / Y 退格"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;
    void BuildOverlay(Widget* overlay) override;

private:
    void OpenKeyboard(class InputField* field);
    class InputField* name_ = nullptr;
    class InputField* password_ = nullptr;
    class InputField* readonly_ = nullptr;
    class InputField* target_ = nullptr;
    class VirtualKeyboard* keyboard_ = nullptr;
    class Box* overlay_ = nullptr;
    class Label* status_ = nullptr;
};

class KeyboardPage : public ControlPage {
public:
    const char* Name() const override { return "KEYBOARD"; }
    const char* Title() const override { return "Keyboard · 虚拟键盘"; }
    const char* Summary() const override { return "自绘键盘：QWERTY / 符号 / 数字 / Shift / 光标 / 选区"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;
    bool OnAction(InputAction action) override;

private:
    class VirtualKeyboard* keyboard_ = nullptr;
    class Label* log_ = nullptr;
    std::string last_action_;
};

class DialogPage : public ControlPage {
public:
    const char* Name() const override { return "DIALOG"; }
    const char* Title() const override { return "Dialog · 对话框"; }
    const char* Summary() const override { return "模态 + 遮罩 + Focus Trap + 进出动画 + B 取消"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;
    void BuildOverlay(Widget* overlay) override;
    bool OnAction(InputAction action) override;

private:
    class Dialog* dialog_ = nullptr;
    class Box* overlay_ = nullptr;
    class Label* status_ = nullptr;
    std::string last_result_ = "未打开";
    int open_count_ = 0;
};

class MenuPage : public ControlPage {
public:
    const char* Name() const override { return "MENU"; }
    const char* Title() const override { return "Menu · 菜单"; }
    const char* Summary() const override { return "垂直菜单 / 子菜单 / 分隔符 / 快捷键提示 / 分区翻页"; }
    void Build(Widget* host, UiContext& ui) override;
    void OnUpdate(float dt) override;
    void FillProperties(std::vector<PropSection>& out) const override;
    std::vector<std::pair<Icons::Button, std::string>> Navigation() const override;

private:
    class Menu* menu_ = nullptr;
    class Label* status_ = nullptr;
    std::string last_action_ = "-";
    int activate_count_ = 0;
};

// 工厂：按 Tab 顺序返回 16 个控件页
std::vector<std::unique_ptr<ControlPage>> CreateControlPages();

} // namespace gui_dev::cv
