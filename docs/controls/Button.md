# Button：按钮（基类 + 8 种形态）

来源：`component_view/components/Button.h`（实现 `component_view/components/Button.cpp`）
一句话：统一了边框 / 圆角 / 阴影 / 流光焦点框的按钮基类，以及文字、图标 + 文字、纯图标、开关、自定义右侧文字、LR 选项、LR 数值、文件列表行共 8 种形态。

## 基类 Button

### 最小示例

```cpp
Button* b = panel->Emplace<Button>("基类按钮");   // panel 为任意可容纳子控件的 Widget*
b->setIcon(Icons::Glyph(Icons::Material::Settings))
    ->setSubtitle("说明行", true)
    ->resize(320.0f, Theme::kControlHeight);
connect(b, &Widget::clicked, this, [] { /* 点击 */ });
```

### 公开字段

| 字段 | 默认 | 说明 |
|---|---|---|
| `icon` / `text` / `subtitle` | 空 | 图标字形 / 主文字 / 说明行 |
| `show_subtitle` / `text_align` | `false` / `TextAlign::Left` | 说明行开关 / 左侧「图标 + 文字」整块的对齐 |
| `icon_gap` / `icon_cell` | `8.0f` / `-1.0f` | 图标格与文字间距 / 图标正方形格边长（`<0` = 内容区高度） |
| `font_size` / `subtitle_size` | `0.0f` / `0.0f` | `0` → `Theme::kFontButton`(18) / `kFontButtonSub`(15) |
| `text_color` / `subtitle_color` | `kTextPrimary` / `kTextMuted` | 主文字 / 说明行颜色 |
| `text_color_follows_theme` / `subtitle_color_follows_theme` | `true` | `setTextColors()` 后置 `false` |
| `lr_slot_width` | `-1.0f` | LR 中间格宽，`<0` → `Global::component_style.lr_slot_width`(90) |
| `icon_visible` | `true` | `isIconVisible()` 还要求 `icon` 非空 |
| `icon_color` / `icon_color_follows_text` | `kTextPrimary` / `true` | 跟随时用主文字色（含状态色） |
| `show_right` | `true` | `false` 时右侧既不画也不占宽 |
| `flowing_focus` / `focus_phase_offset` | `true` / `0.0f` | 是否用流光焦点框 / 让各按钮的流光错开 |
| `focus_margin` / `focus_width` | `-1.0f` | `<0` → 全局值（2 / 3） |
| `focus_saturation` / `focus_brightness` | `-1.0f` | `<0` → 全局值（0.75 / 1.0） |

### 链式接口

| 方法 | 说明 |
|---|---|
| `setText` / `setSubtitle(value, visible = true)` / `showSubtitle(bool)` | 文字内容与说明行 |
| `setIcon` / `setIconVisible(bool)` / `isIconVisible()` | 图标与显隐（隐藏后图标格与间距都不占宽） |
| `setIconColor(ImVec4)` / `setIconFollowsText(bool)` | 显式给色后不再跟随主文字/状态 |
| `setShowRight(bool)` / `isRightVisible()` | 右侧区域显隐 |
| `setTextAlign` / `setIconGap` / `setIconCellSize` / `setFontSize(main, sub = 0.0f)` | 排版与字号（字号 `0` = 回落主题值） |
| `setTextColors(main, sub)` | 显式设色，之后不跟主题 |
| `resize(w, h)` / `moveTo(x, y)` / `setContentPadding(float)` | 尺寸、位置、内容到边框的留白 |
| `setBorderVisible` / `setShadowVisible` / `borderVisible()` / `shadowVisible()` | 无框列表行用；关掉后重新套用全局也不会打开 |
| `setBorder` / `setCornerRadius` / `setShadow` | 单实例覆盖全局约定样式 |
| `setSlotWidth` / `setFlowingFocus(bool)` / `applyComponentStyle()` | LR 中间格宽 / 焦点框开关 / 重新套用 `Global::component_style` |

### 子类扩展点（protected 虚函数，除标注外）

| 成员 | 说明 |
|---|---|
| `rightSideWidth()` / `drawRightSide(dl, right_rect)` | 右侧区域宽度与绘制 |
| `onRightSideKey(InputAction)` | 右侧区域按键消费，由 `Button::OnPadAction` 转发 |
| `SubtitleAllowed()` / `CaptionOutside()` | `TextButton` 返回 `false` / `IconButton` 返回 `true` |
| `mainFontSize()` / `subFontSize()` | 已解析字号（`font_size > 0` 用之，否则主题值） |
| `ResolvedFocusMargin()` / `ResolvedFocusWidth()` / `ResolvedSlotWidth()` | 已解析的全局回退 |
| `SubtitleVisible()` / `InkMain()` / `Ink(color)` | 说明行是否生效 / 主文字墨色 / 统一过 `EffectiveOpacity()` 的颜色 |
| `computeLeftBlock(content)` / `drawLeftBlock(dl, block)` | 左侧「图标 + 文字」块 |
| `lrKeysWidth()` / `drawLrRow(dl, right_rect, content, color)` | `[L] 固定间隔 [R]` 那一行 |
| `border_visible_` / `shadow_visible_`（非虚成员） | 边框 / 阴影显示 |

