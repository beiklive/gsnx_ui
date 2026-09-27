# Box

来源：`component_view/components/Box.h`（实现 `component_view/components/Box.cpp`）
一句话：最基础的矩形组件，也是后面所有组件的容器；既能当纯容器，也能自己拿焦点。

## 最小示例

```cpp
// 容器：不调 makeFocusable()，只负责排版 / 背景，焦点落在子节点上
Box* panel = page.Root().Emplace<Box>("panel");
panel->moveTo(40.0f, 40.0f)
    ->resize(600.0f, 0.0f)                  // 0 = 高度按内容自适应
    ->roundCorners(Global::component_style.corner_radius)
    ->fillWith(Theme::kBgWidget)
    ->outline(1.0f, Theme::U32(Theme::kBorder));

// 可聚焦控件：手柄 / 键盘选中它，A 触发 clicked（demo.cpp 的 box_ 就是这么用的）
Box* card = panel->Emplace<Box>("card");
card->resize(240.0f, 120.0f)->makeFocusable();
card->onClicked(&page, [card] { card->fillWith(card->hasFocus() ? Theme::kAccent : Theme::kBgWidget); });
```

## API

### 构造

| 成员 / 方法 | 说明 |
|---|---|
| `Box()` | 默认名 `"box"`，底色取 `Theme::kBgWidget` 并打开 `background_follows_theme`，同时套用全局约定样式 |
| `explicit Box(std::string widget_name)` | 先按默认构造再覆盖 `name` |

### 链式设置

| 成员 / 方法 | 说明 |
|---|---|
| `Box& moveTo(float x, float y)` | 位置（相对父节点内容区左上角），等价于 `SetPosition` |
| `Box& resize(float width, float height)` | 尺寸（外框尺寸），等价于 `SetSize` |
| `Box& fillWith(ImU32 color)` | 底色（打包值），并关闭主题跟随 |
| `Box& fillWith(const ImVec4& color)` | 底色（`Theme::rgba()/rgb()` 的结果），并关闭主题跟随 |
| `Box& roundCorners(float value)` | 圆角，等价于 `SetRadius` |
| `Box& outline(float width, ImU32 color)` | 边框（统一宽度 + 颜色） |
| `Box& dropShadow(float blur, ImU32 color)` | 阴影，`enabled = true`、`blur` 取传入值、`offset = (0, 4)` |
| `Box& applyComponentStyle()` | 重新套用 `ApplyComponentBoxStyle()`；改过 `Global::component_style` 后调它生效 |

### 焦点

| 成员 / 方法 | 说明 |
|---|---|
| `Box& makeFocusable(bool value = true)` | 设 `focusable`；为 true 时顺带调 `focusVisual()`，保证有可见反馈 |
| `Box& focusVisual(float scale = 1.04f, const ImVec2& translate = ImVec2(0.0f, 0.0f), float frame_offset = 3.0f, ImU32 frame_color = Theme::U32(Theme::kAccent))` | 设焦点框外扩 / 颜色、聚焦缩放与位移 |
| `Box& focusOnlySelf(bool value = true)` | 复合控件语义：自己可聚焦，但不把子节点算进焦点导航 |

### 保护 / 子类扩展点

| 成员 / 方法 | 说明 |
|---|---|
| `void OnThemeChanged() override` | 若 `background_follows_theme && background != 0` 则重取 `Theme::kBgWidget`，再 `applyComponentStyle()` 重新取边框色 / 阴影浓淡 |

Box 自身不再声明信号，`clicked` 等全部继承自 [Widget](Widget.md)。

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `clicked`（继承 `Widget`） | 无 | `makeFocusable()` 后按 A，或鼠标点击松开时 |
| `pressed` / `released` / `hoverEntered` / `hoverLeft` / `focusIn` / `focusOut` / `enabledChanged`（继承 `Widget`） | 无 | 与基类一致，见 [Widget.md](Widget.md) |

## 尺寸 / 主题约定

- 构造时不写死视觉，而是套用 `Global::component_style`（`ApplyComponentBoxStyle()`）：
  `border_width` / `border_color` / `corner_radius` / `shadow_offset` / `shadow_blur` / `shadow_color`。
- 默认底色 `Theme::kBgWidget`，`background_follows_theme = true`，所以切主题会跟着变；
  显式 `fillWith` 后该开关变 false，切主题需要自己重新设色。
- 切主题的完整流程：`Global::ApplyTheme()`（刷新 `component_style` 的边框色 / 阴影）→ 对根节点（如 `Page::Root()`）调 `RefreshThemeTree()` 递归触发 `OnThemeChanged()`。
- `focusVisual` 的默认焦点框颜色是 `Theme::U32(Theme::kAccent)`。

## 注意点

- 链式函数名不能和 `Widget` 的成员变量同名（`border` / `shadow` / `size` / `focusable` 都是成员变量），
  所以这里用 `outline` / `dropShadow` / `resize` / `makeFocusable` 这类名字；直接调 `SetBorder` / `SetShadow` / `SetSize` 也可以。
- `makeFocusable(true)` 会连带打开默认焦点视觉；只想可聚焦、不要缩放位移时，可在之后自己覆盖 `focus_scale` / `focus_translate`。
- `OnThemeChanged()` 用 `background != 0` 判断：透明节点（`background = 0`）不会在切主题时被补上底色。
