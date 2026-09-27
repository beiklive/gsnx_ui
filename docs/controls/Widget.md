# Widget

来源：`component_view/Widget.h`（实现 `component_view/Widget.cpp`）
一句话：所有组件的父类，把坐标 / 尺寸 / 圆角 / 边框 / 阴影 / 布局 / 溢出滚动 / 焦点 / 命中测试 / 事件全部做完。

## 最小示例

```cpp
// Widget 是基类；页面里真正建的是它的子类（demo.cpp 里都是 Box 起手）
Box* panel = page.Root().Emplace<Box>("panel");
panel->SetPosition(40.0f, 40.0f)
    ->SetSize(600.0f, 0.0f)          // 0 = 该轴按内容自适应
    ->SetPadding(16.0f, 16.0f, 16.0f, 16.0f)
    ->SetLayout(LayoutMode::Vertical, ImVec2(0.0f, 10.0f))
    ->SetClip(true)
    ->SetFocusable(true);

Label* title = panel->Emplace<Label>("设置");
title->SetName("panel_title");
title->setFontSize(Theme::kFontHeader);

connect(panel, &Widget::clicked, &page, []( ) { /* 点击处理 */ });
```

## API

### 公开字段

| 成员 / 方法 | 说明 |
|---|---|
| `std::string name` / `WidgetId id` / `int z_order` | 组件名 / 唯一 ID（默认 0）/ 层级，`z_order` 越大越靠上（只影响绘制与命中顺序） |
| `bool visible` / `bool enabled` / `float opacity` / `bool interactive` | 隐藏不绘制；禁用不响应且乘 `disabled_opacity`；`opacity` 0..1 作用于填充/边框/文字；`interactive` = 是否参与鼠标命中 |
| `ImVec2 position` / `anchor` / `pivot` / `offset` | 位置相关（都相对父内容区）：`anchor` 分配剩余空间（0.5=居中），`pivot` 是自身枢轴，`offset` 最后叠加的像素偏移 |
| `EdgeInsets margin` / `padding` / `float corner_radius` / `corner_tl` / `corner_tr` / `corner_bl` / `corner_br` | 外距 / 内距 / 圆角（统一值 + 单角覆盖，单角 `-1` = 跟随统一值） |
| `ImVec2 size` / `min_size` / `max_size` / `float aspect_ratio` | 外框尺寸：某轴 0 = 按内容自适应，`max_size` 0 = 不限制；`aspect_ratio` >0 约束宽高比 |
| `ImU32 background` / `bool background_follows_theme` / `bool background_state_follows_theme` | 底色（0 = 不填充）；是否来自调色板；是否按 `State()` 从主题取 |
| `BorderStyle border` / `ShadowStyle shadow` / `bool border_state_follows_theme` | 边框 / 阴影（字段见 `Types.h`）；边框色是否按状态走 |
| `Overflow overflow` / `ImVec2 scroll` / `scroll_target` / `scroll_max` / `content_extent` | 溢出处理（`Visible` / `Hidden` / `Scroll`）/ 滚动量、目标滚动量、上限、子节点总尺寸 |
| `bool scroll_enabled` / `scroll_bar` / `scroll_bar_auto_hide` / `float scroll_bar_thickness` | 滚动与滚动条开关、滚动条粗细（默认 5.0） |
| `float scroll_smoothing` / `bool scroll_overscroll` / `bool scroll_snap` | 指数平滑速度（默认 14.0）/ 越界回弹 / 目标吸附到页宽整数倍 |
| `bool scroll_inertia` / `float scroll_friction` / `scroll_fling_threshold` / `scroll_fling_max` | 拖动惯性；衰减速度 5.5、进入惯性的松手速度下限 320、速度上限 4200（px/s） |
| `LayoutMode layout` / `ImVec2 gap` / `Align align_x` / `Align align_y` | 子节点排布方式与间距、对齐 |
| `bool focusable` / `focus_inert` / `focus_on_hover` / `int focus_zone` | 可聚焦 / 临时退出焦点导航 / 悬停即接管焦点 / 焦点分区（0 = 不分区，跨区只有左右可过） |
| `bool capture_horizontal` / `capture_vertical` / `focus_only_self` | 自己消费方向键 / 复合控件语义（只把自己当焦点停靠点） |
| `bool focus_frame` / `float focus_frame_width` / `focus_frame_offset` / `ImU32 focus_frame_color` | 焦点框开关、粗细、外扩、颜色 |
| `float focus_scale` / `ImVec2 focus_translate` / `float focus_animation_speed` | 聚焦缩放、位移、动画速度（默认 1.0 / 0 / 16.0） |
| `float disabled_opacity` / `ImVec2 visual_scale` / `visual_translate` | 禁用时额外透明度（默认 0.45）/ 子类每帧设置的即时视觉变换（按压缩放等） |
| `bool hovered` / `down` / `focused` / `selected` / `float focus_mix` | 每帧刷新的只读交互状态 |
| `Widget* parent` / `std::vector<std::unique_ptr<Widget>> children` | 树结构（只读）；`measured_size` / `rect` / `content_rect` / `margin_rect` 是 `LayoutTree()` 之后的几何结果 |

