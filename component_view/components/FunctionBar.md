# FunctionBar：功能按钮行

一条**胶囊容器**（左右两端半圆、上下直线）里等距排 N 个**无边框圆形按钮**；
**名字只在某个按钮获得焦点时显示**，画在容器下方居中。

## 1. 学习来源（GBAStation `SwitchLayout`）

对应 GBAStation `src/ui/view/SwitchLayout.cpp` 的「功能按钮行」（`_drawFunctions` cpp:1248-1370）：

| GBAStation 的做法 | 出处 | 本组件 |
|---|---|---|
| 一条横向胶囊条（左右全圆角、上下直线）里等距排 6 个功能项 | cpp:1248-1292 | 容器 = 普通 `Box`，圆角 = 高度的一半 → 同样的胶囊形 |
| 每项是一个大图标，聚焦项外圈一圈渐变流光 | cpp:1329-1359 | 每项 = 现成的 `IconButton`（圆形形态），**去掉边框/底色/阴影**；聚焦时就是 Button 那套 `Draw::FlowingRing` |
| 名字画在图标下方 | cpp:1361-1367 | **改成只在聚焦时显示**，并且画在**胶囊容器下方**（淡入淡出，不推动布局） |
| 焦点项自己维护（`m_functionFocus` / `_moveHorizontal`） | cpp:837-850、hpp:108 | 不做：直接复用焦点系统，←/→ 由全局焦点导航在 N 个按钮之间走 |
| 按 A 先播「按下回弹」再执行动作（延迟 0.38s 跳页） | cpp:612-621、cpp:876-880 | **未移植**：库里「A = 立即触发」的语义要保持一致，节奏动画交给宿主 |

## 2. 快速开始

```cpp
FunctionBar* bar = panel.Emplace<FunctionBar>();
bar->SetItems({
    {Icons::Glyph(Icons::Material::Games),    "游戏库", [this] { OpenLibrary(); }},
    {Icons::Glyph(Icons::Material::Settings), "设置",   [this] { OpenSettings(); }},
});
connect(bar, &FunctionBar::activated, this, [](int index) { /* 统一埋点 / 日志 */ });
```

`Item::on_activate` 可空：留空时就只发 `activated(index)`，由宿主统一分发（demo 就是这么用的）。

## 3. 结构

```
FunctionBar（Box：胶囊容器，圆角 = 高度一半；边框/阴影取 Global::component_style，底色 Theme::kBgWidget）
├─ IconButton × N（圆形、边框/底色/阴影全关，只有图标 + 流光焦点框，等距摊开）
└─ 聚焦项名称（本组件绘制，居中在胶囊下方；只在有焦点时淡入）
```

胶囊的自然宽度 = `2*edge_padding + N*item_size + (N-1)*gap`；宿主给的宽度更宽时，胶囊按自然宽度**居中**，
按钮在胶囊内等距摊开（两端各留 `edge_padding`）。**名称行常驻占位**，所以聚焦 / 失焦不会让布局跳动。

## 4. 接口

| 方法 | 说明 |
|---|---|
| `SetItems(std::vector<Item>)` | 重建全部项 |
| `AddItem(icon, label, on_activate = {})` | 追加一项 |
| `SetStyle(const Style&)` | 尺寸参数（见下） |
| `itemAt(i)` / `count()` / `focusedIndex()` / `CapsuleWidth()` | 读取项、焦点与胶囊宽度 |
| `signal activated(int)` | 某项被触发（A / 点击 / 触摸） |

| Style 字段 | 默认 | 说明 |
|---|---|---|
| `item_size` | 44 | 圆形按钮直径（图标大小跟着它走） |
| `capsule_padding` | 10 | 胶囊上下内边距 |
| `edge_padding` | 18 | 胶囊左右内边距（半圆两端留白） |
| `gap` | 22 | 相邻按钮最小间距（容器更宽时自动摊开） |
| `label_gap` / `label_size` / `label_height` | 6 / `kFontSmall` / 24 | 名称与胶囊的间距、字号、常驻行高 |
| `label_fade` | 12 | 名称淡入淡出速度（1/s） |
| `radius` | -1 | <0 = 胶囊（高度的一半）；>0 可改成普通圆角面板 |

## 5. 交互

* **手柄**：← / → 在按钮之间移动焦点（Button 的流光框就是焦点指示），聚焦时名字在容器下方淡入；A 触发。
* **触摸 / 鼠标**：直接点某个圆按钮就触发它（和普通 Button 完全一样）。
* **未聚焦时**：容器下方的名字淡出，按钮只显示图标。
* **焦点分区**：`Rebuild()` 结束时会 `SetFocusZone(focus_zone)` 把分区同步给新建的按钮 ——
  宿主通常是「先 `AddTo()`（页面在那里设分区）再 `SetItems()`」，
  不补这一步新建按钮的分区是 0，方向键在分区之间导航会找不到它们（已实测踩过）。

## 6. 限制 / 未做

1. **不做横向滚动**：按钮多了就挤（受 `item_size` / `gap` 控制）；需要滚动请宿主包一层容器。
2. **名字只显示当前焦点项**：不给每项常驻标签（这是需求要的形态）；要常驻名字用 CapsuleTabs。
3. **不做选中态**：功能行是「动作」不是「选择」。
4. **不移植 GBAStation 的点击延迟动画**：理由见 §1。