### 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `clicked` | 无 | 指针在控件内松开，或焦点态按 Confirm（A / Enter） |
| `pressed` / `released` | 无 | 指针按下开始 / 松开（`released` 无论是否命中） |
| `hoverEntered` / `hoverLeft` | 无 | 鼠标进入 / 离开（桌面端） |
| `focusIn` / `focusOut` | 无 | 获得 / 失去手柄焦点 |
| `enabledChanged` | 无 | `enabled` 变化 |
| `toggled` | `bool` | `ToggleButton` 状态变化 |
| `selectionChanged` | `int` | `OptionButton` 当前索引变化 |
| `valueChanged` | `float` | `ValueButton` 松开时发一次（短按或长按） |

### 尺寸 / 主题约定

| 项 | 值 |
|---|---|
| 统一高度 / 未给高时的高度 | `Theme::kControlHeight`(56) / `max(56, main + sub + 8)` |
| 自然宽度下限 | `120`（右侧有内容时再加 `right + 16`） |
| 主文字 / 说明行字号 | `Theme::kFontButton`(18) / `kFontButtonSub`(15) |
| 边框 / 圆角 / 阴影 | 1px `rgb(190,190,195)` / 5px / 偏移 `(4,4)`、模糊 10、`rgba(0,0,0,120)` |
| 内容留白 / LR 中间格宽 / 滚动速度 | `12` / `90` / `26 px/s` |
| 流光焦点框 | 边距 2、粗细 3、流速 0.28 圈/s、饱和度 0.75、亮度 1.0 |
| 底色 | `Normal = Theme::kBgWidget`，`Hovered` / `Pressed` 取主题状态色 |

以上默认值都在 `Global::component_style`；`OnThemeChanged()` 会重新套用并刷新所有「跟随主题」的颜色。

### 注意点

- 构造时 `focusable = true`、`focus_on_hover = true`、`focus_frame = false`：焦点框由页面级图层的 `BuildFocusVisual()` 绘制，不是 Widget 自带的单色框；`Global::pause_focus_frame` 打开时改用 `Theme::kError` 配色的角标框（不跟随圆角）。
- `setBorderVisible(false)` / `setShadowVisible(false)` 是持久设置：之后切主题或调 `applyComponentStyle()` 都不会把它们重新打开。
- `setIconColor()` 会同时置 `icon_color_follows_text = false`；恢复跟随时用 `setIconFollowsText(true)`。
- 文字块（主文字 + 说明行）整体相对内容区垂直居中，关掉说明行主文字不会偏上。
- 没有主文字（纯图标）时说明行的位置由 `CaptionOutside()` 决定：基类画在图标下方，`IconButton` 画在控件外。

## 8 种形态

```cpp
panel->Emplace<TextButton>("确定");
panel->Emplace<IconTextButton>(Icons::Glyph(Icons::Material::Play), "播放");
panel->Emplace<IconButton>(Icons::Glyph(Icons::Material::Favorite));
panel->Emplace<ToggleButton>(Icons::Glyph(Icons::Material::Wifi), "无线网络");
panel->Emplace<CustomButton>(Icons::Glyph(Icons::Material::Storage), "存储路径");
panel->Emplace<OptionButton>(Icons::Glyph(Icons::Material::ImagePlaceholder), "画面缩放");
panel->Emplace<ValueButton>(Icons::Glyph(Icons::Material::Memory), "音量");
panel->Emplace<FileButton>(FileButton::FileKind::Image, "cover.png", 4823);
```

### TextButton

| 项 | 说明 |
|---|---|
| 构造 | `TextButton()` / `TextButton(std::string value)` |
| 默认 | `name = "text_button"`、`text_align = Center`、`show_subtitle = false` |
| 覆盖 | `SubtitleAllowed()` 返回 `false`：`setSubtitle` / `showSubtitle` 在此无效，弹窗提示按钮不会带出小字 |

### IconTextButton

| 项 | 说明 |
|---|---|
| 构造 | `IconTextButton()` / `IconTextButton(glyph, value)` |
| 默认 | `name = "icon_text_button"`、`text_align = Left`；无额外字段，全部走基类接口 |

### IconButton

| 项 | 说明 |
|---|---|
| 构造 | `IconButton()` / `IconButton(std::string glyph)` |
| 默认 | `name = "icon_button"`、`text_align = Center` |
| `shape` | `IconButtonShape::RoundedSquare`（默认）/ `Circle` |
| `side` | `52.0f`；`setSide` 内部 `max(value, 8.0f)` 并同步 `size` 与圆角（圆 = 半边长，圆角方形 = 全局圆角） |
| `caption_gap` | `4.0f`，说明行与按钮之间的间距 |
| 方法 | `setShape` / `setSide` / `setCaptionGap` |
| 注意点 | `CaptionOutside()` 为 `true`，说明行画在控件外：优先下方，下方不足再等距放到上方，上下都不够就不画；可用区域 = 画布 ∩ 父节点 |

