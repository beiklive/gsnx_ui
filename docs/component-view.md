# component_view 组件库参考

`component_view/` 是这套 UI 的组件层：**不依赖任何业务代码**，只依赖 `framework/` 里的引擎能力
（输入抽象、图标表、宿主桥接、主循环）。本文是组件的 API 参考、视觉/动画规范与接入清单。

- 演示入口：[demo.cpp](../demo.cpp)（左侧 tab + 4 个子页面，展示全部组件）
- 最小接入示例：[examples/min_demo/main.cpp](../examples/min_demo/main.cpp)（另一个项目要写的全部代码，约 150 行）

---

## 1. 分层与目录

```text
framework/               引擎层（不含业务 UI）
├── core/App.{h,cpp}     App 基类 + AppRunner 主循环（帧率上限在这里）
├── ui/UiContext.*       宿主桥接：BeginFrame/EndFrame、Pad()、DeltaTime、SetUiZoom
├── ui/Icons.*           图标表：按键图标 16 个 + Material 图标 42 个（字形来自字体资源）
├── platform/Backend.h   后端接口 + BackendConfig（title/size/vsync/max_fps）
└── platform/Input.h     InputAction / PadState / InputFrame（头文件，无实现）

component_view/          组件层（业务 UI 直接用它）
├── Object.h             Qt 风格信号槽：Object / Signal<Args...> / connect / emit
├── Global.{h,cpp}       画布尺寸、鼠标/手柄、焦点、输入消费、每帧 Begin/EndFrame
├── Theme.{h,cpp}        调色板（浅/深两套，运行时切换）+ 720p 尺寸规范
├── Types.h              Rect / EdgeInsets / BorderStyle / ShadowStyle / BoxVisual / Transform2D / 枚举
├── Anim.h              动画工具：SmoothTo / MoveTowards / AdvanceOnce / EaseOutCubic / EaseOutBack / Stagger*
├── Draw.{h,cpp}         绘制原语（圆角矩形 / 软阴影 / 文本 / 流光框 / 跑马灯 / 省略号…）
├── Widget.{h,cpp}       所有组件的父类：几何 / 焦点 / 溢出滚动 / 命中 / 事件
├── FocusRing.{h,cpp}    ★ 焦点框图层（页面级，一帧一个）
├── Toast.{h,cpp}        Toast 管理器（页面级，画在最上层）
├── components/          Box / Button(7 形态) / Badge / Header / TabColumn / CapsuleTabs
└── pages/Page.{h,cpp}   页面基类（持有根 Box、Toast、焦点框图层）
```

**每帧顺序**（`AppRunner` 已经封装好，宿主只需实现 `App::OnFrame`）：

```text
Global::BeginFrame(ui) → page.Update(dt) → page.Render() → Global::EndFrame()
                          │                │
                          │                ├─ root_->DrawTree()      组件树
                          │                ├─ OnOverlay()            页面叠加层
                          │                ├─ focus_ring_.Draw()     焦点框（一帧一个）
                          │                └─ toasts_.Draw()         通知（最上层）
                          └─ LayoutTree → HitTest → NavigateFocus → UpdateTree → OnInput → OnUpdate
```

---

## 2. 接进一个新项目要做什么

### 2.1 需要一起带上的东西

| 类别 | 具体 |
|---|---|
| 代码 | `component_view/` 全部 + `framework/`（至少 `platform/Input.h`、`platform/Backend.h`、`ui/UiContext.*`、`ui/Icons.*`、`ui/Fonts.*`、`core/App.*`、`core/Scene.*`、SDL2 后端） |
| 资源 | `MaterialIcons-Regular.ttf`（Material 图标）+ 按键图标字体（L/R/A/B 等手柄字形），路径由 `platform/Fonts.*` 注册 |
| 编译 | C++17 + 随仓库的 imgui（`third_party/imgui`，锁定 v1.92.9b）+ SDL2 |
| 链接 | `target_link_libraries(your_app PRIVATE gui_dev_components)`（CMake 里已把 component_view 打成静态库） |

### 2.2 宿主必须做的初始化（缺一不可）

```cpp
void OnStart(UiContext& ui) override {
    ui.SetUiZoom(1.0f);                       // UI 缩放：整套组件一起放大（1.2 = 大 20%）
    Theme::SetMode(Theme::ThemeMode::Dark);   // 主题（运行时可切）
    Theme::ApplyToImGui();                    // 让 ImGui 原生控件跟着主题
    Global::ApplyTheme();                     // 约定样式（边框/阴影）跟着主题
    Scenes().Reset(std::make_unique<MyScene>());
    page_ = std::make_unique<MyPage>();
    page_->Bind(ui);
}
```

