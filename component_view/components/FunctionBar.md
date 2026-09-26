# FunctionBar：功能按钮行

一块面板里等分排开的动作项：**图标在上、名字在下**，左右移动焦点，A / 点击触发。

## 1. 学习来源（GBAStation `SwitchLayout`）

对应 GBAStation `src/ui/view/SwitchLayout.cpp` 的「功能按钮行」（`_drawFunctions` cpp:1248-1370）：

| GBAStation 的做法 | 出处 | 本组件 |
|---|---|---|
| 一条横向胶囊条，里面等距排 6 个功能项（游戏库 / 文件列表 / 数据管理 / 设置 / 关于 / 退出） | cpp:146-153、cpp:1248-1257 | `SetItems()` 传什么就排什么（demo 用同样的 6 项） |
| 每项 = 大图标 + 名字，选中项放大并套一圈渐变流光 | cpp:1308-1359 | `Button` 的「纯图标 + 说明行」形态 + Button 自带的流光焦点框 |
| 按 A 先播「按下回弹」再执行动作（延迟 0.38s 跳页，避免动画被切掉） | cpp:612-621、cpp:876-880 | **不移植**：库里「A = 立即触发」的语义要保持一致，节奏动画交给宿主 |
| 面板底 / 描边 / 阴影自己用 nanovg 画 | cpp:1268-1292 | 面板 = 普通 `Box` + `ApplyComponentBoxStyle()`，底色 `Theme::kBgPanel` |
| 焦点项判定自己维护（`m_functionFocus` / `_moveHorizontal`） | cpp:837-850、hpp:108 | **不做**：直接复用焦点系统，左右由全局焦点导航在 6 个 Button 之间走 |

一句话：**这是「把 GBAStation 的功能行拆成『面板 + 现成 Button』」的版本** ——
因为它复用了 Button，所以焦点框、禁用置灰、触摸点击、说明行排版都不需要新代码。

## 2. 快速开始

```cpp
FunctionBar* bar = panel.Emplace<FunctionBar>();
bar->SetItems({
    {Icons::Glyph(Icons::Material::Games),    "游戏库",   [this] { OpenLibrary(); }},
    {Icons::Glyph(Icons::Material::Settings), "设置",     [this] { OpenSettings(); }},
});
connect(bar, &FunctionBar::activated, this, [](int index) { /* 统一埋点 / 日志 */ });
```

`Item::on_activate` 可空：留空时就只发 `activated(index)`，由宿主统一分发（demo 就是这么用的）。

## 3. 组成与接口

```
FunctionBar（Box，圆角 / 边框 / 阴影 / 底色都走全局约定）
└─ Button × N（等宽；text 为空 → 图标居中，subtitle 就是名字）
```

| 方法 | 说明 |
|---|---|
| `SetItems(std::vector<Item>)` | 重建全部项（旧项会被清掉） |
| `AddItem(icon, label, on_activate = {})` | 追加一项 |
| `SetStyle(const Style&)` | 尺寸参数（见下） |
| `itemAt(i)` / `count()` / `focusedIndex()` | 读取项与焦点 |
| `signal activated(int)` | 某项被触发（A / 点击 / 触摸） |

| Style 字段 | 默认 | 说明 |
|---|---|---|
| `item_height` | 84 | 单项高（图标 + 名字） |
| `item_min_width` | 96 | 单项最小宽度（条太窄时按它给，横向溢出由宿主管） |
| `gap` | 10 | 项间距 |
| `padding` | -1 | <0 = 用 `Global::component_style.content_padding` |
| `radius` | -1 | <0 = 用 `Global::component_style.corner_radius` |
| `icon_size` | 30 | 图标方形格边长（图标大小跟着格子走） |

## 4. 交互

* **手柄**：← / →（或摇杆）在项之间移动焦点，A 触发当前项；焦点框是 Button 的流光框。
* **触摸 / 鼠标**：直接点某一项就触发它（和普通 Button 完全一样）。
* **焦点分区**：`Rebuild()` 结束时会 `SetFocusZone(focus_zone)` 把分区同步给新建的子项 ——
  宿主通常是「先 `AddTo()`（页面在那里设分区）再 `SetItems()`」，
  不补这一步的话新建按钮的分区是 0，方向键在分区之间导航会找不到它们（已实测踩过）。

## 5. 限制 / 未做

1. **不做横向滚动**：项多了就窄（受 `item_min_width` 保护）；需要滚动条请宿主包一层容器。
2. **不做选中态**：功能行是「动作」不是「选择」；要有选中态用 CapsuleTabs / TabColumn。
3. **不移植 GBAStation 的点击延迟动画**：理由见 §1。
4. 每一项都是真实 `Button`，所以也继承了 Button 的默认尺寸/字号规则；要更紧凑就改 `Style::item_height`
   与 `icon_size`。
