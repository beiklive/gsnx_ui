# FunctionBar

来源：`component_view/components/FunctionBar.h`（实现 `component_view/components/FunctionBar.cpp`）
一句话：功能按钮行 —— 一条胶囊容器里等距排 N 个无边框圆形图标按钮，聚焦项的名字显示在容器下方居中。

## 最小示例

```cpp
#include "component_view/components/FunctionBar.h"

FunctionBar* bar = panel.Emplace<FunctionBar>();
bar->AddItem(Icons::Glyph(Icons::Material::Games), "游戏库", [this] { OpenLibrary(); });
bar->AddItem(Icons::Glyph(Icons::Material::Settings), "设置"); // on_activate 可空
connect(bar, &FunctionBar::activated, this, [](int i) { /* 统一分发 / 埋点 */ });
```

## API

### 数据结构 Item

| 字段 | 类型 / 默认 | 说明 |
|---|---|---|
| `icon` | `std::string` | Material 字形 |
| `label` | `std::string` | 名字，只在聚焦时显示在胶囊下方 |
| `on_activate` | `std::function<void()>` | 点 / A 时执行；可空，留空就只发 `activated(index)` |

### 公开方法

| 方法 | 说明 |
|---|---|
| `FunctionBar()` | 构造：`applyComponentStyle()`，底色 `Theme::kBgWidget`，`layout = LayoutMode::Free` |
| `FunctionBar& SetItems(std::vector<Item> items)` | 重建全部项（内部 `Rebuild()`） |
| `FunctionBar& AddItem(std::string icon, std::string label, std::function<void()> on_activate = {})` | 追加一项并重建 |
| `FunctionBar& SetStyle(const Style& value)` | 换尺寸参数；同步更新已有按钮的 `setSide(item_size)` 与容器圆角 |
| `IconButton* itemAt(int index) const` | 取第 index 个按钮（越界返回 `nullptr`） |
| `int count() const` | 按钮数 |
| `int focusedIndex() const` | 当前持有焦点的项下标，没有则 `-1` |
| `float CapsuleWidth() const` | 胶囊宽度：`min(自然宽度, rect.Width())`，未布局时返回自然宽度 |

### Style

`style` 成员是**私有**的，只能通过 `SetStyle()` 整体替换。全部字段如下：

| 字段 | 默认值 | 说明 |
|---|---|---|
| `item_size` | `44.0f` | 圆形按钮直径 |
| `capsule_padding` | `10.0f` | 胶囊上下内边距 |
| `edge_padding` | `18.0f` | 胶囊左右内边距（半圆两端留白） |
| `gap` | `22.0f` | 相邻按钮最小间距（容器更宽时自动摊开） |
| `label_gap` | `6.0f` | 名称与胶囊底边的间距 |
| `label_size` | `Theme::kFontSmall`（14） | 名称字号 |
| `label_height` | `24.0f` | 名称行高度：常驻占位，聚焦时不会把布局顶动 |
| `label_fade` | `12.0f` | 名称淡入淡出速度（1/s） |
| `radius` | `-1.0f` | `< 0` = 胶囊（高度的一半）；`>= 0` 用普通圆角 |

### 子类扩展点

| 方法 | 说明 |
|---|---|
| `ImVec2 MeasureContent(const ImVec2& available) override` | 算出 `capsule_rect_` 与每个按钮的 `position`；高度默认 = 胶囊高 + `label_height` + 外框 |
| `void OnDrawContent(ImDrawList* dl, const Rect& content) override` | 画胶囊容器（阴影 / 底色 / 边框），垫在子按钮之下 |
| `void OnDrawOverlay(ImDrawList* dl, const Rect& content) override` | 画聚焦项名称，盖在子按钮之上 |
| `void OnUpdate(float dt) override` | 记录 `label_index_`，按 `label_fade` 推进 `label_alpha_` |
| `void OnThemeChanged() override` | `applyComponentStyle()` + 重新取 `Theme::kBgWidget` + 重算圆角 |

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `activated` | `int` | 某项被触发（A / 点击 / 触摸）；在 `Item::on_activate` 之后发出 |

## 交互与键盘 / 手柄

| 输入 | 行为 |
|---|---|
| ← / → | 由全局焦点导航在 N 个按钮之间移动（`Rebuild()` 里把 `focus_zone` 同步给新按钮，方向键才找得到它们） |
| ↑ / ↓ | 不消费，交给页面在功能行与其它区域之间移动焦点 |
| A / Confirm | `IconButton` 的确认路径 → `clicked` → 先执行 `Item::on_activate`，再 `emit activated(index)` |
| 触摸 / 鼠标点按钮 | 与普通 `Button` 完全一样，直接触发该项 |
| 焦点视觉 | 复用 `Button` 的流光焦点框；按钮本体无边框、无底色、无阴影 |

名称显示：`OnUpdate` 里 `focusedIndex()` 有值就把 `label_index_` 记为它并按 `label_fade` 淡入，没有焦点则淡出；淡出期间仍显示最后一个名字，避免闪烁。

## 尺寸 / 主题约定

| 项 | 值 / 来源 |
|---|---|
| 胶囊高度 | `item_size + capsule_padding * 2` |
| 自然宽度 | `edge_padding * 2 + N * item_size + (N - 1) * gap` |
| 胶囊圆角 | `radius >= 0 ? radius : 胶囊高度 / 2`（默认左右半圆、上下直线） |
| 容器底色 | `Theme::kBgWidget` |
| 边框 / 阴影 | `applyComponentStyle()`，即 `Global::component_style` |
| 名称文字 | `Theme::kTextPrimary`，透明度再乘 `label_alpha_` |
| 按钮 | `IconButton` 圆形形态，`border.width = 0`、`shadow.enabled = false`、`background = 0`、`showSubtitle(false)` |

胶囊在可用宽度内整体居中，按钮在胶囊内等距摊开，两端各留 `edge_padding`（`FunctionBar.cpp:125-140`）。

## 注意点

1. 名称行常驻占位（`label_height`），所以聚焦 / 失焦不会让布局跳动。
2. `Rebuild()` 结束时会 `SetFocusZone(focus_zone)` 把分区同步给新建按钮。宿主通常是「先 `AddTo()`（页面在那里设分区）再 `SetItems()`」；不补这一步新建按钮的分区是 0，方向键分区导航会找不到它们（`FunctionBar.cpp:110-112`）。
3. 不做 GBAStation 的 0.38s 点击延迟动画：库里「A = 立即触发」的语义保持一致。
4. 不做横向滚动：按钮太多就挤，需要滚动时由宿主包一层容器。
5. 按钮重建会清空子节点（`Clear()`），重建后 `itemAt()` 之前拿到的指针失效。

设计说明见 `component_view/components/FunctionBar.md`。