切主题时（例如「浅色/深色」按钮）：

```cpp
Theme::ToggleMode();
Theme::ApplyToImGui();
page_->RefreshTheme();   // = Global::ApplyTheme() + 根节点装饰复位 + 整棵树重新取色
```

### 2.3 写一个页面

```cpp
class MyPage : public Page {
public:
    const char* Title() const override { return "settings"; }

    void OnBuild() override {                       // 只调一次：往 Root() 里放组件
        header_ = Root().Emplace<Header>("显示");
        toggle_ = Root().Emplace<ToggleButton>(Icons::Glyph(Icons::Material::Wifi), "无线网络");
        connect(toggle_, &ToggleButton::toggled, this, [](bool on) { /* … */ });
    }

    void OnUpdate(float dt) override {              // 每帧：排版（画布尺寸可变）
        (void)dt;
        const float w = Global::canvas_size.x - 40.0f;
        header_->position = ImVec2(20.0f, 20.0f);
        header_->size.x = w;
        toggle_->moveTo(20.0f, 90.0f);
        toggle_->resize(w, Theme::kControlHeight);
    }

    void OnInput() override { /* 页面级快捷键；按键要先 Global::Available(a) 再 MarkConsumed(a) */ }
};
```

布局约定：`position` 相对**父节点内容区**（`Root()` 的内容区 = 画布），`size = 0` 表示按内容自适应。
内容装不下就 `overflow = Overflow::Scroll`（还有 `scroll_bar_auto_hide`），焦点项会自动滚进可见区。

---

## 3. 设计空间、尺寸与主题

### 3.1 两个坐标概念

- **设计空间 = 720p**：组件里写的一切数字（56 行高、20 字号…）都是这个空间的值。
- **逻辑画布** = `drawable / (auto_scale × ui_zoom)`。后端负责缩放，组件只认 `Global::canvas_size`。
  - `auto_scale = min(高/720, 宽/1280)`：16:9 下恒为 1:1 设计空间；更方/更窄的屏逻辑画布会变高（面板居中）。
  - `ui_zoom`：整套 UI 放大倍数（1.2 → 逻辑画布 1067×600）。**只在启动前设置**（运行期改渲染缩放会让 SDL/Metal 状态错乱）。

### 3.2 尺寸常量（`Theme.h`）

| 常量 | 值 | 用途 |
|---|---|---|
| `kFontTitle / kFontHeader / kFontBody / kFontSmall / kFontTiny` | 22 / 20 / 16 / 14 / 12 | 标题 / 区块标题 / 正文 / 说明 / 极小 |
| `kControlHeight` | 56 | 按钮、列表行统一高度 |
| `kBadgeHeight` | 26 | 机种徽标 |
| `kListRowHeight / kKeySize` | 32 / 30 | 列表行 / 虚拟键盘按键（预留给后续组件） |
| `kRadiusNone/Small/Radius/Large` | 0 / 3 / 6 / 12 | 圆角档位 |
| `kGapSmall / kGap / kGapLarge` | 6 / 10 / 16 | 间距档位 |
| `kPagePadding / kTabColumnWidth / kHudHeight` | 18 / 210 / 36 | 页面留白 / 左侧 tab 列宽 / HUD 条高 |

### 3.3 颜色角色（`Theme::`，运行时可切）

| 角色 | 深色 | 浅色 | 用途 |
|---|---|---|---|
| `kBgEditor / kBgPanel / kBgWidget / kBgWidgetHi / kBgInput` | 30,30,30 / 37,37,38 / 45,45,48 / 55,55,58 / 60,60,60 | 255 / 243 / 232 / 220 / 255 | 页面 / 面板 / 控件 / 悬停 / 输入 |
| `kBorder / kBorderStrong / kControlBorder` | 60 / 84 / 190 | 215 / 160 / 203 | 边框 |
| `kTextPrimary / kTextBright / kTextMuted / kTextDisabled` | 212 / 255 / 133 / 106 | 31 / 0 / 106 / 150 | 文字四档 |
| `kAccent / kAccentHover / kSelection / kButton / kButtonActive` | 0,122,204 / … / 38,79,120 / … | 0,95,184 / … / 173,214,255 / … | 强调 / 选中底 / 按钮 |
| `kSuccess / kWarning / kError / kTeal / kWhite …` | 两套主题共用 | — | 语义色 |
| `kCapsuleFill / kCapsuleStroke / kCapsuleShadow` | 白 + 淡阴影 | 黑 + 淡阴影 | 胶囊高亮（CapsuleTabs） |