### 访问器与树

| 成员 / 方法 | 说明 |
|---|---|
| `bool isDown()` / `hasFocus()` / `isHovered()` / `isSelected()` / `isEnabled()` / `isVisible()` / `isFocusable()` | 只读状态访问器（Qt 命名） |
| `WidgetState State() const` | 判定顺序 Disabled > Pressed(down) > Hovered > Normal；focused / selected 不参与 |
| `Widget& Add(std::unique_ptr<Widget>)` / `template <typename T, typename... Args> T* Emplace(Args&&...)` | 挂子节点 / 建子节点并返回裸指针 |
| `Widget* Find(const std::string&)` / `template <typename T> T* FindAs(const std::string&)` | 按 `name` 查找（`FindAs` 带 `dynamic_cast`） |
| `void Remove(Widget*)` / `void Clear()` / `bool ContainsDescendant(const Widget*)` | 摘除 / 清空 / 判定后代 |
| `bool Contains(const ImVec2&) const` / `Rect ContentRect() const` / `float CornerTL() / CornerTR() / CornerBL() / CornerBR() const` | 命中矩形 / 内容区矩形 / 已解析单角覆盖的四角圆角 |

### 每帧流程

| 成员 / 方法 | 说明 |
|---|---|
| `void LayoutTree(const ImVec2& parent_content_pos, const ImVec2& parent_content_size)` | 测量 + 定位整棵子树 |
| `void UpdateTree(float dt)` / `void DrawTree(ImDrawList*)` | 更新滚动与交互、递归子节点、最后 `OnUpdate` / 绘制整棵子树 |
| `void RefreshThemeTree()` / `void CollectFocusables(std::vector<Widget*>&)` | 自己 + 所有子节点重新取调色板颜色 / 收集可聚焦组件 |
| `Widget* HitTest(const ImVec2&)` / `Widget* ScrollHost()` | 鼠标命中（子节点优先 + `z_order` 高者优先）/ 最近的滚动容器祖先（自己也算） |
| `void Move(const ImVec2&)` / `bool EnsureVisible(Widget*)` / `void EnsureRectVisible(const Rect&)` / `void ScrollPage(int direction, float scale = 1.0f)` | 平移子树 / 把焦点目标滚进可见区 / 把矩形滚进可见区 / 翻页滚动（-1 上页，+1 下页） |
| `void RequestFocus()` / `void YieldFocus()` / `Widget* FirstFocusable()` / `void SetFocusZone(int zone)` | 取 / 让出焦点 / 子树第一个可聚焦组件 / 给整棵子树设焦点分区 |
| `virtual FocusVisual BuildFocusVisual() const` / `Rect DrawRect()` / `DrawContentRect()` / `float DrawScale()` | 焦点框描述（由页面级 `FocusRing` 绘制）/ 绘制期已变换的矩形与缩放 |
| `template <typename Context, typename Callable> Connection onClicked(Context*, Callable)` | `connect(this, &Widget::clicked, …)` 的便捷写法 |

### 链式设置（按分组节选，全部返回 `Widget&`）

