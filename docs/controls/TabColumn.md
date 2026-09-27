# TabColumn

来源：`component_view/components/TabColumn.h`（实现 `component_view/components/TabColumn.cpp`）
一句话：左侧纵向单选标签列 —— 只负责「列」本身（排布、选中态、上下遍历、点选、溢出滚动），选中后切什么内容由页面接 `selectionChanged` 自己处理。

## 最小示例

```cpp
#include "component_view/components/TabColumn.h"

TabColumn* tabs = panel.Emplace<TabColumn>();
tabs->setItems({{Icons::Glyph(Icons::Material::Settings), "常规"},
                {Icons::Glyph(Icons::Material::Info), "关于"}});
tabs->setFocusTarget(toggle_); // → / A 进内容区的入口控件
connect(tabs, &TabColumn::selectionChanged, this, [](int index) { /* 切内容 */ });
```

## API

### 公开字段与读取

| 成员 | 说明 |
|---|---|
| `struct Item { std::string icon; std::string text; }` | 一项：Material 字形（可空）+ 文字 |
| `Style style` | 几何与动效，见下 |
| `int index() const` | 当前选中下标 |
| `int count() const` | 项数 |
| `Button* itemAt(int index) const` | 取第 index 项的内部 `Button`（越界返回 `nullptr`） |
| `const std::vector<Item>& items() const` | 原始项数据 |
| `Widget* focusTarget() const` | 当前内容区入口控件 |

### 链式 setter 与导航方法

| 方法 | 说明 |
|---|---|
| `TabColumn& setItems(std::vector<Item> items)` | 重建整列：清空后按 `Item` 建 `Button` 子项，重置 `focus_anim_`，应用 `gap.y`、`SetFocusZone(style.focus_zone)`，并 `PlayEnter()` 播一次入场 |
| `TabColumn& setIndex(int value, bool notify = true)` | 选中第 value 项（`ClampIndex` 夹到 `[0, count-1]`）；值没变直接返回，`notify` 控制是否发 `selectionChanged` |
| `TabColumn& setFocusTarget(Widget* target)` | 设置内容区入口控件，供 `EnterContent()` / `OnPadAction(PageRight)` 使用 |
| `bool EnterContent()` | 入口控件存在、`visible` 且 `enabled` 时 `RequestFocus()` 并返回 `true`，否则返回 `false` |
| `void FocusCurrentItem()` | 焦点回到本列当前选中项（子页面里按 B 时用） |
| `void PlayEnter()` | 重播一次入场（把 `enter_time_` 归零） |

### Style

| 字段 | 默认值 | 说明 |
|---|---|---|
| `item_height` | `Theme::kControlHeight`（56） | 每项高度 |
| `item_gap` | `4.0f` | 项间距（赋给 `gap.y`） |
| `item_radius` | `5.0f` | 选中底圆角（`<= 0` = 胶囊） |
| `content_padding` | `16.0f` | 项内左边距（左侧色条就在这里） |
| `padding_y` | `12.0f` | 项内上下留白 |
| `indicator_width` | `4.0f` | 左侧强调色条宽度，`0` = 不画 |
| `indicator_inset` | `10.0f` | 色条离项左边缘 |
| `indicator_margin_y` | `12.0f` | 色条上下留白 |
| `focus_zone` | `1` | 整列（含 item）的焦点分区 |
| `enter_duration` | `0.28f` | 整列入场时长（秒） |
| `enter_stagger` | `0.025f` | 逐项错开（秒/项） |
| `enter_offset` | `-30.0f` | 入场时从左侧滑入的偏移（px） |
| `focus_duration` | `0.16f` | 焦点切换响应时长（秒） |
| `focus_offset` | `4.0f` | 焦点项右移量（px，套 `EaseOutBack`） |

### 子类扩展点