> 规则：组件不写死颜色，一律读角色色；显式设过色的属性会关掉「跟随主题」标志（`*_follows_theme = false`）。

---

## 4. 组件参考

### 4.1 Widget（基类，`component_view/Widget.h`）

所有组件的父类，提供几何、焦点、滚动、命中与事件。

| 分类 | 成员 |
|---|---|
| 几何 | `position / anchor / pivot / offset / margin / padding / size / min_size / max_size / aspect_ratio` |
| 视觉 | `corner_radius`(+四角覆盖) / `background`(+`background_follows_theme`) / `border` / `shadow` / `opacity` / `visible` / `enabled` |
| 布局 | `layout`(Free/Vertical/Horizontal) + `gap` + `align_x/align_y` |
| 滚动 | `overflow`(Visible/Hidden/Scroll) / `scroll` / `scroll_target` / `scroll_max` / `scroll_smoothing` / `scroll_overscroll` / `scroll_bar*` |
| 焦点 | `focusable` / `focus_on_hover` / `focus_zone` / `capture_horizontal` / `capture_vertical` / `focus_only_self` / `focus_frame*` / `focus_scale` / `focus_translate` |
| 状态（只读） | `hovered / down / focused / selected / focus_mix` |
| 信号 | `clicked / pressed / released / hoverEntered / hoverLeft / focusIn / focusOut / enabledChanged` |
| 树 | `Add/Emplace<T>/Find/FindAs<T>/Remove/Clear/ContainsDescendant` |
| 每帧 | `LayoutTree / UpdateTree / DrawTree / RefreshThemeTree / CollectFocusables / HitTest` |
| 工具 | `EnsureVisible / EnsureRectVisible / ScrollPage / ScrollHost / FirstFocusable / SetFocusZone / RequestFocus / YieldFocus / Move` |
| 子类钩子 | `MeasureContent / OnDrawContent / OnDrawOverlay / OnUpdate / Activate / OnPadAction / OnAfterLayout / OnThemeChanged / BuildFocusVisual` |

子类只需要三件事：**量内容**（`MeasureContent`）、**画内容**（`OnDrawContent`）、**每帧状态**（`OnUpdate`）。

### 4.2 Box（`components/Box.h`）

最基础矩形，两种身份：

```cpp
Box* panel = Root().Emplace<Box>("panel");
panel->moveTo(20,20).resize(400,300).fillWith(Theme::kBgPanel).roundCorners(5);
panel->makeFocusable();               // 变成可聚焦控件（A 触发 clicked）
panel->focusVisual(1.04f);            // 聚焦时的缩放/位移/焦点框
```

### 4.3 Button 家族（`components/Button.h`）

同一套骨架（`icon` + `text` + `subtitle` + 右侧区域），7 种形态：

| 类型 | 用途 | 右侧内容 |
|---|---|---|
| `TextButton` | 纯文字（弹窗确认/取消），可带 `setFontSize` | — |
| `IconTextButton` | 图标 + 文字（图标占左侧正方形格） | — |
| `IconButton` | 纯图标，`setSide()` + `setShape(RoundedSquare/Circle)` | 说明行画在控件外侧 |
| `ToggleButton` | 开关，`toggled(bool)` | 拨动开关（轨道+旋钮动画） |
| `CustomButton` | 自定义右侧文字/颜色（`setRightText`） | 文字 |
| `OptionButton` | LR 选项选择器：`[L] 选项 [R]`，L/R 切换 | `[L] 值 [R]`，超长跑马灯 |
| `ValueButton` | LR 数值选择器：长按加速，松开才发 `valueChanged` | `[L] 值 [R]` |

通用能力：`setIcon/setText/setSubtitle/showSubtitle/setTextAlign/setFontSize/setTextColors/setBorder/
setCornerRadius/setShadow/setFlowingFocus/setContentPadding/resize/moveTo`；
信号：继承 Widget 的 `clicked` 等 + 各自的 `toggled/selectionChanged/valueChanged`。
颜色属性默认跟主题（`text_color_follows_theme` 等），显式设色后不再跟随。

### 4.4 Badge（`components/Badge.h`）

机种徽标（圆角矩形 + 居中短文本）。机种表：`EmuPlatform`（1=GBA … 14=Dolphin，0=Unknown），
`PlatformBadgeInfoOf(platform)` 给出文字与底色；`BadgeStyleOf(style)` 给 4 种变体的尺寸
（`GridListDetail / IisuCover / GameDataView / GridItem`）。默认高 26、字号 14、圆角 4（`setRadius(<0)` = 胶囊）。

