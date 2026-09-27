# ProgressBar

来源：`component_view/components/Content.h`（实现 `component_view/components/Content.cpp`）
一句话：确定进度（0..1，显示值平滑跟随）与不确定进度（跑动条）两种形态的进度条，页面里可以直接用。

## 最小示例

```cpp
ProgressBar* bar = page.Root().Emplace<ProgressBar>();
bar->setLabel("正在加载核心");
bar->setBarHeight(12.0f);
bar->setValue(0.62f);

ProgressBar* waiting = page.Root().Emplace<ProgressBar>();
waiting->setIndeterminate(true);
waiting->setLabel("正在查找更新…");
waiting->show_percent = false;   // 不确定进度不显示百分比
```

## API

### 构造与公开字段

| 成员 / 方法 | 说明 |
|---|---|
| `ProgressBar()` | 默认名 `"progress_bar"`，`text_color = Theme::kTextMuted`，`radius = Global::component_style.corner_radius` |
| `float value` | 0..1；显示值会按 `smooth_speed` 平滑跟随（`setValue` 会夹到 0..1） |
| `bool indeterminate` | true = 跑动条，默认 false |
| `bool show_percent` | 右侧显示百分比，默认 true；不确定进度分支不画百分比 |
| `bool show_label` | 是否显示 `label`，默认 false；`setLabel` 会按 `label.empty()` 打开 / 关闭 |
| `std::string label` | 条上方的说明文字 |
| `float bar_height` | 条高，默认 10.0；绘制与测量都按 `Maxf(bar_height, 2.0f)` |
| `float radius` | 条圆角，默认取全局约定圆角；绘制时夹到 `0..bar.Height() * 0.5` |
| `float font_size` | 0 = `Theme::kFontSmall` |
| `float label_gap` | 文字与条之间的间距，默认 8.0 |
| `ImVec4 track_color` | 轨道色，alpha = 0 → `Theme::kTrack` |
| `ImVec4 fill_color` | 填充色，alpha = 0 → `Theme::kAccent` |
| `ImVec4 text_color` | 文字色，默认 `Theme::kTextMuted` |
| `bool text_color_follows_theme` | 默认 true；本文件没有暴露关闭它的链式方法，需要时直接改字段 |
| `float smooth_speed` | 显示值平滑速度，默认 12.0；0 = 直接跳到 `value` |

### 链式设置（全部返回 `ProgressBar&`）

| 成员 / 方法 | 说明 |
|---|---|
| `setValue(float next)` | 写入 `Clampf(next, 0.0f, 1.0f)` |
| `setLabel(std::string value)` | 设 `label`，并把 `show_label` 设为 `!label.empty()` |
| `setIndeterminate(bool enabled)` | 设 `indeterminate` |
| `setBarHeight(float value)` | 设 `bar_height` |
| `setColors(ImVec4 fill, ImVec4 track)` | 同时设 `fill_color` 与 `track_color` |

### 保护 / 子类扩展点

| 成员 / 方法 | 说明 |
|---|---|
| `ImVec2 MeasureContent(const ImVec2& available) override` | 宽度 = `size.x > 0 ? size.x : Maxf(available.x, 220.0f)`；高度 = 文字高 + （有文字时的 `label_gap`）+ `Maxf(bar_height, 2.0f)` |
| `void OnDrawContent(ImDrawList* dl, const Rect& content) override` | 先画 label（超出按内容区宽度的 0.72 倍省略）与百分比，再画圆角轨道；确定模式填充 `bar.Width() * shown_`，不确定模式画宽度 38% 的跑动条 |
| `void OnUpdate(float dt) override` | 平滑推进 `shown_`；`indeterminate` 时按 `dt * 0.55` 推进跑动相位并回绕到 0..1 |
| `void OnThemeChanged() override` | `text_color_follows_theme` 为 true 时重取 `Theme::kTextMuted`，并把 `radius` 重取为 `Global::component_style.corner_radius` |

ProgressBar 不声明自己的信号，事件接口继承自 [Widget](Widget.md)。

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| 继承自 `Widget` 的 `clicked` / `pressed` / … | 无 | 默认不参与焦点；信号表见 [Widget.md](Widget.md) |

## 尺寸 / 主题约定

- 轨道默认 `Theme::kTrack`、填充默认 `Theme::kAccent`，都通过“alpha = 0 → 主题色”的约定生效；
  `setColors` 传入带 alpha 的颜色即可覆盖（不会改 `Theme` 常量）。
- 文字默认 `Theme::kFontSmall` 字号、`Theme::kTextMuted` 颜色。
- 宽度未显式设定时测量下限是 220.0；条高下限 2.0。
- 绘制时填充 / 轨道 / 文字都按 `EffectiveOpacity()` 乘一次透明度。

## 注意点

- `setLabel("")` 会把 `show_label` 置为 false（因为 `show_label = !label.empty()`）；只想隐藏文字可以直接设 `show_label = false`，`label` 字符串会保留。
- `show_label` / `show_percent` 任一为真，条就会下移一行（`bar_top = content.min.y + 文字高 + label_gap`）；两者都关掉时条从内容区顶部开始。
- `indeterminate = true` 时 `setValue` 的结果不参与绘制（百分比也不画），但 `shown_` 仍在按 `value` 平滑推进。