### ToggleButton

| 项 | 说明 |
|---|---|
| 构造 | `ToggleButton()` / `ToggleButton(glyph, value)` |
| `checked` / `isChecked()` | 默认 `false` |
| `on_color` / `off_color` / `knob_color` | `Theme::kAccent` / `kSwitchOff` / `kSwitchKnob` |
| `switch_colors_follow_theme` | `true`；`setSwitchColors()` 后置 `false` |
| `switch_width` / `switch_height` / `knob_speed` | `46.0f` / `26.0f` / `14.0f`（收敛速度，越大越快） |
| 方法 | `setChecked(value, notify = true)` / `setSwitchSize(w, h)` / `setSwitchColors(on, off, knob)` / `setKnobSpeed` / `knobMix()` |
| 注意点 | `setChecked` 会同步 `selected`，值未变化时直接返回、不发 `toggled`；`setSwitchSize` 把宽夹到 `>= 16`、高夹到 `[10, max(宽, 10)]`；`knob_mix_` 首帧直接对齐 `checked`，不播动画 |

### CustomButton

| 项 | 说明 |
|---|---|
| 构造 | `CustomButton()` / `CustomButton(glyph, value)` |
| `right_text` / `right_color` / `right_color_follows_theme` | 右侧自定义文字（默认空）/ `Theme::kTextPrimary` / `true` |
| 方法 | `setRightText(value, color)` / `setRightColor(color)` |
| 注意点 | `setRightText` 必须同时给颜色；两个 setter 都会把 `right_color_follows_theme` 置 `false`；右侧显隐统一用基类 `setShowRight()` |

### OptionButton

| 项 | 说明 |
|---|---|
| 构造 | `OptionButton()` / `OptionButton(glyph, value)` |
| `options` / `index` / `wrap` | 选项列表（默认空）/ 当前索引 `0` / `true`（到头是否绕回） |
| `option_color` / `option_color_follows_theme` | `Theme::kTextBright` / `true`（浅色主题黑字、深色白字） |
| 方法 | `setOptions(values, start_index = 0)` / `setIndex(value, notify = true)` / `setOptionColor` / `setWrap` / `optionCount()` / `currentOption()` |
| 按键 | `PageLeft` / `PageRight` 调 `index ∓ 1` |
| 注意点 | `options` 为空时 `setIndex` 直接返回、`currentOption()` 返回 `""`；`setOptions` 的 `start_index` 一律夹进 `[0, size-1]`；`index` 未变化不发 `selectionChanged` |

### ValueButton

| 项 | 说明 |
|---|---|
| 构造 | `ValueButton()` / `ValueButton(glyph, value)` |
| 值域 | `value = 0` / `min_value = 0` / `max_value = 100` / `step = 1` / `precision = 0` / `wrap = false` |
| `value_color` / `value_color_follows_theme` | `Theme::kTextBright` / `true` |
| 长按加速 | `repeat_delay = 0.35`、`repeat_interval = 0.12`、`repeat_min_interval = 0.03`、`repeat_accel_time = 1.6`、`repeat_max_multiplier = 8` |
| 方法 | `setup(initial, min, max, step, precision)` / `setValue(next, notify = true)` / `setValueColor` / `setRange` / `setStep` / `setPrecision` / `setWrap` / `setRepeat(delay, interval)` / `setRepeatAcceleration(accel_time, max_multiplier)` / `valueText()` / `isRepeating()` |
| 注意点 | `valueChanged` 只在「短按松开」或「长按松开」时发一次，按住过程中只改显示值；顶到边界时不产生变化、松开也不发信号；长按倍率取整，跳跃始终是 `step` 的整数倍；`setup` 不发信号（只夹值），`setStep` 传入近似 0 时保留旧步长 |

### FileButton

| 项 | 说明 |
|---|---|
| `FileKind` | `Folder` / `File`（默认） / `Image` / `Archive` / `Text` |
| `kRowHeight` | `static constexpr float = 50.0f`（未显式给高时 `MeasureContent` 用它） |
| 构造 | `FileButton()` / `FileButton(kind, file_name, size_bytes = -1)` |
| `kind` / `size_bytes` | `FileKind::File` / `-1`（`<0` = 未知） |
| `right_color` / `right_color_follows_theme` | `Theme::kTextMuted` / `true` |
| `name_font_size` | `Theme::kFontHeader`(20)，独立于普通按钮主文字；右侧信息始终用 `kFontButtonSub`(15) |
| 方法 | `setFile(kind, name, size = -1)` / `setFileKind` / `setFileSize` / `setNameFontSize(value)`（`0` = 回落 `kFontButton`）/ `setRightColor` / `rightText()` / `static IconFor(kind)` |
| 注意点 | `setFile` 与 `setFileKind` 都会按类型重设 `icon`（想换可再 `setIcon`）；`rightText()` 在 `show_right = false` 时返回空串，文件夹返回「文件夹」，文件走 `FormatFileSize`（大小未知返回空串、不画右侧），单位 1024 进制、KB 以上保留 2 位小数；右侧显隐全部走基类 `setShowRight()` |