```cpp
Badge* b = Root().Emplace<Badge>(EmuPlatform::GBA);
b->setHeight(Theme::kBadgeHeight).setFixedWidth(true).setMinWidth(92)
   .setColors(PlatformBadgeInfoOf(EmuPlatform::GBA).background, Theme::kWhite);
```

### 4.5 Header（`components/Header.h`）

区块标题：**左侧竖条 + 标题 + 右侧补充文字 + 底部分隔线**，用来把页面内容水平分段。

| 参数 | 默认 | 说明 |
|---|---|---|
| `height` | 58 | 整条高度（对齐 GBAStation SettingPage 的 section header） |
| `bar_width / bar_height / bar_radius / bar_offset_x` | 4 / 24 / 2 / 2 | 竖条 |
| `text_offset_x / text_size` | 18 / 20 | 标题 |
| `info_size / info_gap` | 14 / 12 | 右侧补充文字（放不下自动不画） |
| `divider*` | 1px，左缩进 18 / 右 6 / 离底 6 | 底部分隔线 |

```cpp
Header* h = Root().Emplace<Header>("显示设置");
h->setInfo("共 12 项").setBarColor(Theme::kAccent);   // 颜色默认跟主题
h->position = ImVec2(0,0); h->size.x = content_width; // 给宽度，分隔线就横跨整段
```

### 4.6 TabColumn（`components/TabColumn.h`）

左侧纵向标签列（当 Tab 用）。组合式实现：自己只做「单选 + 选中底 + 焦点分区 + 滚动」，每一项都是现成的 `TextButton`。

| 交互 | 行为 |
|---|---|
| ↑ / ↓ | 列内逐项移动；**焦点落到哪一项，选中就立刻跟到哪一项**（切焦点 = 切页面） |
| A / 点击 | `EnterContent()`：焦点交给 `setFocusTarget()` 指定的内容区入口控件 |
| → / R | 同上（键盘/手柄的翻页键也能进内容区） |
| 子页面按 B | 由页面调 `FocusCurrentItem()` 把焦点收回本列（demo 里的做法） |

```cpp
TabColumn* tabs = Root().Emplace<TabColumn>();
tabs->setItems({{Icons::Glyph(Icons::Material::Settings), "常规"},
                {Icons::Glyph(Icons::Material::Info), "关于"}});
connect(tabs, &TabColumn::selectionChanged, this, [](int i) { /* 切可见性 */ });
tabs->setFocusTarget(first_widget_of_page);   // A / R 的落点
```

`Style` 关键值：`item_height 56`、`item_gap 4`、`item_radius 5`（选中底圆角，`<=0` = 胶囊）、
`indicator_width 4`（左侧强调色条）、`focus_zone 1`（内容区请设成别的分区）、
动效 `enter_duration 0.28 / enter_stagger 0.025 / enter_offset -30 / focus_duration 0.16 / focus_offset 4`。
选中底**不做位移动画**（切到哪一项就贴哪一项）；入场逐项从左侧滑入 + 淡入；焦点响应用 `EaseOutBack` 轻微右移。

### 4.7 CapsuleTabs（`components/CapsuleTabs.h`）

横向胶囊标签条（学 GBAStation 游戏库顶部的机种轮播）：选中项停在条带中心，其余按中心距左右展开并衰减。

| 参数 | 默认 | 说明 |
|---|---|---|
| `spacing / capsule_width / capsule_height` | 132 / 104 / 42 | 几何（圆角 = 高一半 = 完全胶囊） |
| `font_min → font_max` | 17 → 22 | 越靠中心字号越大 |
| `alpha_min` / `fade_span` | 0.42 / 1.55 | 透明度衰减 / 超过多少个间距就不画 |
| `slide_speed` | 8 | 横滑速度（≈125ms 走完一格，位置套 `EaseOutCubic`） |
| `fill_alpha → fill_alpha_max`、`stroke_alpha → stroke_alpha_max` | 22→44、70→135（/255） | 胶囊填充与描边 |
| `shadow_offset / shadow_blur` | (3,3) / 5 | 胶囊阴影 |

交互：L/R 切换、点标签直接选中、A（或再点已选中）发 `activated`；信号 `selectionChanged(int)`。

### 4.8 Toast（`Toast.h`，不在组件树里）

页面级通知，`Page::Toasts()` 拿到管理器：

