# Separator

来源：`component_view/components/Content.h`（实现 `component_view/components/Content.cpp`）
一句话：水平（默认）或垂直的分隔线，颜色跟主题，两端可以缩进留白。

## 最小示例

```cpp
// 水平线：长度撑满父内容区（Content.h 注释里的写法）
page.Root().Emplace<Separator>();

// 垂直线
page.Root().Emplace<Separator>(Separator::Orientation::Vertical);

// 水平线 + 固定长度 + 两端缩进 16px
Separator* rule = page.Root().Emplace<Separator>();
rule->setThickness(2.0f);
rule->setLength(320.0f);
rule->setInset(16.0f);
rule->setColor(Theme::kBorder);
```

## API

### 构造与公开字段

| 成员 / 方法 | 说明 |
|---|---|
| `enum class Orientation { Horizontal, Vertical }` | 方向枚举 |
| `Separator()` | 默认名 `"separator"`，`orientation = Horizontal` |
| `explicit Separator(Orientation orientation)` | 先默认构造再覆盖 `orientation` |
| `Orientation orientation` | 默认 `Horizontal` |
| `float thickness` | 线宽，默认 1.0；绘制与测量都按 `Maxf(thickness, 1.0f)` |
| `float length` | 0 = 撑满父内容区（水平 = 宽度，垂直 = 高度） |
| `float inset_start` | 起点端缩进，默认 0 |
| `float inset_end` | 终点端缩进，默认 0 |
| `ImVec4 color` | 默认 `{0, 0, 0, 0}`，alpha = 0 表示跟主题（`Theme::kBorder`） |
| `bool color_follows_theme` | 默认 true；`setColor` 会置 false |

### 链式设置（全部返回 `Separator&`）

| 成员 / 方法 | 说明 |
|---|---|
| `setOrientation(Orientation value)` | 设方向 |
| `setThickness(float value)` | 设线宽 |
| `setLength(float value)` | 设长度（0 = 撑满） |
| `setInset(float start, float end = -1.0f)` | 设两端缩进；`end < 0` 时 `inset_end = start`（两端同值） |
| `setColor(ImVec4 value)` | 设线色并关闭主题跟随 |

### 保护 / 子类扩展点

| 成员 / 方法 | 说明 |
|---|---|
| `ImVec2 MeasureContent(const ImVec2& available) override` | 水平：`(length > 0 ? length : max(available.x, 1), max(thickness, 1))`；垂直：宽高对调 |
| `void OnDrawContent(ImDrawList* dl, const Rect& content) override` | 在内容区中线画一条线；水平为 y 取中点、x 从 `min.x + inset_start` 到 `max.x - inset_end`，垂直对调 |
| `void OnThemeChanged() override` | `color_follows_theme` 为 true 时把 `color` 重置为 `ImVec4(0,0,0,0)`（即继续走 `Theme::kBorder`） |

Separator 不声明自己的信号；默认 `focusable = false`，只作为排版用的装饰件，事件接口见 [Widget](Widget.md)。

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| 继承自 `Widget` 的 `clicked` / `pressed` / … | 无 | 默认不参与焦点；信号表见 [Widget.md](Widget.md) |

## 尺寸 / 主题约定

- 颜色：`color_follows_theme` 为 true 或 `color.w <= 0` 时用 `Theme::kBorder`，否则用显式 `color`。
- 绘制时按 `EffectiveOpacity()` 乘一次透明度（禁用时会叠加 `disabled_opacity`）。
- `length = 0` 时长度取父给的可用尺寸；水平取 `available.x`，垂直取 `available.y`。
- 线宽与测量都至少按 1.0 处理（`Maxf(thickness, 1.0f)`），所以 `thickness = 0` 仍会画 1px。

## 注意点

- `setInset(start)` 只传一个参数时两端缩进相同；只要一端不同就传两个参数（`end` 为负数会被替换成 `start`）。
- 缩进是直接在线上截取的，不改变测量尺寸：`inset_start` + `inset_end` 大于可用宽度时线会被截短甚至画不出可见线段。
- `OnThemeChanged()` 只是把 `color` 复位为 alpha = 0，并不是把主题色写进字段。
