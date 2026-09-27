# CapsuleTabs

来源：`component_view/components/CapsuleTabs.h`（实现 `component_view/components/CapsuleTabs.cpp`）
一句话：横排胶囊标签条 —— 选中项永远停在条带中心、其它标签按中心距向两侧铺开，选中项背后一个会滑动的半透明胶囊高亮。

## 最小示例

```cpp
#include "component_view/components/CapsuleTabs.h"

CapsuleTabs* tabs = panel.Emplace<CapsuleTabs>();
tabs->setLabels({"帧率", "分辨率", "缩放"}, 0);
connect(tabs, &CapsuleTabs::selectionChanged, this, [](int index) { /* 换内容 */ });
connect(tabs, &CapsuleTabs::activated, this, [](int index) { /* 再按 A / 点已选中项 */ });
```

## API

### 公开字段与读取

| 成员 | 说明 |
|---|---|
| `std::vector<std::string> labels` | 标签文字 |
| `int index` | 当前选中下标 |
| `bool wrap` | L / R 到头是否绕回（默认 `true`） |
| `Style style` | 尺寸与动效，见下 |
| `const char* currentLabel() const` | 当前标签文字；空列表返回 `""` |
| `int count() const` | 标签数 |

### 构造与链式 setter

| 方法 | 说明 |
|---|---|
| `CapsuleTabs()` | 默认构造 |
| `explicit CapsuleTabs(std::vector<std::string> values)` | 带标签构造（内部 `setLabels`） |
| `CapsuleTabs& setLabels(std::vector<std::string> values, int start_index = 0)` | 换整组标签；`start_index` 夹到 `[0, count-1]`，并把 `slide_` 归位（**换数据不播动画**） |
| `CapsuleTabs& setIndex(int value, bool notify = true)` | 切换选中项；`wrap` 时按模运算、否则夹到两端；值没变直接返回；切项时置 `slide_ = 0` 从相邻格滑入 |

### Style

| 字段 | 默认值 | 说明 |
|---|---|---|
| `spacing` | `132.0f` | 相邻标签中心距 |
| `capsule_width` | `104.0f` | 胶囊宽 |
| `capsule_height` | `42.0f` | 胶囊高（圆角取一半 → 胶囊形） |
| `font_min` | `Theme::kFontButton`（18） | 最靠边的标签字号 |
| `font_max` | `22.0f` | 中心标签字号 |
| `alpha_min` | `0.42f` | 最靠边的标签透明度 |
| `fade_span` | `1.55f` | 超过这么多个间距（相对格位）就不画 |
| `slide_speed` | `8.0f` | 横滑速度（1/s，8 ≈ 125ms 走完一格） |
| `fill_alpha` | `22.0f / 255.0f` | 胶囊填充基准 alpha |
| `fill_alpha_max` | `44.0f / 255.0f` | 中心处填充 alpha |
| `stroke_alpha` | `70.0f / 255.0f` | 描边基准 alpha |
| `stroke_alpha_max` | `135.0f / 255.0f` | 中心处描边 alpha |
| `shadow_offset` | `{3.0f, 3.0f}` | 胶囊阴影偏移 |
| `shadow_blur` | `5.0f` | 胶囊阴影柔化半径 |

### 子类扩展点

| 方法 | 说明 |
|---|---|
| `ImVec2 MeasureContent(const ImVec2& available) override` | 高度 = `capsule_height + shadow_offset.y + shadow_blur`；宽度 = `spacing*(count-1) + capsule_width`，再夹到 `available.x` |
| `void OnDrawContent(ImDrawList* dl, const Rect& content) override` | 画胶囊（阴影 / 填充 / 描边）与全部标签 |
| `void OnUpdate(float dt) override` | 按 `style.slide_speed` 推进 `slide_` 到 1 |
| `void Activate() override` | 指针路径：点到的标签选中 / 再点已选中项发 `activated` |
| `bool OnPadAction(InputAction action) override` | 处理 `PageLeft` / `PageRight` 切项 |

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `selectionChanged` | `int` | 选中项变化（新下标） |
| `activated` | `int` | 手柄 A（`Button` 语义的确认）/ 再点一次已选中项 |

## 交互与键盘 / 手柄

| 输入 | 行为 |
|---|---|
| L / `PageLeft` | 上一项；`wrap = false` 且已在第一项时返回 `false`（不消费，留给页面级快捷键） |
| R / `PageRight` | 下一项；`wrap = false` 且已在最后一项时同样不消费 |
| A / Confirm | 走到 `Activate()`：指针在自身上时取鼠标 x 命中的标签，命中项就是当前项则发 `activated`，否则 `setIndex(hit)`；非指针路径直接对当前项发 `activated` |
| 触摸 / 鼠标点标签 | 同上：点哪个选哪个，点已选中那个才是 `activated` |
| ← / → / ↑ / ↓ | 本组件不消费（未设 `capture_horizontal`），交给全局焦点导航 |

焦点相关：`focusable = true`、`focus_on_hover = true`、`focus_frame = false` —— 胶囊本身就是焦点指示，聚焦时描边加亮并向 `Theme::kAccent` 偏移，填充 alpha 再加 `focus_mix * 0.06`、描边再加 `focus_mix * 0.25`。

绘制与动效：`SlotOffset()` 返回 `slide_dir_ * (1 - easeOutCubic(slide_))`，标签中心 = `center.x + (i - index + offset) * spacing * scale`；`prominence = max(0, 1 - |offset|)` 线性推出胶囊 alpha、标签字号（`font_min → font_max`）与透明度（`alpha_min → 1.0`），`|相对格位| > style.fade_span` 的标签直接不画。切项方向在 `wrap` 时走环形最短路径。

## 尺寸 / 主题约定

| 元素 | 主题角色 |
|---|---|
| 胶囊填充 | `Theme::kCapsuleFill`（浅色主题下是黑色半透明） |
| 胶囊描边 | `Theme::kCapsuleStroke`，聚焦时与 `Theme::kAccent` 混合 |
| 胶囊阴影 | `Theme::kCapsuleShadow` |
| 标签文字 | `Theme::kTextPrimary` |

`background = 0`，控件自己不画底色 / 边框，只有胶囊。

## 注意点

1. 胶囊画在标签**之前**：胶囊是半透明的，后画文字才不会被蒙上一层（`CapsuleTabs.cpp:174`）。
2. `setLabels()` 会把 `slide_` 置 1、`slide_dir_` 置 0，所以换数据不播横滑动画；只有 `setIndex()` 改值才滑。
3. `Activate()` 里指针分支用 `ItemAtX(Global::mouse.x)`，命中半径是 `spacing * scale * 0.5`，点在标签之间的空隙上会落到最近的标签。
4. 标签超出 `fade_span`（默认 1.55 个间距）就不绘制，这是「聚在中心」的裁剪，不是数据丢失。