```cpp
Toasts().ShowSuccess("保存成功");
Toasts().ShowError("读取失败");
Toasts().ShowInfo("正在加载…");      // 连续调用会排队，不会互相覆盖
```

- 生命周期：入场 0.25s → 停留 3s → 退场 0.25s；X 轴滑动、Y 轴指数补位（后面的自动上移）
- 尺寸：宽 200~320 自适应，高最小 44（长文本自动换行加高），离右边缘 5px、顶部 20px
- 外观：卡片 = 左侧 3px 圆角 + 右侧直角、左侧 4px 状态色条（圆角 2）、Material 图标 + 正文
- `dedup_window = 0`：默认不去重（同一条连续触发会连续弹）；改 >0 可让同文案在窗口内只刷新
- 不参与焦点/命中（不会抢输入），画在组件树与焦点框之上

### 4.9 FocusRing（`FocusRing.h`，页面级图层）

焦点框从控件里**解耦**出来：控件只通过 `Widget::BuildFocusVisual()` 描述「我要什么焦点框」
（`FocusVisual{enabled, flowing, rect, radius, width, alpha, color, phase, saturation, brightness}`），
由 `Page` 每帧在最上层画**一个**：

| 行为 | 值 |
|---|---|
| 跟随 | 矩形/圆角 `SmoothTo` 26/s；目标中心跳 >180px 直接对齐（不横穿屏幕） |
| 淡入淡出 | alpha `SmoothTo` 22/s |
| 裁剪 | 自动裁到最近的滚动容器，不会画到面板外 |
| 两种形态 | 单色圆角框（`Widget` 默认，`focus_frame = true`）、流光闭合框（`Button`，`flowing_focus = true`） |

### 4.10 GlassBox（`components/GlassBox.h`）

液态玻璃（Liquid Glass）风格的浮层，可鼠标/触摸拖动。**是近似效果**：`ImDrawList` 没有片元着色器、
采样不到背后帧缓冲，所以「模糊 / 真折射 / 饱和增强」做不到，这里用可用的手段拼：

| 层 | 做法 |
|---|---|
| 主体 | 冷色 veil + 白雾两层叠出「厚度」 |
| 顶部高光 | 3 层从亮到透的圆角矩形（保形圆角，不露方角） |
| 镜面光斑 | 多层椭圆叠出软边；拖动时按速度**反向甩开**（液面晃动），松手指数回正 |
| 边缘透镜带 | 外沿压暗 1px + 内侧亮线 + 两圈边缘雾气 |
| 文字 | `setLabel()`（左上）+ `setHint()`（居中） |

```cpp
GlassBox* glass = Root().Emplace<GlassBox>("液态玻璃");
glass->setLabel("Liquid Glass").setHint("按住拖动我");
glass->resize(420, 250);
glass->SetZOrder(100);                          // 盖在页面内容之上
glass->position = ImVec2(440, 170);
connect(glass, &GlassBox::movedTo, this, [](ImVec2 p) { /* 记录位置 */ });
```

- 拖动：按住拖动，位置夹在画布内（`draggable = false` 可关）；`isDragging()` 查询状态；`movedTo(ImVec2)` 信号
- **不参与焦点**（`focusable = false`），不会抢手柄导航
- 主题：深色主题偏冷白、浅色主题偏乳白；圆角默认 18（玻璃要显厚）
- 想要真折射需要后端支持：渲染到 FBO → 模糊 → UV 位移 shader。Switch 的 GL 可以；
  mac 的 `SDL_Renderer` 只能用「多次降采样 + 线性过滤」近似（或直接用模拟器的帧纹理做背景采样）

### 4.11 GlassTabs（`components/GlassTabs.h`）

液态玻璃风格的**底部 Tab 条**：抓背景 → 模糊 → 折射 → 玻璃材质 → 高光/边缘 → 内容，
用 ImGui 现有能力实现，**不需要着色器**（原理与成本见下）：

| 步骤 | 实现 |
|---|---|
| 抓背景 | `setBackdrop(纹理, 它在画布上的矩形)` —— 模拟器传游戏帧纹理，demo 传背景图 |
| 模糊 | 对这张纹理做 **13 抽头环形核**的带偏移采样（`AddImageRounded`），半径 `blur_radius` |
| 折射 | 从边缘往里画 3 圈「UV 略微放大」的采样（越靠边放大越多、权重越低）→ 边缘背景被掰弯，中心不变 |
| 材质 | 染色 + 白雾 + 顶部高光 + 镜面光斑（带拖动晃动）+ 内亮边 1.4px + 外暗边 1px |
| 内容 | 图标 + 文字 + 选中胶囊（`capsule_slide > 0` 才滑动，默认直接切） |

