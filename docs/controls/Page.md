# Page（页面基类）

来源：`component_view/pages/Page.h`（实现 `component_view/pages/Page.cpp`）
一句话：一个页面 = 一棵挂在 `Root()` 下的组件树（根 Box 铺满画布）+ 页级输入/更新/绘制，宿主每帧只调 `Update(dt)` 与 `Render()`。框架总览见 [component-view.md](../component-view.md)。

## 最小示例

```cpp
class SettingsPage : public Page {
public:
    const char* Title() const override { return "settings"; }

    void OnBuild() override {
        auto* button = Root().Emplace<TextButton>("弹一个 Toast");
        connect(button, &Widget::clicked, this, [this] { Toasts().ShowSuccess("按钮被按下"); });
    }

    void OnUpdate(float dt) override {
        (void)dt;
        button_->position = ImVec2(20.0f, 20.0f);
    }

private:
    TextButton* button_ = nullptr;
};

// 宿主：OnFrame 里每帧两调（examples/min_demo/main.cpp）
page_->Bind(ui);
page_->Update(dt);
page_->Render();
```

## API

| 成员 / 方法 | 说明 |
|---|---|
| `virtual const char* Title() const = 0` | 页面标题，纯虚，子类必须实现 |
| `virtual void OnBuild()` | 首次 `Update()` 时调用一次，往 `Root()` 里塞组件 |
| `virtual void OnInput()` | 命中测试 / 焦点导航 / 树更新之后，`OnUpdate` 之前 |
| `virtual void OnUpdate(float dt)` | 每帧状态更新，在整棵树的 `UpdateTree` 之后 |
| `virtual void OnOverlay(ImDrawList* dl)` | 页面叠加层绘制，在 `root_->DrawTree` 之后、弹窗之前 |
| `void Bind(UiContext& ui)` | 绑定宿主上下文；未绑定则 `Update` 在 `OnBuild` 后直接返回 |
| `void Update(float dt)` | 宿主每帧调用：布局 → 命中 → 导航 → 树更新 → `OnInput` → `OnUpdate` |
| `void Render()` | 宿主每帧调用：按 UILayer 从下往上画 |
| `Box& Root()` | 页面根节点，首次调用时创建，铺满整个画布 |
| `ToastManager& Toasts()` | Toast 通知，业务只调 `ShowSuccess` / `ShowError` / `ShowInfo` |
| `PopupManager& Popups()` | 弹窗 / 模态框栈（`Show*` / `CloseTop` / `CloseAll`…） |
| `FocusManager& Focus()` | 焦点管理（弹窗打开 / 关闭时自动用） |
| `void RefreshTheme()` | 切主题后调一次：重置根节点装饰 + 整树重新取色 |
| `UiContext& ui() const` | 取宿主上下文（`Bind` 之后有效） |

## 信号 / 事件

| 名称 | 参数 | 触发时机 |
|---|---|---|
| （无） | | `Page` 自身不声明信号；页面逻辑通过控件信号 `connect(...)` 或 `Toasts()` / `Popups()` 驱动 |

## 时序 / 约束

`Update(dt)` 内部顺序（`component_view/pages/Page.cpp`）：

| 顺序 | 动作 |
|---|---|
| 1 | `built_` 为 false 时先 `OnBuild()`（只一次） |
| 2 | `root_` 或 `ui_` 为空则 return |
| 3 | 把 `&focus_` / `&popups_` 挂到 `Global::focus_manager` / `Global::popup_manager` |
| 4 | 根节点 `position=(0,0)`、`size=Global::canvas_size`，`LayoutTree(canvas_pos, canvas_size)` |
| 5 | `popups_.Layout()`（栈内每层都摆好） |
| 6 | `root_->CollectFocusables(focusables_)` |
| 7 | 命中测试：`popups_.HitTest(mouse)`，无命中且 `!popups_.BlocksBackground()` 时才测 `root_->HitTest` |
| 8 | 拖动滚动：按下时记 `pointer_drag_host`，超过 `kDragThreshold`（8px）才开始滚 |
| 9 | `popups_.CollectFocusables(...)` → `Global::NavigateFocus(nav_focusables_)` |
| 10 | `root_->UpdateTree(dt)`、`popups_.UpdateTree(dt)` |
| 11 | 释放帧清 `pointer_drag_host` / `pointer_dragging`（让控件先看到 dragging） |
| 12 | `popups_.HandleDismiss()`（在控件拿到按键之后） |
| 13 | 焦点变化时自动滚动：`root_->EnsureVisible` 失败再 `popups_.EnsureVisible` |
| 14 | `OnInput()` → `OnUpdate(dt)` |
| 15 | `toasts_.Update(dt)` → `popups_.Advance(dt)` |

`Render()` 顺序：`Background` 底色 → `root_->DrawTree` → `OnOverlay` → `popups_.Draw` → `focus_ring_.Draw(dl, Global::focused)` → `toasts_.Draw`。

## 注意点

- `Root()` 创建的根 Box 是透明的：`background=0`、`border.width=0`、`shadow.enabled=false`、`corner_radius=0`、`padding=0`、`layout=Free`；子节点用 `position` 自己定位。
- `RefreshTheme()` 中 `root_->RefreshThemeTree()` 会经 `Box::OnThemeChanged()` 重新打开边框/阴影，所以根节点的「不画装饰」必须在其后复位（`Page.cpp` 已处理）。
- `OnBuild` 在 `built_` 守卫下只执行一次，且发生在 `ui_` 空检查之前：`Bind()` 没调时首帧仍会 Build，随后直接 return。
- 弹窗打开时页面拿不到 hover / 点击：命中测试在 `popups_.HitTest` 这一层就结束（模态 + 遮罩拦截）。
- 焦点框画在弹窗之上、Toast 之下，见 `component_view/UILayer.h` 与 `FocusRing.md`。
- `popups_.Advance(dt)` 放在最后：关闭动画播完才真正移除，同时弹出焦点作用域并恢复焦点。