| 分组 | 方法 |
|---|---|
| 标识 / 状态 | `SetName` `SetVisible` `SetEnabled` `SetOpacity` `SetZOrder` |
| 位置 | `SetPosition(x,y)` `SetX` `SetY` `SetAnchor` `SetPivot` |
| 尺寸 | `SetWidth` `SetHeight` `SetSize` `SetMinSize` `SetMaxSize` |
| 边距 / 内距 | `SetMargin(EdgeInsets)` `SetMargin(l,t,r,b)` `SetMarginLeft/Top/Right/Bottom` `SetPadding(EdgeInsets)` `SetPadding(l,t,r,b)` `SetPaddingLeft/Top/Right/Bottom` |
| 圆角 | `SetRadius(v)` `SetRadius(tl,tr,bl,br)` `SetRadiusTopLeft/TopRight/BottomLeft/BottomRight` |
| 底色 / 边框 | `SetBackground(ImU32)` `SetBackground(const ImVec4&)` `SetBackgroundColor(r,g,b,a)`；`SetBorder(w, ImU32)` `SetBorder(w, ImVec4)` `SetBorderWidth` `SetBorderColor` `SetBorder(BorderSide,w,ImVec4)` `SetBorderTop/Right/Bottom/Left` |
| 阴影 | `SetShadow(const ShadowStyle&)` `SetShadowEnabled` `SetShadowOffset` `SetShadowBlur` `SetShadowSpread` `SetShadowColor` |
| 布局 / 溢出 | `SetLayout(LayoutMode, const ImVec2& spacing = ImVec2(0.0f, 0.0f))` `SetAlign(x,y)` `SetAlignX` `SetAlignY` `SetOverflow` `SetClip(bool)` |
| 焦点 | `SetFocusable` `SetFocusOnlySelf` `SetFocusOnHover` `SetFocusFrame(bool, float frame_offset = 3.0f)` `SetFocusScale` `SetFocusVisual(scale, translate, frame)` |

### 保护 / 子类扩展点

| 成员 / 方法 | 说明 |
|---|---|
| `void ApplyComponentBoxStyle()` | 套用 `Global::component_style` 的边框 / 圆角 / 阴影 |
| `void ApplyStateColors()` | 按状态从主题取底色 / 边框色（只在对应开关打开时） |
| `virtual ImVec2 MeasureContent(const ImVec2& available)` | 内容自身需要的尺寸，不含 padding/border/margin，默认 0 |
| `virtual void OnDrawContent(ImDrawList*, const Rect& content)` | 内容绘制（背景/边框之后、子节点之前） |
| `virtual void OnDrawOverlay(ImDrawList*, const Rect& content)` | 叠加绘制（子节点之后：角标、自定义焦点框） |
| `virtual void OnUpdate(float dt)` | 每帧状态（可选） |
| `virtual void Activate()` | Confirm / 鼠标点击的统一入口 |
| `virtual bool OnPadAction(InputAction action)` | 手柄按键分发（仅自己持有焦点），返回 true = 已消费 |
| `virtual void OnAfterLayout()` | 布局完成后回调（算滚动上限等） |
| `virtual void OnThemeChanged()` | 主题切换时重新取调色板颜色 |
| `ImU32 Tint(ImU32 color) const` / `float EffectiveOpacity() const` | 已乘 opacity / disabled 的颜色与不透明度 |

## 信号

| 信号 | 参数 | 触发时机 |
|---|---|---|
| `clicked` | 无 | 鼠标在命中处松开、或持有焦点时再按 Confirm（`OnPadAction` 未消费） |
| `pressed` | 无 | 命中处按下开始 |
| `released` | 无 | 按下结束（无论是否命中） |
| `hoverEntered` / `hoverLeft` | 无 | 鼠标进入 / 离开（桌面端才有意义） |
| `focusIn` / `focusOut` | 无 | 获得 / 失去手柄焦点 |
| `enabledChanged` | 无 | `enabled` 发生跳变时发一次（不是每帧发） |

## 尺寸 / 主题约定

- 焦点框颜色默认 `Theme::kAccent`（`focus_frame_color`）；字号一档读 `Theme::kFontBody` 等常量。
- 底色 / 边框色来自主题角色色：`Theme::ControlBackgroundColor(State())`、`Theme::ControlBorderColor(State())`，
  由 `background_state_follows_theme` / `border_state_follows_theme` 决定是否启用。
- `Global::component_style` 提供约定框参数并在 `ApplyComponentBoxStyle()` 里套用：
  `border_width` / `border_color` / `corner_radius` / `shadow_offset` / `shadow_blur` / `shadow_color`；
  组件只允许引用 `Theme::` 常量，不再各写一套字面量。

## 注意点

- 不借用 ImGui 的 item 机制（不调 `InvisibleButton`）：命中测试自己做，才能按 `z_order` / 子节点优先决定谁被点中。
- `size` 是外框尺寸（含 padding/border），0 表示该轴按内容自适应；`position` 等坐标都是相对父内容区的，绝对矩形每帧由 `LayoutTree()` 算好后放在 `rect` 里。
- `Widget` 不可拷贝（拷贝构造 / 赋值已 delete），子节点用 `std::vector<std::unique_ptr<Widget>>` 持有。