```cpp
GlassTabs* tabs = Root().Emplace<GlassTabs>();
tabs->setItems({{Icons::Glyph(Icons::Material::VideogameAsset), "游戏"},
                {Icons::Glyph(Icons::Material::Save), "存档"},
                {Icons::Glyph(Icons::Material::Settings), "设置"}});
tabs->size = ImVec2(760, 104);
tabs->setBackdrop(ui.GetBackend().LoadTexture("img/xxx.png"), canvas_rect); // 或游戏帧纹理
connect(tabs, &GlassTabs::selectionChanged, this, [](int i) { /* 切页 */ });
```

- 交互：← / →（或 L / R）切 Tab、点 Tab 直接选中、`draggable` 拖动（`movedTo` 信号）；`focus_only_self` + `capture_horizontal`，↑/↓ 仍可离开
- 成本：模糊抽头 × 采样数（13 次贴图四边形，仅在玻璃区域内）＋ 3 圈折射采样；比一次全屏高斯便宜得多
- 想要真·采样当前帧缓冲（含几何/文字）需要后端离屏渲染 + UV 位移 shader；这条路线在 Switch 的 GL 上可行，
  mac 的 `SDL_Renderer` 不行 —— 所以当前设计是「宿主把背后那张纹理给我」

### 4.12 Page（`pages/Page.h`）

页面基类：持有根 `Box`、`ToastManager`、`FocusRing`，并提供每帧流程与焦点自动滚动。

```cpp
virtual const char* Title() const = 0;
virtual void OnBuild();        // 一次
virtual void OnInput();        // 页面级按键（配合 Global::Available / MarkConsumed）
virtual void OnUpdate(float dt);
virtual void OnOverlay(ImDrawList*);   // 画在组件树之上、焦点框之下
Box& Root();  ToastManager& Toasts();  void RefreshTheme();  UiContext& ui();
```

---

## 5. 动画规范

### 5.1 工具（`component_view/Anim.h`）

| 函数 | 用途 |
|---|---|
| `SmoothTo(current, target, speed, dt)` | 指数趋近（帧率无关、不过冲）：跟随、透明度、滚动 |
| `MoveTowards(current, target, duration, dt)` | 定时长线性推进：焦点切换 120~220ms、按压 80~150ms |
| `AdvanceOnce(current, duration, dt)` | 0→1 一次性推进（走完停住） |
| `EaseOutCubic / EaseOutBack` | 缓出 / 带轻微过冲（「弹一下」） |
| `Clamp01` | 夹到 0..1 |
| `StaggerElapsed(elapsed, index, stagger, duration)` + `StaggerTotal(count, stagger, duration)` | 逐项错开：**按已播秒数算**，总时长要含错开尾巴，否则最后几项永远到不了 1 |

> 语义与 `framework/gamemenu/MenuAnimation.h` 一致（那套是游戏内菜单专用的），组件库用自己的这份。

### 5.2 各组件动效一览

| 组件 | 时长 | 曲线 | 内容 |
|---|---|---|---|
| TabColumn 入场 | 0.28s + 25ms/项 | EaseOutCubic | 从左滑入 30px + 淡入 |
| TabColumn 焦点响应 | 0.16s | EaseOutBack | 右移 4px + 文字提亮 |
| TabColumn 选中底 | — | 无位移 | 直接在选中项上（需求：背景不做移动动画） |
| CapsuleTabs 横滑 | ≈0.125s（8/s） | EaseOutCubic | 整条横滑一格 + 胶囊淡入 |
| Toast 出入场 | 0.25s / 0.25s | EaseOutCubic（入） | X 滑动 + alpha；Y 用指数补位 |
| ToggleButton 拨动 | 指数平滑 | — | 轨道颜色 + 旋钮位移 |
| 焦点框 | 跟随 26/s | — | 矩形/圆角/透明度平滑 |
| GlassBox 液面晃动 | 回正 7.5/s | SmoothTo | 高光按拖动速度反向偏移，松手回正 |
| 按压 | `visual_scale` | — | 宿主/子类自己设（Widget 提供 `visual_scale/translate`） |

### 5.3 淡入淡出靠什么

`Widget::opacity`（0..1）现在对**背景/边框/文字/图标/焦点框**同时生效（`EffectiveOpacity()`），
所以整页/整块的淡入淡出只需要改 `opacity` + `visual_translate`，不用逐个子控件改颜色。