| 方法 | 说明 |
|---|---|
| `ImVec2 MeasureContent(const ImVec2& available) override` | 返回 `{0,0}`：宽高都由页面显式给 |
| `void OnDrawContent(ImDrawList* dl, const Rect& content) override` | 画选中底（`Theme::kSelection`）+ 左侧色条（`Theme::kAccent`） |
| `void OnUpdate(float dt) override` | 推进入场 / 焦点动画；同步「焦点项 = 选中项」；焦点自动滚动 |
| `bool OnPadAction(InputAction action) override` | 只处理 `PageRight`：有 `focus_target_` 就 `RequestFocus()` 并返回 `true` |

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `selectionChanged` | `int` | 选中项变化（新下标）；`setIndex` 值没变时不发 |
| `activated` | `int` | 对已选中项再按 A / 非触摸路径再点一次 |

## 交互与键盘 / 手柄

| 输入 | 行为 |
|---|---|
| ↑ / ↓ | 全局焦点导航在列内各项之间移动（整列同一 `focus_zone`） |
| ← / → | 全局焦点导航：**同分区还有别的控件时先在本分区内移动**，只有该方向没有同分区候选时才跨分区（这就是「内容区按 ← 回左列、左列按 → 进内容」的规则，`Global.cpp:204-208`） |
| A / Confirm | 内部 `TabItem` 先吃掉按键并调用 `EnterContent()`；成功（入口控件可见且启用）时返回 `true`，不走 `Button` 的 `clicked` / `activated` |
| R / `PageRight` | 扩展点：`TabColumn::OnPadAction` 在 `focus_target_ != nullptr` 时 `RequestFocus()`。容器 `focusable = false`，正常不会收到手柄键（见注意点 6） |
| B / Cancel | 本组件不消费，由页面调 `FocusCurrentItem()` 把焦点收回列里 |
| 点击 / 触摸项 | `clicked` → `SelectAt()`：下标不同就 `setIndex`；相同且是触摸（`Global::pointer_touch`）则直接返回；相同且非触摸才发 `activated` |

**选中 / 焦点关系**：焦点落到哪一项，选中就立刻跟到哪一项（`OnUpdate` 里比对 `Global::focused` 与各 item，命中就 `setIndex(i)`），同一帧发出 `selectionChanged` —— 「切焦点 = 切页面」，不需要再按 A。焦点项变化且落本列内时会 `EnsureVisible(focused)` 把它滚进可见区。

## 尺寸 / 主题约定

| 元素 | 主题角色 |
|---|---|
| 选中底 | `Theme::kSelection` |
| 左侧强调色条 | `Theme::kAccent` |
| 选中项文字 | `Theme::kTextBright`，并按焦点动画用 `Theme::Mix` 往亮色靠 60% |
| 未选中项文字 | `Theme::kTextPrimary` |
| 容器 | `background = 0`，列本身不画底 / 边框 |

内部 `Button` 子项被显式关掉边框、阴影、底色与状态底色/边框色，圆角改成与选中底一致（`item_radius` 或高度一半），保证流光焦点框的圆角对得上。

## 注意点

1. 构造函数有内边距：右内边距 = `6 + focus_offset + 8`。只留 6px 的话，焦点框和右移后的焦点项右边会被溢出裁剪切掉（`TabColumn.cpp:40-43`）。
2. 选中底**不做位移动画**：底色直接落在当帧选中项自己的矩形上（含它的入场 / 焦点位移），切到哪一项就落在哪一项，不会从上一项滑过去。
3. `EnterContent()` 有前置条件：目标为 `nullptr`、不可见或禁用都返回 `false`。
4. 触屏按下时焦点切换已经完成过一次 `selectionChanged`，所以释放时再次命中同一项**不发** `activated`，避免重播页面入场动画（`TabColumn.cpp:142-153`）。
5. `setItems()` 建完就播一次入场；换数据不需要再手动 `PlayEnter()`。
6. `OnPadAction(PageRight)` 是 `TabColumn` 自己的实现，但构造函数里 `focusable = false`、`Widget` 只给持有焦点的控件派发手柄键，所以实际进入内容区走的是 item 上的 A / Confirm（`TabItem` 先吃掉按键调 `EnterContent()`），或全局导航的 → 跨分区。