---

## 6. 输入与焦点

### 6.1 动作（`framework/platform/Input.h`）

`Up Down Left Right Confirm Cancel ActionX ActionY Menu Minus PageLeft PageRight TriggerLeft TriggerRight`

按键映射（SDL2 后端）：

| 动作 | 键盘 | 手柄 |
|---|---|---|
| Confirm / Cancel | Enter/Space / Esc/Backspace | 面键 A / B（**任天堂手柄自动对掉**，见下） |
| ActionX / ActionY | X / Y | 面键 X / Y |
| Menu / Minus | Tab 或 = / - | Start / Back |
| PageLeft / PageRight | Q / E | L1 / R1 |
| TriggerLeft / TriggerRight | Z / C | ZL / ZR |

**面键布局**：SDL 的手柄 API 按「位置」报键（A=下、B=右、X=左、Y=上，即 Xbox 习惯），
而 Switch 手柄机身上印的是 A=右、B=下、X=上、Y=左。后端识别到任天堂手柄（或 switch 平台）时会把
A/B、X/Y 对掉，保证「按机身上的 A」= 确认。可用 `GUI_DEV_FACE_SWAP=1/0` 强制覆盖，启动时会打一行日志。

### 6.2 焦点模型

- **Focus 是一等状态**：`Global::focused`，每帧 `root_->CollectFocusables()` + `Global::NavigateFocus()` 处理方向键。
- **焦点分区**（`focus_zone`）：同分区内按「方向最近邻」移动；**只有左右方向会跨分区**。
  典型用法：左列 `focus_zone = 1`，内容区 `= 2` → ↑↓ 留在列内，→ 进内容、← 回列表。
- **控件自己吃方向键**：`capture_horizontal / capture_vertical = true`（列表、滑条、虚拟键盘用）。
- **复合控件**：`focus_only_self = true`（只把自己当停靠点，内部导航自己做）。
- **按键消费**：一帧里同一个键只能被消费一次。页面级快捷键要写
  `if (Global::pad.Pressed(a) && Global::Available(a)) { …; Global::MarkConsumed(a); }`。
- **焦点自动滚动**：`Page::Update` 检测焦点变化后调 `root_->EnsureVisible(focused)`，任何滚动容器都受益。

### 6.3 鼠标 / 触摸

- 鼠标位置换算到逻辑画布：`window 点 × 逻辑尺寸 / 窗口尺寸`，鼠标和触摸走同一个换算（`Sdl2Backend::WindowToLogical`）。
- 桌面上 `focus_on_hover = true` 的控件悬停即接管焦点；触摸则通过 SDL_FINGER* → ImGui 鼠标事件。
- 命中测试自己实现（子节点优先 + `z_order` 高者优先），不借用 ImGui 的 item 机制。

---

## 7. 绘制工具（写新组件时用，`Draw.h`）

`RoundedRectFilled / RoundedRectOutline / SoftShadow / ComponentBox / MeasureText / Text / TextOutlined /
Ellipsize / MarqueeText / FlowingRing / Hsv / CheckMark / TriangleRight / RoundedRectVerticalGradient / CurrentFont`

- `ComponentBox(dl, rect, BoxVisual)`：阴影 + 底色 + 边框一次画完，Box/Button/Toast 共用同一套框。
- `SoftShadow`：多层环带逼近高斯模糊，**每层只画自己那一环**（填充量 O(周长×环宽)，面板级阴影也不会拖垮填充率）。
- `FlowingRing`：沿圆角矩形按弧长采样，颜色随相位流动（焦点框用）。

---

## 8. 调试开关（环境变量）

| 变量 | 作用 | 属于 |
|---|---|---|
| `GUI_DEV_WINDOW=WxH` | 指定窗口尺寸 | demo |
| `GUI_DEV_ZOOM=0.5~3` | 启动缩放（等同 `kDefaultZoom`） | demo |
| `GUI_DEV_NO_VSYNC=1` | 关垂直同步（测帧率用） | demo |
| `GUI_DEV_MAX_FPS=30/60/0` | 帧率上限（0 = 不限；Switch 默认 60） | demo / `BackendConfig::max_fps` |
| `GUI_DEV_THEME=dark` | 深色启动（截图核对两套主题） | demo |
| `GUI_DEV_PERF=1` | 每秒打印 fps / 帧时间 / drawcall / 顶点数 | demo |
| `GUI_DEV_TRACE_SIGNAL=1` | 打印按钮状态变化（脚本化验收） | demo |
| `GUI_DEV_FACE_SWAP=1/0` | 强制手柄面键是否按任天堂布局 | 后端 |
| `GUI_DEV_TRACE_TOUCH=1` | 打印手指事件（归一化 → 窗口点 → 逻辑坐标） | 后端 |
| `GUI_DEV_EXIT_AFTER=n` | 跑 n 帧后自动退出 | demo |
| `GUI_DEV_DEBUG_LAYOUT=1` | 打印布局信息 | 框架 |
| `GUI_DEV_TOUR_TAB=n` | 指定 imgui_tour 的初始 Tab | imgui_tour |

---

## 9. 现状与缺口

### 9.1 现在有什么（可用于新项目）

盒子/面板（`Box`）、按钮 7 形态（`Button` 家族）、机种徽标（`Badge`）、区块标题（`Header`）、
纵向 Tab 列（`TabColumn`）、横向胶囊标签条（`CapsuleTabs`）、液态玻璃浮层（`GlassBox`）、
液态玻璃 Tab 条（`GlassTabs`）、通知（`Toast`）、
焦点框图层（`FocusRing`）、页面/布局/滚动/焦点/主题/动画基础设施（`Widget / Page / Global / Theme / Anim / Draw`）。

### 9.2 还没有的（别的项目若需要，要补）

`Label`（纯文本/多行）、`Image`、`ImageButton`、`Checkbox`、`RadioGroup`、`Slider`、`Progress`、
`List`（列表/列表行）、`ScrollBox`（独立滚动容器）、`Menu`、`InputField`、`VirtualKeyboard`、`Dialog`。

这些在**历史提交 `35ec70d`**（"回调改为 Qt 风格信号槽"）里有实现，且当时的 `Widget` API 与现在基本一致
（`component_view/components/` 下 16 个组件 + `pages/` 下的展示页）。所以是**移植适配**（过一次编译 + 视觉对齐
+ 换成现在的 `Theme` 角色色与 `Draw` 原语），不是从零重做：

```bash
git show 35ec70d:component_view/components/Slider.h     # 看旧实现
git diff 35ec70d -- component_view/Widget.h             # 对比新旧 Widget API
```

---

## 10. 复用结论（能不能直接搬）

**能**，条件是带上第 2.1 节那三样（代码 / 字体资源 / imgui+SDL2）：

| 项目 | 结论 |
|---|---|
| 目录结构 | `component_view/` 与业务完全解耦，演示、示例、测试都在它外面；`demo.cpp` 只是其中一个使用者 |
| 构建 | 已是独立静态库 `gui_dev_components`；`examples/min_demo` 是一个 150 行的完整接入样板（`imgui_tour` 也链接了这个库，只用 `Theme` 与 ImGui 原生控件混排） |
| 依赖 | 只依赖 `framework/` 的输入抽象、图标表、宿主桥接与主循环 —— 这几块本来就是「模拟器家族共用引擎」 |
| 主题/尺寸 | 全部走 `Theme` 角色色与 720p 尺寸常量，没有业务色值、没有「GBAStation 专用」分支 |
| 动画 | `Anim.h` + 各组件自带时长；`pause_menu` 的 `gamemenu/MenuAnimation.h` 与组件库互不依赖（Toast 已改为用组件库这份） |
| 缺口 | 输入类组件（虚拟键盘/输入框/对话框/列表/滑条…）不在当前库内，见 9.2，按需从 `35ec70d` 移植 |
| 未验证 | Switch 真机（触摸校准、手柄面键、帧率上限的实际观感）需要在设备上过一遍 |

---

## 11. 环境与验收

```bash
# 构建
cmake --preset mac && cmake --build --preset mac            # mac Debug
cmake --build --preset mac-release                          # mac Release
cmake --build --preset switch                               # Switch（/opt/devkitpro）
ctest --test-dir build/mac                                  # 信号槽语义测试

# 跑演示
./build/mac/gui_dev_demo                  # 组件总览（tab 列 + 4 个子页面）
./build/mac/gui_dev_liquid_glass_demo     # 液态玻璃 Tab 条（拖动/切 Tab 观察模糊与折射）
./build/mac/gui_dev_min_demo              # 最小接入示例
GUI_DEV_PERF=1 GUI_DEV_MAX_FPS=60 ./build/mac/gui_dev_demo   # 性能统计
```

改动一律要求：三套目标（mac Debug / mac Release / Switch）编译通过 + `ctest` 通过 + 视觉/交互按上面各节的默认值核对。
