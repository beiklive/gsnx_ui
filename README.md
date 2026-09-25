# GUI_DEV

GBAStation 模拟器家族的**统一前端组件库**。各模拟器核心共用同一套 UI 代码与视觉规范，
避免每个核心各写一份界面。

- UI 框架：Dear ImGui（git submodule，锁定 `v1.92.9b`，master 稳定线）
- 窗口/渲染：SDL2（mac 与 Switch 共用同一份后端实现）
- 工具链：mac 用系统 clang + homebrew `sdl2`；Switch 用 `/opt/devkitpro`（devkitA64 + libnx）

📖 **组件库参考文档：[`docs/component-view.md`](docs/component-view.md)** —— 组件 API、主题/尺寸规范、
动画时长、输入与焦点约定、调试开关、以及「搬进别的项目」的完整清单。
最小接入样板：[`examples/min_demo/main.cpp`](examples/min_demo/main.cpp)。

## 目录结构

```text
GUI_DEV/
├── CMakeLists.txt              # imgui / gui_dev_backend / gui_dev / gui_dev_components + 演示/示例/测试目标
├── CMakePresets.json           # mac / mac-release / switch 预设
├── demo.cpp                    # ★ 演示入口：在这里登记 component_view 的页面
├── cmake/toolchains/
│   └── Switch.cmake            # devkitA64 工具链入口（含 ar 修正）
├── framework/                  # ★ 框架层（引擎；component_view 建立在它之上）
│   ├── core/                   # App 基类 + AppRunner 主循环
│   ├── ui/                     # UiContext / Theme / Icons / Texture / Scene / Components
│   ├── platform/               # Backend 接口 + 抽象输入 + SDL2 后端
│   └── gamemenu/               # 暂停菜单 UI 层（Persona 式视觉语言）
├── component_view/             # ★ 组件库（业务无关，可整体搬进别的项目）
│   ├── Object.h                # ★ Qt 风格信号槽：Object / Signal / connect / emit
│   ├── Global.{h,cpp}          # 全局变量：画布 / 鼠标 / 手柄 / 焦点 / 分区 / 输入消费
│   ├── Theme.{h,cpp}           # 调色板（浅色/深色两套，运行时可切）与 720p 尺寸规范
│   ├── Types.h                 # Rect / EdgeInsets / BorderStyle / ShadowStyle / BoxVisual / Transform2D
│   ├── Anim.h                  # 动画工具：SmoothTo / MoveTowards / EaseOutCubic / EaseOutBack / Stagger*
│   ├── Draw.{h,cpp}            # 绘制原语：圆角矩形 / 软阴影 / 描边文字 / 省略号 / 流光框 / 跑马灯
│   ├── Widget.{h,cpp}          # ★ 父类：坐标 / 尺寸 / 圆角 / 边框 / 阴影 / 溢出滚动 / 焦点动画 / 事件
│   ├── FocusRing.{h,cpp}       # ★ 焦点框图层：控件只描述，页面每帧统一画一个
│   ├── Toast.{h,cpp}           # ★ 通用通知：生命周期 / 队列 / 滑入滑出 / 多 Toast 自动补位
│   ├── components/             # Box / Button(7 形态) / Badge / Header / TabColumn / CapsuleTabs / GlassBox
│   └── pages/                  # Page 基类（Demo 宿主）
├── examples/                   # 示例（与组件库互不依赖）
│   ├── min_demo/               # ★ 最小接入示例：另一个项目要写的全部代码
│   ├── imgui_tour/             # ★ ImGui 自身能力导览（8 个 Tab，页面不滚动）
│   ├── flow_box/               # framework/ui 组件预览（流光焦点框）
│   ├── pause_menu/             # 暂停菜单 Demo（Persona 式动态菜单）
│   └── widget_lessons/         # 自定义控件 8 例
├── tests/
│   └── qt_signal_test.cpp      # 信号槽语义测试（ctest）
├── third_party/imgui/          # submodule
├── docs/                       # 组件文档（component-view.md）+ 界面快照
├── assets/
│   ├── font/                   # switch_font.ttf / switch_icons.ttf / MaterialIcons-Regular.ttf
│   └── img/                    # UI 图片（border_gradient.png）
└── build/                      # 构建产物（已 gitignore）
```

## 构建

### macOS（主要开发环境）

```bash
cmake --preset mac
cmake --build --preset mac
./build/mac/gui_dev_imgui_tour    # ★ ImGui 原生能力导览（8 Tab：总览/输入/布局/弹层/高级/绘图/字体样式/系统工具）
./build/mac/gui_dev_demo          # ★ 手柄优先控件库 demo（左 16 个 Tab + 右展示区）
./build/mac/gui_dev_flow_demo     # framework/ui 组件预览（可聚焦 Box / 流光边框）
./build/mac/gui_dev_pause_demo    # 暂停菜单 Demo（Persona 式动态菜单）
./build/mac/gui_dev_widget_demo   # 自定义控件 8 例
```

依赖：`brew install sdl2 libpng`（`sdl2-compat` 也可）。预设里显式指定了
`PKG_CONFIG_EXECUTABLE=/opt/homebrew/bin/pkg-config`，否则会命中排在 PATH 前面的
devkitPro pkg-config（它只认 Switch portlibs）。

### Switch

```bash
cmake --preset switch
cmake --build --preset switch
# 产物：build/switch/dist/gui_dev_demo.nro
```

需要 devkitPro 环境变量（默认 `/opt/devkitpro`）与 `switch-sdl2`、`switch-pkg-config`。

## 架构约定

**单向依赖**（重要，破坏它会直接链接失败）：

```text
demo.cpp  ->  gui_dev_components  ->  gui_dev  ->  gui_dev_backend  ->  imgui
             (component_view/)      (framework/)
```

- `framework/ui/` 与 `framework/core/` **禁止** include SDL/GLFW/libnx 等平台头文件，平台能力一律走
  `gui_dev::Backend`。
- `component_view/` 只依赖 `gui_dev`（`UiContext` / `TextureRef` / `Backend` / `Icons`），
  不依赖任何 `examples/`，因此 mac 与 Switch 共用同一份组件代码。
- `gui_dev_backend` **禁止**调用 `gui_dev` 里的符号（`Theme::Apply()` 因此放在
  `AppRunner` 而不是后端里）。两个静态库互相引用会形成链接环：GNU ld 单遍扫描，
  先出现的一方必然解析失败。

**新增平台**：在 `framework/platform/backends/<name>/` 实现 `Backend` + `CreatePlatformBackend()`，
然后在 CMake 的 `GUI_DEV_BACKEND` 分支里加一个选项。`framework/ui` 一行不用改。

**新增界面**：两种方式——用 `framework` 的 `Scene` + `Components::*`；
或者用 `component_view` 的 `Page` + `Widget` 组件树（见下节）。

### 生命周期约定（曾因此崩溃）

场景常持有 `TextureRef` / 字体等后端资源，而 `Backend` 由 `AppRunner` 持有。
**持有后端资源的对象必须先于 `Backend` 析构**，否则退出时会对已释放的 Backend
调 `ReleaseTexture()` → `SIGSEGV`（`AppRunner::Run()` 内部因此显式
`app_.Scenes().Clear()`，就在 `ui_.reset()` / `backend_->Shutdown()` 之前）。

双保险：`BackendLiveness`（`Backend.h`）是挂在 Backend 上的存活标记，
`TextureRef` 一并保存它；Backend 一析构标记即置 false，之后释放纹理只会打警告、
不会崩：

```text
[gui_dev] 纹理在后端销毁之后才释放，已跳过（GPU 资源泄漏）。请确保持有纹理的对象先于 Backend 析构。
```

### 退出路径冒烟测试

`timeout` 杀进程走不到析构，因此专门留了正常退出入口：

```bash
GUI_DEV_EXIT_AFTER=60 ./build/mac/gui_dev_demo   # 跑满 60 帧后正常退出，退出码应为 0
```

配合 `Backend::RequestQuit()`（UI 里的「退出」入口也用它）。

## component_view 组件库（重建中）

组件库已清空重做，现在有两层：**Box（矩形底 / 容器 / 可聚焦控件）** 和 **Button（7 种形态）**。

```bash
cmake --preset mac && cmake --build --preset mac
./build/mac/gui_dev_demo
```

保留下来的是「地基」，组件从 Box 开始一个个往上长：

| 文件 | 作用 |
|---|---|
| `component_view/Object.h` | Qt 风格信号槽（`Object` / `Signal<Args...>` / `connect` / `emit`） |
| `component_view/Types.h` | `Rect` / `EdgeInsets` / `BorderStyle` / `ShadowStyle` / `Transform2D` / 布局枚举 |
| `component_view/Theme.{h,cpp}` | VSCode Dark+ 调色板（**RGBA/ImVec4**）+ 720p 手持尺寸基准 + `Theme::ApplyToImGui()` |
| `component_view/Anim.h` | 动画工具：`SmoothTo` / `MoveTowards` / `EaseOutCubic` / `EaseOutBack` / 逐项错开 |
| `component_view/Draw.{h,cpp}` | 绘制原语：圆角矩形 / 软阴影 / 文本 / 描边 / 省略号 / 流光框 / 跑马灯 |
| `component_view/Widget.{h,cpp}` | 父类：坐标 / 尺寸 / 圆角 / 边框 / 阴影 / 溢出滚动 / 焦点动画 / 命中测试 / 输入分发 |
| `component_view/Global.{h,cpp}` | 全局变量：画布 / 鼠标 / 触摸 / 手柄 / 焦点 / 分区 / 输入消费 / 约定样式 |
| `component_view/components/Box.{h,cpp}` | 矩形底 + 圆角 / 边框 / 阴影；可当容器，也可 `makeFocusable()` 当控件 |
| `component_view/components/Button.{h,cpp}` | 按钮 7 种形态（见下节） |
| `component_view/components/Badge.{h,cpp}` | 机种徽标（14 个机种 + 其它，4 种尺寸变体） |
| `component_view/components/Header.{h,cpp}` | 区块标题：竖条 + 标题 + 右侧补充文字 + 分隔线 |
| `component_view/components/TabColumn.{h,cpp}` | 左侧纵向 Tab 列（焦点即切页、A 进内容、焦点框分层） |
| `component_view/components/CapsuleTabs.{h,cpp}` | 横向胶囊标签条（中心高亮 + 距离衰减） |
| `component_view/FocusRing.{h,cpp}` | 焦点框图层：`Widget::BuildFocusVisual()` 描述，页面统一绘制 |
| `component_view/components/GlassBox.{h,cpp}` | 液态玻璃浮层（近似）：多层半透明 + 高光/边缘透镜带 + 拖动液面晃动 |
| `component_view/pages/Page.{h,cpp}` | 页面基类：root Box 铺满画布 + 布局/命中/更新/绘制 |
| `demo.cpp` | 演示入口：登记页面、每帧驱动、三个调试开关 |

### 现在的 demo 长什么样

左列 6 个行内按钮 + 右上角一个可聚焦 Box + 右下角两个纯图标按钮（圆角正方形 / 圆形）
+ **窗口最右侧的控制列**（从上往下排控制按钮：主题 / 成功 / 失败 / 信息 / 长文本）：

```cpp
// demo.cpp
void OnBuild() override {
    TextButton* plain = Root().Emplace<TextButton>("普通按钮");  // 弹窗提示文字：不带说明行
    plain->moveTo(20.0f, 20.0f);
    plain->resize(396.0f, 52.0f);

    IconButton* circle = Root().Emplace<IconButton>(Icons::Glyph(Icons::Material::Favorite));
    circle->setSide(76.0f);                          // 只有圆角正方形 / 圆形两种形态
    circle->setShape(IconButtonShape::Circle);
    circle->setSubtitle("圆形");                      // 有说明行时落到图标下方居中
    circle->moveTo(526.0f, 166.0f);

    box_ = Root().Emplace<Box>("box");               // 可聚焦容器
    box_->moveTo(440.0f, 20.0f);

    // 右侧控制列：贴右边缘 20px，从上往下排（主题 / 放大 / 缩小）
    theme_button_ = AddControl(Icons::Material::DarkMode, "btn_theme", ThemeName());
    connect(theme_button_, &IconButton::clicked, this, [this] { ToggleTheme(); });

    zoom_in_ = AddControl(Icons::Material::ZoomIn, "btn_zoom_in", "放大");
    connect(zoom_in_, &IconButton::clicked, this, [this] { StepZoom(+1); });

    zoom_out_ = AddControl(Icons::Material::ZoomOut, "btn_zoom_out", "缩小");
    connect(zoom_out_, &IconButton::clicked, this, [this] { StepZoom(-1); });
}
```

### UI 缩放（启动时设定）

界面的物理缩放 = 后端自动缩放 × 用户倍率。自动缩放由分辨率算（`min(h/720, w/1280)`），
用户倍率在**启动时**设定（720p 手持基准下 1.0 显得小，demo 默认 **1.25**）：

```cpp
ui.SetUiZoom(1.25f);                          // 0.5..3.0，1.0 = 不额外缩放；OnStart 里调
GUI_DEV_ZOOM=1.25 ./build/mac/gui_dev_demo    // 调试用
ui.UiZoom();                                  // 当前倍率
```

逻辑画布 = `drawable / (自动缩放 × 倍率)`，所以倍率变大 = 画布变小 = 界面变大；
字体光栅化密度只跟分辨率走（不含倍率），字形按需烘焙，不需要也不应该运行期重建图集。

> **为什么没有运行期缩放按钮**：运行期改 `SDL_RenderSetScale` 会让 SDL 的几何/视口状态错乱。
> mac 上的 SDL 是 sdl2-compat（2.32.72，SDL2 API 跑在 SDL3 上），实测点一次放大后 20 帧内必崩
> （Metal：`AGX: Texture read/write assertion failed … Region width OOB`）；把 SDL 缩放固定住、
> 或跳过 ImGui 绘制，都不再崩 —— 说明崩在 SDL 的缩放路径上（sdl2-compat 的
> [SDL_RenderSetScale 兼容性问题 #425](https://github.com/libsdl-org/sdl2-compat/issues/425)），
> 不是组件库的代码。Switch 端的运行期缩放也可能踩到同类问题，所以统一改成启动时设定，
> 界面上不再提供运行期缩放按钮。要在设备上运行期缩放的话，需要换真实 SDL2 或自己实现缩放几何的渲染路径。

### Toast 通知

业务代码只碰这几个函数，坐标 / 动画 / 生命周期 / 排列 / 图标 / 颜色都在 `ToastManager` 里：

```cpp
Toasts().ShowSuccess("保存状态成功");   // Page 里的入口
Toasts().ShowError("读取状态失败");
Toasts().ShowInfo("正在加载游戏...");
Toasts().Show(ToastType::Info, "任意文案");
```

视觉上就是「一个从右边滑进来的 Button Box」：底色 / 圆角 / 阴影 / 字体 / 内边距**全部复用**
`Global::component_style`（和 Button 走同一个画法函数 `Draw::ComponentBox()`），
Toast 只额外画左侧状态色条 + Material 图标 + 文本。

边框 / 圆角 / 阴影都来自 `Global::component_style`，和 Button 完全一致（`Draw::ComponentBox()`）。

| 项 | 值（`ToastStyle`，可改） |
|---|---|
| 边框 | 走 `Global::component_style`（1px 灰白，和 Button 同一套） |
| 底色 / 圆角 / 阴影 | 走 `Global::component_style`（和 Button 同一套） |
| 左侧状态色条 | 直角长条，左/上/下离边框 2px；Success 绿 / Error 红 / Info 蓝（颜色只用于色条和图标） |
| 图标 | Material `check_circle` / `error_outline` / `info`，22px |
| 尺寸 | 宽 200~320 自适应、最小高 `kControlHeight`(56)，长文本自动换行并按行数增高 |
| 位置 | 右上角：顶部 20、右边 5、多个 Toast 间距 10 |
| 时长 | 入场 0.25s（EaseOutCubic）→ 停留 **3s**（从入场完成开始算）→ 出场 0.25s → 删除 |

关键机制（也是没有「删除元素后坐标错乱」的原因）：

```text
X 轴：Entering 时 slide 0 → 1（EaseOutCubic）；Exiting 时 1 → 0（exponentialIn 风格）并 alpha = slide 淡出
Y 轴：每帧 targetY = 上一条的 y + 上一条的 height + spacing；
      currentY = SmoothTo(currentY, targetY, reflow_speed, dt)
```

两个轴完全独立，所以「A 正在向右退出 / B、C 正在向上补位 / D 正在从右边进入」可以同时发生；
顶部 Toast 消失后，后面的自动向上补位 —— 不需要 Toast 之间互相知道坐标。

- 去重：**默认关闭**（`dedup_window = 0`，每次 Show 都建一条）；设成 >0 时同类型 + 同文案在该窗口内
  只刷新停留时间、不新建（正在退出的会被拉回来重新滑入）。
- 不接管输入：Toast 不在 Widget 树里，不参与命中测试 / 焦点导航 / 手柄分发（`Page::Update` 里只推进动画）。
- 绘制层级：`Page::Render` 里画在页面内容与 overlay **之后**，而 `Global::draw_list` 是 ImGui 前景 draw list → 高于所有 ImGui 窗口。
- 线程：Toast 是 UI 层服务，`Show*` / `Update` / `Draw` 都在 UI 线程（本项目 UI 单线程）。以后真有后台线程要发通知，
  再在 `Show*` 前面挂一个线程安全队列由 UI 线程排空即可。

demo 的右侧控制列加了四个触发按钮（成功 / 失败 / 信息 / 长文本），长文本那条用来验证换行和后续补位。

### 主题：浅色 / 深色（运行时整套切换）

调色板里的「角色色」都是运行时可变的变量，`Theme::SetMode()` 会把整套颜色换掉：

```cpp
Theme::SetMode(Theme::ThemeMode::Dark);   // 深色：页面 #1E1E1E + 深色控件 + 浅色文字
Theme::SetMode(Theme::ThemeMode::Light);  // 浅色：页面 #FFFFFF + 浅灰控件 + 深色文字
Theme::ToggleMode();                      // 一键互切
Theme::IsLight();                         // 当前是不是浅色
```

切换分三步，顺序别错：

```cpp
Theme::ToggleMode();        // 1. 换调色板
Theme::ApplyToImGui();      // 2. 让 ImGui 原生控件跟着走（WindowBg / FrameBg / 文字色…）
page.RefreshTheme();        // 3. 组件树重新取色
```

`Page::RefreshTheme()` = `Global::ApplyTheme()`（约定边框色/阴影浓淡）+ 根节点装饰复位 +
`Widget::RefreshThemeTree()`（递归调用每个组件的 `OnThemeChanged()`）。

组件怎么跟主题：

| 情况 | 行为 |
|---|---|
| 组件默认色（`Box` 底色、`Button` 底色/文字色、`ToggleButton` 开关色、焦点框色） | 自动跟着主题变 |
| 用户显式设过的颜色（`Box::fillWith` / `Widget::SetBackground` / `Button::setTextColors` / `ToggleButton::setSwitchColors`） | 固定住，切主题不动 |
| 自己写的组件要跟主题 | 重写 `Widget::OnThemeChanged()`，在那里重新取 `Theme::kXxx` |

新增主题相关颜色时，把它放进 `Theme::SetMode()` 的两套值里即可；`Theme::kBgEditor / kBgWidget /
kTextPrimary / kAccent / kControlBorder / kSwitchOff / kSwitchKnob` 等都是这样切换的。

> `framework/ui/Icons.h` 里新增了 `LightMode`(U+E518) / `DarkMode`(U+E51C) 两个 Material 图标，
> 主题切换按钮用它。字形自检（启动时打印）会核对覆盖率：现在是 16 个按键图标 + 37 个 Material 图标。

### 字号与控件尺寸统一（对齐 GBAStation SettingPage）

尺寸只有一处定义：`component_view/Theme.h` 的尺寸常量；组件（Box / Button / Badge / Toast）与 demo
都引用它，不再各写一套字面量。基准抄的是 GBAStation `src/ui/page/SettingPage.cpp` 那套手持尺寸：

| 角色 | 值 | SettingPage 里的对应 |
|---|---|---|
| 大标题 / 卡片标题 | `kFontTitle` = 22 | 捕获卡片标题 26、区块标题 20 之间 |
| 区块标题 | `kFontHeader` = 20 | `SettingsSectionHeaderView` 的 `nvgFontSize(vg, 20.f)`，行高 58 |
| 正文（按钮主文字 / Toast 正文） | `kFontBody` = 16 | 页面里最常用的字号（16 出现 15 次） |
| 说明行 / 提示 / 徽标文字 | `kFontSmall` = 14 | `makeHint` 的 `setFontSize(14.f)`，边距 4/10/16/16 |
| 极小号 | `kFontTiny` = 12 | 次要标注 |
| 按钮 / 列表行高 | `kControlHeight` = 56 | 标题行 58、卡片行 54~60 → 取中间值 |
| 标签（机种徽标）高 | `kBadgeHeight` = 26 | 徽标/标签量级 |
| 内容留白 | `component_style.content_padding` = 12 | hint 左右 16、容器 padding 12/24/24/24 |

于是所有控件一眼同高：左列按钮 56、图标按钮 56×56（边长 = 行高）、右侧控制列 56、
Toast 最小高 56；正文统一 16、说明行统一 14、徽标统一 26 高 + 白字。
实测（1.25 倍缩放下量像素）：左列/控制列每个按钮 56 高、间距 10；Toast 高 56；
徽标 92×26 两列、行距 32。

### 焦点与导航（Box 既能当容器，也能当控件）

一个 Box 有两种身份，按需要开关：

```cpp
// 容器：什么都不用调。它只负责位置/背景/子节点排版，焦点落在子节点上
Box* panel = parent->Emplace<Box>("panel");

// 可聚焦控件：方向键能选中它，A/回车/鼠标左键触发 clicked 信号
Box* card = parent->Emplace<Box>("card");
card->makeFocusable();          // = focusable + 焦点框 + 聚焦缩放
connect(card, &Box::clicked, this, [card] { /* ... */ });
connect(card, &Box::focusIn, this, [card] { /* ... */ });
```

可聚焦的 Box 同时还能当容器：子节点照样能被聚焦，因为 `focus_only_self` 默认 false
（`true` = 复合控件语义，只把自己当焦点停靠点，列表/滑条那种内部自己导航的才需要）。

导航相关的开关（都在 `Widget` 上）：

| 属性 | 作用 |
|---|---|
| `focusable` | 是否进入焦点列表（`Page::Update` 每帧收集） |
| `focus_only_self` | true = 只把自己当停靠点；false = 自己和子节点都能被聚焦 |
| `focus_zone` | 焦点分区：跨分区只允许左右方向（左列 Tab / 右内容区就是两个分区） |
| `capture_horizontal` / `capture_vertical` | 自己消费这两个方向，全局导航让位（滑条/L 列表内部导航） |
| `focus_on_hover` | 桌面端：鼠标悬停即接管焦点（鼠标和手柄共用一个焦点） |
| `focus_frame` / `focus_scale` / `focus_translate` / `focus_frame_color` / `focus_animation_speed` | 焦点视觉（框 + 缩放 + 位移，都是指数平滑，吃 dt） |
| `hasFocus()` / `focusIn` / `focusOut` | 读状态 / 收信号 |

鼠标、键盘方向键、手柄方向键走的是同一条路：`Page::Update()` 里
`Global::CollectFocusables → Global::NavigateFocus(焦点列表)`，算法是**最近邻**
（主方向投影距离 + 2 倍垂直偏移），优先级：同分区且无祖先/后代关系 → 同分区 → 跨分区（仅左右）。
程序化切焦点用 `widget->RequestFocus()`（要求 focusable）或 `Global::SetFocus(widget)`；
初始焦点不设置的话会自动落在焦点列表的第一个。

### 坐标系（先记住这三条）

1. 设计空间固定 **1280x720**（后端按 `min(高/720, 宽/1280)` 缩放），所有尺寸都按 720p 写。
2. `position` 是**相对父节点内容区左上角**的偏移；页面根节点没有 padding，所以 `(0,0)` 就是屏幕左上角。
3. `size` 是**外框尺寸**（含 padding/border）；`size = 0` 表示该轴按内容自适应。

### 加组件的步骤（后面每加一个都这么走）

1. 在 `component_view/components/` 下新建 `Xxx.{h,cpp}`，继承 `cv::Widget`；
2. 需要自绘就重写 `OnDrawContent(ImDrawList*, const Rect&)`，需要测量尺寸就重写 `MeasureContent(avail)`；
3. 在 `demo.cpp` 的 `OnBuild()` 里 `Emplace<Xxx>(...)` 放出来；
4. `component_view/` 下的 `.cpp` 由 CMake GLOB 自动纳入，不用改构建脚本。

## ImGui 能力导览（gui_dev_imgui_tour）

`examples/imgui_tour/` 是一个**只用 ImGui 原生接口**写的导览：Tab 分页，页面本身不滚动，
每个 Tab 里的控件都能真的操作（鼠标 / 键盘 / 手柄都可以）。

```bash
./build/mac/gui_dev_imgui_tour
GUI_DEV_TOUR_TAB=5 ./build/mac/gui_dev_imgui_tour   # 抓帧/CI：直接选中第 5 个 Tab
```

| Tab | 覆盖的 ImGui 能力 |
|---|---|
| 总览 | Text / TextColored / TextDisabled / LabelText / BulletText / TextWrapped、Button / SmallButton / ArrowButton / ColorButton / InvisibleButton 自绘、Checkbox / RadioButton / Selectable、ProgressBar、SetTooltip / BeginTooltip、BeginDisabled、PushStyleVar+PushStyleColor、GetContentRegionAvail / CalcTextSize |
| 输入 | InputText(char[]) / InputText(std::string 走 imgui_stdlib) / InputTextWithHint / Password / Multiline、InputInt / InputFloat、DragInt / DragFloat / DragFloat3 / DragFloatRange2、SliderInt / SliderFloat / VSliderFloat / SliderAngle、BeginCombo、BeginListBox、ColorEdit4 / ColorPicker4 / ColorButton |
| 布局 | CollapsingHeader、TreeNodeEx（Leaf / Selected / DefaultOpen）、SameLine / Indent / Dummy / BeginGroup / AlignTextToFramePadding、嵌套 BeginTabBar、BeginChild（独立裁剪与滚动）、BeginTable（表头 / 斑马纹 / 可排序 TableGetSortSpecs / 可调列宽 / ScrollY）、分隔条拖拽 |
| 弹层 | OpenPopup / BeginPopup、BeginPopupContextItem（右键菜单）、BeginPopupModal（模态独占输入、多按钮）、BeginMenuBar + BeginMenu + MenuItem（快捷键、选中、禁用、子菜单、内置换配色）、SetTooltip / 延迟提示 / 富内容 Tooltip |
| 高级 | BeginDragDropSource / SetDragDropPayload / BeginDragDropTarget / AcceptDragDropPayload（自定义载荷）、ImGuiListClipper（10 万条只提交可见行）、BeginMultiSelect + ImGuiSelectionBasicStorage（1.92 官方多选） |
| 绘图 | ImDrawList：AddLine / AddRect / AddRectFilled / AddRectFilledMultiColor（渐变）/ AddCircle(Filled) / AddTriangleFilled / AddConvexPolyFilled / AddPolyline / AddBezierCubic / Path API（PathLineTo+PathBezierCubicCurveTo+PathStroke）、PushClipRect、AddImage / AddImageRounded / AddImageQuad、AddText（指定字体字号 + cpu_fine_clip_rect）、AddCallback（渲染期回调）、PlotLines / PlotHistogram |
| 字体样式 | ImFontAtlas 字体列表、PushFont(font, size) 动态字号（1.92 按需光栅化）、style.FontSizeBase / FontScaleMain、运行时改 FrameRounding / FrameBorderSize / ItemSpacing、StyleColorsDark/Light/Classic、ShowStyleEditor、style.Colors 全表 |
| 系统工具 | ImGuiIO 全量读数（Framerate / DisplaySize / FramebufferScale / MousePos / Capture / Backend 名 / ConfigFlags / BackendFlags）、ImDrawData 统计、IsKeyDown（含 Gamepad 键）、NavActive/NavVisible/NavId、io.Config* 开关、ImGuiStorage、剪贴板、SaveIniSettingsToMemory / LoadIniSettingsFromMemory、ImGuiTextBuffer + ImGuiTextFilter 日志、ShowDemoWindow / ShowMetricsWindow / ShowIDStackToolWindow / ShowDebugLogWindow / ShowAboutWindow |

### 这个 demo 里踩到 / 用到的东西

- **内置手柄导航**：demo 把 `PadState` 翻译成 `ImGuiKey_Gamepad*` 喂给 `io.AddKeyEvent`，
  再打开 `NavEnableKeyboard | NavEnableGamepad`（手柄 A=Activate、B=Cancel、X=Menu/Layer、
  Y=ContextMenu、按住 X + L1/R1 = ImGui 的窗口切换，即 Ctrl+Tab）。Tab 翻页由我们自己的输入层
  处理（手柄 ZL/ZR、键盘 Q/E），用 `ImGuiTabItemFlags_SetSelected` 实现。
- **字号基准**：后端每帧会把 `style.FontScaleMain` 重置为 1.0（它用来算字体光栅密度），
  所以导览用 `style.FontSizeBase = 17px` 定 720p 手持基准，控件高度跟着字号走。
- **渲染统计**：`ImDrawData` 只在 `ImGui::Render()` 之后有效，`UiContext` 现在会在
  `EndFrame()` 里顺手采集并暴露 `LastDrawCalls() / LastVertices() / LastIndices()`。
- **多选 API 的顺序**：`BeginChild` 要包住 `BeginMultiSelect/EndMultiSelect`（反过来会踩
  `EndTable` 的状态断言），`SelectionBasicStorage::ApplyRequests` 在 Begin/End 两次都要调用。

界面快照：`docs/imgui-tour-overview.png`、`-inputs.png`、`-drawing.png`、`-system.png`。

## 自定义控件 101

教学 demo：`gui_dev_widget_demo`（源码 `examples/widget_lessons/WidgetDemo.cpp`，
8 个循序渐进的最小例子，每段注释写了「用了哪些 API / 为什么 / 坑在哪」）。

### 心智模型

ImGui 是**立即模式 + 布局游标**。它不认识你的控件，所以自定义控件固定两步：

1. **占位** —— 让 ImGui 管布局、命中测试、ID、裁剪、遮挡；
2. **自绘** —— 在占位矩形里用 `GetWindowDrawList()` 画任何东西。

```cpp
bool MyWidget(const char* id, const char* label) {
    // 1) 量尺寸 + 取原点（必须在占位之前取）
    const ImVec2 text = ImGui::CalcTextSize(label);
    const ImVec2 size(text.x + 28.0f, text.y + 14.0f);
    const ImVec2 mn = ImGui::GetCursorScreenPos();

    // 2) 占位：InvisibleButton 会推进布局游标，并登记一个可交互 item
    const bool clicked = ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 mx(mn.x + size.x, mn.y + size.y);

    // 3) 自绘：绘制顺序 = 代码顺序
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(mn, mx, hovered ? IM_COL32(0x2E, 0x6F, 0xB8, 0xFF)
                                      : IM_COL32(0x44, 0x4A, 0x54, 0xFF), 5.0f);
    dl->AddRect(mn, mx, IM_COL32_WHITE, 5.0f, 0, 1.5f);
    dl->AddText(ImVec2(mn.x + 14.0f, mn.y + 7.0f), IM_COL32_WHITE, label);
    return clicked;
}
```

### API 速查

| 用途 | API |
|---|---|
| 占位（可交互） | `InvisibleButton(id, size, flags)` |
| 占位（不可交互） | `Dummy(size)` / `ItemSize` + `ItemAdd`（internal） |
| 命中与状态 | `IsItemHovered` / `IsItemActive`（按住期恒真）/ `IsItemClicked` / `IsItemFocused` / `IsItemDeactivatedAfterEdit` |
| 几何 | `GetItemRectMin/Max/Size`（已提交 item）、`GetCursorScreenPos`、`CalcTextSize`、`GetTextLineHeight` |
| 时间 | `GetIO().DeltaTime`（控件内动画用它，不必外部传 dt） |
| 绘制 | `GetWindowDrawList()` → `AddRectFilled/AddRect/AddLine/AddCircle(Filled)/AddTriangleFilled/AddConvexPolyFilled/AddText/AddImage(Quad)`、`PathArcTo`+`PathStroke`、`PushClipRect` |
| ID | `PushID/PopID`（循环里必须）、`GetID`、`GetStateStorage()` 存控件本地状态 |
| 特殊交互 | `IsMouseDragging`、`GetMouseDragDelta`、`IsKeyPressed`、`BeginDragDropSource/Target`、`BeginPopupContextItem` |
| 完整按钮语义 | `imgui_internal.h` 的 `ButtonBehavior(bb, id, &hovered, &held)`（键盘激活/重复/拖出取消/Tooltip 都由它管） |

### 三条铁律

1. **动画必须吃 dt**：`v += (target - v) * (1 - exp(-speed * dt))`，不要 `v += 0.1f`（帧率一变速度就变）。
2. **状态存在帧外**：控件函数体内的 `static`、结构体成员，或 `ImGui::GetStateStorage()`（按 ID）。
   本项目要求同时可被多处实例化，所以正式控件都用成员/结构体，见 `GameMenuButton`。
3. **ID 必须唯一**：ImGui 靠 ID 认控件（状态/动画/焦点都挂 ID）。循环里用 `PushID(i)`，
   否则会出现「改一个动全部」或直接断言。

### 本项目里踩过的坑（都是真事）

| 坑 | 现象 | 正解 |
|---|---|---|
| `SetCursorPos` 后没有 item | 1.92 断言 `ErrorCheckUsingSetCursorPosToExtendParentBoundaries` | 复位光标后补 `Dummy(0,0)`，或干脆别动光标 |
| 忘记推进布局 | 后面的控件叠在同一位置 | 必须 `InvisibleButton`/`Dummy`，不要只 `SetCursorScreenPos` 画完就走 |
| 每帧堆分配 | 卡顿、内存抖动 | 文字用 `snprintf` 写固定缓冲，别每帧构造 `std::string` |
| 动画状态残留 | 再次打开菜单时动画从上次的值接着跑 | 每个元素提供 `Reset()`，在 `OnEnter` 里统一调用 |
| 交互状态和视觉不同步 | 悬停变了但按钮没动 | 把 hover/active 存进自己的动画状态，`Update(dt)` 里推进 |

### 三个真实例子（可直接对照读）

| 控件 | 文件 | 看点 |
|---|---|---|
| 可聚焦 Box（流光边框） | `framework/ui/Components.cpp` `FocusableBox` | 等弧长重采样 + 贴图沿边框滚动 + 羽化抗锯齿 |
| 斜切菜单按钮 | `framework/gamemenu/GameMenuElement.cpp` `GameMenuButton` | 焦点弹性位移、扫描高光、随机错位色块、按压反馈 |
| 存档槽卡片 | 同上 `GameMenuSaveSlot` | 多实例独立动画、缩略图占位、每实例随机状态 |

## 暂停菜单（gamemenu）

`gui_dev_pause_demo`：假游戏画面 + 游戏运行时暂停菜单，**全部用 DrawList 自绘**，
一个 ImGui 控件（Button/Selectable/TabItem）都不用，因此没有 ImGui 默认视觉与 Nav 焦点环。

### 分层

```text
GameMenuHost        外壳：ZL+ZR 开关、入场/退场状态机、视图栈、输入分发
└── GameMenuView    一层页面：自己有哪些项、焦点在哪、怎么画
    ├── MainMenuView    一级菜单（高频 / 配置 / 危险 三组）
    ├── StateSlotView   存档 / 读档（同一个 UI，确认行为不同）
    ├── SettingsView    独立设置 / 全局设置（左侧分类 + 右侧选项）
    └── DialogView      确认对话框（重置 / 退出 / 未保存提示，覆盖层）
        └── Element     单项：动画状态 + 绘制（GameMenuButton / Panel / Tab /
                        Selector / MenuOptionRow / SaveSlot / FocusFrame）
```

动作出口是接口而非直接调用：`MainMenuDelegate` / `StateSlotDelegate` /
`SettingsDelegate` / `DialogDelegate`。demo 里全部是 Mock，
所以 UI 与渲染后端、模拟核心都是解耦的。

### 动画系统（`MenuAnimation.h`）

- `SmoothTo(cur, target, speed, dt) = cur + (target-cur) * (1 - exp(-speed*dt))`：跟随类；
- `MoveTowards(cur, target, duration, dt)`：**必须有明确时长的进度**（焦点 160ms、
  按压 100ms、扫描 220ms、入场 280ms、退场 160ms），也天然与帧率无关；
- `EaseOutCubic / EaseOutQuad / EaseInOutCubic / EaseOutBack`（后者做焦点获得的轻微弹性）；
- `MenuAnimationState{focus, press, enter, exit, sweep, flash}` 挂在每个元素上，只在
  `Update(dt)` 里推进、只在 `Draw` 里读；`OnEnter` 里统一 `Reset()`，避免再次打开时残留；
- 逐项出现用 `StaggerProgress(open_progress, index, cfg)`，25ms 步进。

所有几何与时长都在 `GameMenuTheme`，绘制代码里没有魔法数字。
**全链路只用 `ctx.time` / `dt`，没有 `ImGui::GetTime()`、没有 sleep、没有固定帧计数。**

### 焦点动画（最重要的部分）

未聚焦：黑色斜切按钮 + 白字 + 白色细边框。
获得焦点后同时发生：右移 16px、加宽 34px、微增高、**红色错位色块（随机形状）**、左侧红色竖条、
白色边框 1.5 → 2.5px、文字右移、左侧红色箭头滑入并 `sin(t*8)*3` 轻微摆动、
`EaseOutBack` 回弹、以及**一次** 220ms 的扫描高光（只触发一次，不循环）。
焦点从旧项移到新项时，两个元素的 `focus` 各自 0→1 / 1→0 同时进行，
焦点框（`GameMenuFocusFrame`）再以指数平滑追过去。

### 红色错位色块（每次聚焦随机换形状）

`AccentPlate`：压在按钮下面的红色块。**每次获得焦点都重新掷一次形状**，再用
`SmoothTo` 过渡过去，所以同一条目再次聚焦时看到的位置/宽度也不一样
（Persona 式「不规则边缘 + 轻微位移」）。

- 随机量：左右错位、向下露出高度、宽度伸缩、斜切量浮动，范围全在 `GameMenuTheme`；
- 随机源：xorshift32，按进程启动时间播种 → 每次运行不同、单次运行内可复现；
- 无堆分配、无平台依赖；按键/摇杆/鼠标触发的聚焦走同一条路径。

> 有个必须注意的约束：色块压在按钮下面，**只有露出来的边可见**。
> 所以 `row_gap` 必须留得下 `plate_offset_y` 的最大值（当前 12 vs 18，
> 露出的高度在 5~12px 之间变化），否则变化会被下一个按钮盖住，看着像"没变"。

一级菜单/设置项/存档槽三种元素共用这个组件。

### 开关与布局

- ZL + ZR（键盘 `Z`+`C`）：同时按住为一次开关，边沿触发；关闭时整个面板向右滑出；
- `+`（Tab）：无论在第几层都直接回游戏；`B`（Esc）才逐层返回；
- 菜单**贴左侧**占约 41%，右侧保留游戏画面；遮罩 `dim_alpha = 0.46`（不全黑、不用模糊）；
- 面板入场用 `EaseOutBack` 从屏幕**左侧**滑入，标题比内容稍晚到位，菜单项 25ms 依次出现。

### 操作（demo）

| 按键 | 手柄 | 作用 |
|---|---|---|
| `Z` + `C` | ZL + ZR | 打开 / 关闭菜单 |
| `↑ ↓ ← →` | 十字键 | 移动焦点 / 修改选项 |
| `Enter` | A | 确认（带按压反馈） |
| `Esc` | B | 返回上一层 |
| `Q` / `E` | L / R | 翻页 / 切换分类 |
| `Tab` | + | 直接返回游戏 |

### 性能约束

每帧只改位置/尺寸/颜色/透明度与少量顶点，不建纹理、不建字体、不做离屏与模糊。
已核对 `framework/gamemenu` 无 `std::string` / 无每帧 `std::vector` 增长 / 无 `new`；
文本格式化全部写进固定成员缓冲。

### Mock 与真实实现的边界

demo 里的存档读写、重置、退出、设置项变更**全部是 Mock**（只更新画面与提示），
不接任何模拟核心。接真实实现时只需替换四个 Delegate：
`SaveState/LoadState/FillSlot`、`OnResume/OnRequestReset/OnRequestExit`、
`OnSettingChanged/OnSettingCommand`、`OnDialogResult`。

## 尺寸基准：1280x720 设计空间

**所有 UI 尺寸都按 720p 写**（几何、字号、行高），运行时整体等比放大到实际分辨率。

```cpp
// scale 取更受限的一边：保证逻辑画布恒为「>=1280x720」
scale = clamp(min(h / 720, w / 1280), 0.4, 4.0);

SDL_RenderSetScale(renderer_, scale, scale);        // 几何缩放交给 SDL
io.DisplaySize             = drawable / scale;      // 逻辑坐标 = 设计空间
io.DisplayFramebufferScale = (scale, scale);        // 只作为字体光栅化密度
style.FontScaleMain        = 1.0f;                  // 字号已在设计空间，不能再乘
```

| 画布 | scale | 逻辑画布 | 布局表现 |
|---|---|---|---|
| 1280x720（手持 / 16:9） | 1.00 | 1280x720 | 设计基准，原样 |
| 1920x1080（底座） | 1.50 | 1280x720 | 纯等比放大 |
| 960x720（4:3 窗口） | 0.75 | 1280x**960** | 画布变高：面板封顶并垂直居中 |
| 1600x720（宽屏） | 1.00 | **1600**x720 | 画布变宽：面板加宽到上限，槽位改 3 列 |

**两个坑（都踩过）**：

1. `imgui_impl_sdlrenderer2` **不会**用 `FramebufferScale` 缩放顶点——它只把该值用在
   裁剪矩形上，顶点原样交给 `SDL_RenderGeometryRaw`。所以几何缩放必须显式
   `SDL_RenderSetScale`（后端注释也写明：用户设了它，就由 SDL 负责缩放）。
2. `FramebufferScale` 仍然要设成 `scale`：imgui 1.92 用它做**字体光栅化密度**
   （`imgui.cpp`: `g.FontRasterizerDensity = io.DisplayFramebufferScale.x`），
   否则字形按逻辑尺寸光栅化再被 SDL 放大 → 发虚。

> 更早的做法是 `DisplaySize = drawable` + 只给字体乘 `FontScaleMain`：
> 1080p 上字变大而面板/行高不变，文字会溢出面板。现在不会再出现。

### 布局自适应（`ResolveGameMenuLayout`）

缩放只解决「大小」，画布**比例**变化要靠布局层（`GameMenuTheme.h` 里的
`GameMenuLayout` / `ResolveGameMenuLayout`）：

- **面板位置**：贴**左侧**弹出（`panel_x = screen.min.x + menu_left_margin`），
  游戏画面留在右侧；滑入/滑出都从左边屏幕外进出；
- **面板宽度**：`screen_w * 0.41`，夹在 `[440, 640]`，且至少给右侧游戏画面留 52%；
- **面板高度**：可用高度封顶 `700`，多出来的空间**上下均分**（画布很高时面板垂直居中，
  不会被拉成一条）；
- **槽位栅格**：内容区宽于 `540` 时改 3 列 2 行，否则 2 列 3 行；上下键的步长跟着列数走；
- **一级菜单**：内容在内容区里垂直居中，超长画布下不会贴着顶部留一片空白；
- **设置页**：分类列宽随面板宽度在 `[108, 136]` 之间伸缩；
- **对话框**：宽度随画布缩放，夹在 `[420, 640]`。

验证方式（demo 支持覆盖窗口尺寸）：

```bash
GUI_DEV_WINDOW=960x720  ./build/mac/gui_dev_pause_demo   # 4:3：面板居中不裁剪
GUI_DEV_WINDOW=1600x720 ./build/mac/gui_dev_pause_demo   # 宽屏：面板 640 + 槽位 3 列
GUI_DEV_WINDOW=1920x1080 ./build/mac/gui_dev_pause_demo  # 底座：纯 1.5 倍等比
```

快照见 `docs/pause-adaptive-4x3.png`。

基准数值集中在 `GameMenuTheme`（菜单）与 `ui/Theme.h`（组件层），
绘制代码里不出现尺寸魔法数字。720p 下的关键值：正文字号 25、标题 36、
菜单行高 56、菜单宽 520（≈41% 屏宽）、面板内边距 28。

## 页面画布：直接画在屏幕上

`Components::BeginPanel` / `EndPanel` 是**铺满整屏的根画布**，不是浮动窗口：

- 每帧显式 `SetNextWindowPos(0,0)` + `SetNextWindowSize(DisplaySize)` —— 不指定尺寸
  时 ImGui 会按内容自动撑成一个浮窗。
- `NoDecoration | NoMove | NoBringToFrontOnFocus | NoNavFocus`，窗口圆角/边框压成 0。
- 页头（52px）+ 可滚动内容区 + 页脚（40px）都在画布内，页脚用绝对定位贴到
  `画布高度 - 40`，因此不会有元素溢出屏幕。
- **页脚只能在画布内绘制**：`EndPanel(ui, hints)` 内部完成。如果在 `ImGui::End()`
  之后再调绘制函数，会落到 ImGui 的隐藏 fallback 窗口上，屏幕角落就会多出一块浮动的方块。

核对布局（Switch 上拿不到截图时尤其有用）：

```bash
GUI_DEV_DEBUG_LAYOUT=1 ./build/mac/gui_dev_demo
# [gui_dev] root canvas pos=(0,0) size=(1280,720) display=(1280,720)
```

## 可聚焦 Box（流光聚焦）

> 这一节讲的是 **framework 层** 的 `Components::FocusableBox`（贴图版流光，给宿主内核/启动器用）。
> `component_view` 里的 `Box` / `Button` 不依赖贴图，流光框是 `Draw::FlowingRing` 直接按弧长采样算出来的
> （见上面「Button：7 种形态」）。两套并存，别混着改。

游戏机 UI 的焦点是**显式索引**（手柄方向键选择），不是 ImGui 的 nav 焦点，
所以 `focused` 由调用方传入，组件只负责画。

```cpp
Components::BoxStyle style;
style.flow_texture = flow_texture.ImGuiRef();   // assets/img/border_gradient.png

const Components::BoxResult r = Components::FocusableBox("card", focus == i, style,
    [&](const ImVec2& content_size) {          // 可选：框内的 ImGui 控件
        ImGui::TextUnformatted("游戏库");
    });
if (r.hovered) focus = i;                      // 鼠标悬停接管焦点
if (r.clicked) Launch(i);                      // 单击激活
```

| 参数 | 默认 | 说明 |
|---|---|---|
| `size.x <= 0` | 填满可用宽度 | 放进 `BeginTable` 单元格即自适应栅格 |
| `height` | 132 | 高度兜底 |
| `rounding` / `border_width` / `glow_width` | 12 / 3 / 8 | 圆角、边框粗细、外发光宽度 |
| `padding` | 16 | 内容内边距（回调拿到的可用尺寸已扣除） |
| `flow_speed` | 0.30 | 每秒沿边框转几圈 |
| `flow_cycles` | 1.0 | 贴图沿周长平铺遍数 |
| `flow_dim_alpha` / `flow_peak_alpha` | 0.10 / 1.0 | 暗部 / 光斑不透明度 |
| `flow_segment_length` | 2.0 | 路径等弧长采样步长；越小圆角越圆（见下） |
| `flow_texture` | 无 | 无效时退化为 `focus_fallback_color` 纯色边框 |

流光实现（`DrawFlowBorder`）：

1. 圆角矩形轮廓 → **等弧长重采样**（默认 2px/段，上限 2048 段）；
2. **顶点法线取相邻两段外法线的角平分线并做 miter 修正**，相邻四边形共用顶点 →
   等宽连续带子，拐角不会出现缝隙或 V 形缺口；
3. 每段用渐变贴图贴一条 band，UV 的 u 按累计弧长/周长映射 + `GetTime() * flow_speed` 相位；
   再把 UV 按整数边界**在软件层拆段**，保证 u 始终落在 [0,1]；
4. 同一相位叠 `cos³` 包络控制透明度 → 一道光斑绕框跑；
5. 每段 6 条带：宽而淡的外发光 + 本体 + 两侧各两级 alpha 羽化肩（等价于 imgui
   给 `AddRect` 做抗锯齿的思路：边缘多画一圈低 alpha 顶点）。

### 两个已修的渲染坑

- **8px 采样的圆角是折线**：12px 半径的圆角在 8px 步长下只有 2 段，肉眼就是切角。
  改 2px 后约 9 段，配合共用顶点法线即平滑。
- **UV 超出 [0,1] 不可靠**：SDL2 没有纹理 wrap 模式设置（默认 clamp），
  `u > 1` 会被夹住，导致渐变在周长后半段断裂成纯色。现在软件层按整数边界拆段，
  u 恒在 [0,1] 内，不依赖采样器行为。

贴图要求：横向**周期性无缝**渐变（`border_gradient.png` 是 512×4、周期 256px、全不透明），
换图只要保持这个性质就无需改代码。

## 图片资源

```cpp
TextureRef tex(ui.GetBackend(), "img/border_gradient.png");   // 相对 assets/
if (tex.Valid()) ImGui::Image(tex.ImGuiRef(), tex.Size());
```

- 路径解析：`platform/AssetPaths.h`。桌面查 `assets/`、`../assets/`、`../../assets/`
  与源码目录；Switch 查 `sdmc:/switch/GUI_DEV/assets/` 与 `romfs:/`。
- 解码：**libpng**（mac homebrew / Switch portlibs 都自带）。没用 SDL_image
  （mac 上没装），也没 vendored stb_image。
- Switch romfs 只打包 `img/` 与 `font/MaterialIcons-Regular.ttf`：
  `switch_font.ttf`(10.9MB) 与 `switch_icons.ttf` 用不到（走 pl 共享字体），不进 romfs，
  否则 NRO 会从 7.6MB 涨到 18MB+。要换图可以直接改 `sdmc:/switch/GUI_DEV/assets/` 覆盖 romfs。

## 字体栈

三类字形，来源按平台不同，但 `framework/ui` 只认 `FontSource`：

| 内容 | mac（`assets/font/`） | Switch |
|---|---|---|
| 主文本字体 | `switch_font.ttf`（HOS 转出，与实机排版一致） | pl `PlSharedFontType_Standard` |
| 中文补充 | ——（主字体已含 CJK） | pl `PlSharedFontType_ChineseSimplified` |
| 按键图标 | `switch_icons.ttf`（NintendoExt 转出） | pl `PlSharedFontType_NintendoExt` |
| Material 图标 | `MaterialIcons-Regular.ttf` | 同左，**打包进 NRO 的 romfs** |

全部通过 `assets/` 或 pl 加载，没有字体文件时最后兜底到系统 CJK 字体、再兜底到 imgui 内置字体。

### 合并字体必须声明 GlyphExcludeRanges

`FontSource::content` 声明这个源负责哪类字形（`Text` / `ButtonIcons` / `MaterialIcons`），
`UiContext` 据此给每个源算出 `ImFontConfig::GlyphExcludeRanges`。

不声明会踩一个很隐蔽的坑：**合并字体时同一码位由「第一个能提供它的源」胜出**，而
NintendoExt / `switch_icons.ttf` 覆盖了整整 **1022 个私用区码位**（大量空白占位字形），
其中 **553 个与 MaterialIcons 重叠**。结果是 `save`(U+E161)、`play_arrow`(U+E037)、
`storage`(U+E1DB)、`archive`(U+E149)、`select_all`(U+E162)、`delete_sweep`(U+E16C)
等一大批 Material 图标会被渲染成 NintendoExt 的空方块。

规则：每个源排除「其它源拥有的、且不属于自己」的码位（见 `UiContext::RebuildFonts`）。
`exclusion_` 成员必须活到字体销毁——imgui 只存 `GlyphExcludeRanges` 指针。

## 按键图标（任天堂私用区）

图标字形按平台取，**码位一致**，所以 UI 代码不需要分支：

| 平台 | 字体来源 | 实现 |
|---|---|---|
| Switch | HOS 共享字体 `PlSharedFontType_NintendoExt`（`pl:u`） | `backends/sdl2/SwitchFonts.cpp` |
| macOS | `assets/font/switch_icons.ttf` | `backends/sdl2/DesktopFonts.cpp` |

```cpp
#include "ui/Icons.h"
ImGui::TextUnformatted(Icons::Glyph(Icons::Button::A));   // 字形
// 页脚提示在 EndPanel 里提交（页脚要画在画布内）
Components::EndPanel(ui, {{Icons::Glyph(Icons::Button::B), "返回"}});
```

| 按键 | 码位 | 按键 | 码位 |
|---|---|---|---|
| A | U+E0E0 | ↑ | U+E0EB |
| B | U+E0E1 | ↓ | U+E0EC |
| X | U+E0E2 | ← | U+E0ED |
| Y | U+E0E3 | → | U+E0EE |
| L | U+E0E4 | + (START) | U+E0EF |
| R | U+E0E5 | − (BACK) | U+E0F0 |
| ZL | U+E0E6 | L3 | U+E104 |
| ZR | U+E0E7 | R3 | U+E105 |

- 私用区码位属于字体协议，**只能通过 `Icons::Glyph()` 使用**，不要手写 UTF-8 字节。
- `Icons.cpp` 有 `static_assert` 校验执行字符集是 UTF-8（`\uE0E0` → `EE 83 A0`）。
- 字体重建后 `UiContext` 会做覆盖率自检，缺字形直接打到 stderr：

```text
[gui_dev] 字形自检：按键图标 全部就绪（共 16），Material 图标 全部就绪（共 35）
```

- 图标字形走 imgui 1.92 的动态光栅化，**不要**传 glyph ranges，也不要手工 `Build()` 图集。

## Material 图标

```cpp
#include "ui/Icons.h"
ImGui::TextUnformatted(Icons::Glyph(Icons::Material::Save));   // 软盘
ImGui::TextUnformatted(Icons::Glyph(Icons::Material::Settings));
```

- 35 个码位与 `GBAStation/src/ui/utils/MaterialIcons.hpp` 一致，并已逐个核对存在；
  全部通过渲染目录逐个确认字形正确（不要凭名字猜码位，本字体有 PUA 段与 NintendoExt 重叠）。
- `Glyph(Material)` 由码位在运行时编码成 UTF-8（BMP 固定 3 字节），返回内部轮转缓冲，
  仅用于当帧绘制；跨帧保存请自行 `std::string`。
- `MaterialIcons-Regular.ttf` **必须随应用发布**：Switch 上打进 NRO 的 romfs（见下），
  mac 上从 `assets/font/` 读。

## 已接入的上游约束

| 事项 | 说明 |
|---|---|
| imgui 1.92 字体 | 按需光栅化（`RendererHasTextures`），不要手工 `Build()` 图集，`io.FontGlobalScale` 已移除，改用 `style.FontScaleMain` |
| imgui 1.92 光标 | `SetCursorPos/SetCursorScreenPos` 之后必须紧跟一个 item（如 `Dummy(0,0)`），否则触发 `ErrorCheckUsingSetCursorPosToExtendParentBoundaries` 断言 |
| imgui 公开头文件 | `IM_PI` / `ImCos` / `ImSqrt` 都在 `imgui_internal.h`，组件层只用公开头，所以自带常量与 `<cmath>` |
| Switch 符号 | 必须定义 `IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS`（imgui 默认 shell 用 `fork/execvp/waitpid`，libnx 没有） |
| Switch 归档 | 工具链强制 `CMAKE_AR` 为 devkitPro 的 `aarch64-none-elf-ar`；macOS 宿主的 `llvm-ar` 会让 GNU ld 解析不到归档成员符号 |
| NRO 打包 | 用 devkitPro 的 `nx_create_nro()`；`assets/icon.png` 存在时自动作为图标 |
| 共享字体内存 | `plInitialize` 之后取到的字体在共享内存里，必须 `cfg.FontDataOwnedByAtlas = false`，否则 imgui 会去 free 系统内存 |

## 开发命令

```bash
git submodule update --init --recursive   # 拉取 imgui
git -C third_party/imgui fetch --tags     # 升级 imgui 用
```

## 状态

- 已确认：mac（Debug/Release）与 Switch（NRO）均可编译通过；mac 端**正常退出**（`GUI_DEV_EXIT_AFTER`
  走 RequestQuit 与合成 SDL_QUIT 两条路径）退出码 0、无泄漏告警；抓帧核对过：画布铺满
  1280×720、8 个 Box 栅格排布、焦点 Box 的流光边框完整闭合、注入一次方向键右后焦点
  正确右移、主字体为 HOS `switch_font.ttf`、16 个按键图标与 35 个 Material 图标逐个
  渲染正确；Switch NRO 内已确认含 `font/MaterialIcons-Regular.ttf` 与
  `img/border_gradient.png` 的 romfs。界面快照见 `docs/ui-preview.png`。
- 已确认（尺寸基准 + 自适应）：按 720p 设计 + `SDL_RenderSetScale` 等比缩放；
  720p 下正文 25px / 标题 36px / 行高 56px；在 1280x720 / 960x720(4:3) /
  1600x720(宽屏) 三种画布下抓帧核对：逻辑画布分别为 1280x720 / 1280x960 / 1600x720，
  面板分别贴左 525 / 525(垂直居中) / 640，槽位分别 2 列 / 2 列 / 3 列，均无裁剪与溢出；
  面板从屏幕左侧滑入滑出，游戏画面留在右侧；红色错位色块每次聚焦重新掷形状
  （实测同一条目两次聚焦：色块左边缘移动 16 逻辑像素、形状不同）。
- 已确认（暂停菜单）：mac 端脚本化注入按键走通
  `ZL+ZR 打开 → 焦点移动 → A 进存档页 → B 返回 → A 进设置页 → 改选项 → 重置确认框`，
  抓帧逐个核对（见 `docs/pause-*.png`）；两个 demo 正常退出码 0；
  mac 与 Switch 均编译通过（Switch 产出两个 NRO）。
- 已确认（component_view 清空重建）：组件与展示页全部删除，只留地基（Object/Types/Theme/Draw/
  Widget/Global）+ 一个最小 `Box` + `Page` 宿主；`demo.cpp` 现在只在页面左上角 `(0,0)` 放一个
  128x128 的 Box（`docs/` 里旧的 showcase-*.png 已一并删除）。清空前那套 16 个控件 / 16 个 Tab
  的实现都在 git 历史里（`35ec70d` 及之前），需要哪一块可以直接 `git show` 取回来。
- 已确认（Button 7 种形态 + 约定样式）：`Global::component_style` 统一 1px 灰白边框 / 5px 圆角 /
  右下软阴影 / 与控件留 2px 的完整闭合流光框，单实例都能链式覆盖；7 种形态（纯文字、图标+文字、
  纯图标、开关、自定义右侧文字、LR 选项、LR 数值）逐个抓帧核对，脚本化验收（`GUI_DEV_TRACE_SIGNAL=1`
  打信号）：A 键开关 `开→关`、Q/E（手柄 L/R）选项 `2→0`（wrap）、数值 `70→75`（步长 5）、
  `+`(Tab) 一键开关说明行且文字块仍垂直居中；抓帧见 `docs/buttons-demo.png`（说明行开）与
  `docs/buttons-demo-compact.png`（说明行关）。
- 已确认（Button 第二轮调整）：① 左侧图标改成"固定正方形格 + 格内水平/垂直居中"，纯图标按钮的说明行
  落到图标下方居中；② `TextButton` 不带说明行（弹窗提示文字用途）；③ LR 选择器中间改成**固定间隔**
  （默认 120px），内容居中、超长在间隔里跑马灯滚动且按间隔裁剪；④ `IconButton` 只保留圆角正方形 / 圆形
  两种形态（`setShape` + `setSide`）；⑤ `ValueButton` 长按加速（0.35s 后开始重复、间隔 120→30ms、
  步长倍率 1→8 取整封顶），`valueChanged` 只在松开时发一次。
  脚本化验收：短按 R 三次 → `70/75/80` 三次信号（每次松开一次）；长按 90 帧 → `90` **一次**信号；
  长按 L 90 帧 → `40`；Q 切到超长选项后抓两帧对比，间隔内文字整体左移（3428/14520 像素变化），
  间隔外无污染（裁剪正确）；图标/说明行的墨迹中心 477.2 vs 按钮中心 478（居中）。
- 已确认（Button 第三轮调整）：① 图标改成按「墨迹」居中（行盒下方有 descender 空白，按行盒居中会偏上）——
  实测 `无线网络` 图标墨迹中心 169.5 vs 按钮中心 170；② `IconButton` 的说明行改到按钮外面：默认下方
  （间距 4px），下方不够就等距放上方（把按钮放到 y=640 时实测说明行在 625.5~636），上下都不够就不显示
  （`caption_gap=600` 时无任何说明文字）；③（当时做过、第四轮已按要求去掉）聚焦放大 1.1 倍。
  快照：`docs/buttons-demo{,-compact}.png`。
- 已确认（Button 第四轮调整）：① **聚焦不再缩放**（实测把 `65` 与图标的墨迹在聚焦/未聚焦两态逐项比对，
  完全一致：13.0×9.0 / 21.5×21.5）；② 流光框宽度 2px → **3px**（`Global::component_style.focus_width`，
  抓帧看到顶部彩带从 5 行（物理）变成 8 行，含 AA）；③ **修掉"关掉说明行后主文字不回到居中"的 bug**：
  原来文字块的位置按"图标格高度"算（`block_y = center - block_h/2`，图标格 = 内容区高度），
  关掉说明行后主文字就停在偏上 8px 的位置；现在按文字块自己的高度居中。
  实测（说明行关）：无线网络主文字墨迹中心 169.5 / 存储路径 231.5 vs 按钮中心 170 / 232；
  普通按钮（无说明行）主文字墨迹中心 y=45.5、x=217 vs 按钮中心 (46, 218)。
- 已确认（尺寸统一，参考 GBAStation SettingPage）：`component_view/Theme.h` 的字号/控件高改成
  参考 SettingPage 的一套值（标题 22 / 区块 20 / 正文 16 / 说明 14 / 极小 12、控件高 **56**、
  标签高 26、内容留白 12），组件与 demo 全部改引用常量：Button 默认字号 = kFontBody、
  subtitle = kFontSmall、最小高 = kControlHeight；Badge 字号 = kFontSmall、高 = kBadgeHeight；
  Toast 正文 = kFontBody、最小高 = kControlHeight、内边距 16/12、图标 22；
  demo 的左列按钮 / 图标按钮边长 / 控制列按钮全部 = kControlHeight。
  实测：左列与控制列每个按钮 56 高、间距 10；图标按钮 56×56；Toast 高 56；
  徽标统一 92×26、两列、行距 32、白字。mac Debug/Release + Switch 编译通过、ctest 全绿、6 个 demo 退出码 0。
- 已确认（第六轮反馈修复）：
  ① **缩放崩溃**：运行期点放大后 mac 必崩（Metal `AGX … Region width OOB`），逐项实验定位到
  「运行期改 SDL 渲染缩放」这一条路径（把 SDL 缩放固定住或跳过 ImGui 绘制都不崩；字体图集尺寸
  一直是 imgui 512×512 == sdl 512×512、无待更新块，不是我们的纹理管理），mac 的 SDL 是
  sdl2-compat（有已知的 SDL_RenderSetScale 兼容问题）。改成**启动时设定缩放**（demo 默认 1.25），
  去掉运行期缩放按钮；同时不再让缩放触发布局图集重建、字体密度只跟分辨率走。
  ② **Toast 连点只能触发一次**：去重窗口默认 0（关闭），实测连点 4 次 → 4 条（色条 y 22/76/130/184）。
  ③ Toast 右边距 20 → **5px**：实测右边缘 1019（画布 1024）。
  ④ Toast 左侧两角改直角、色条直角且左/上/下离边框 2px：实测左上角 3×3 全是边框/填充（无页面白）、
  色条从 (821,22) 起（= 框 (819,20) + 2px）、宽 4、无圆角。
  ⑤ 徽标墙改成**两列、统一尺寸、白字、不套容器 Box**：实测两列 8 行、每个徽标 84×24、行距 32、
  墙外页面纯白（255,255,255）、徽标内最亮像素 (255,255,255)（白字）、填充色 = 平台色 α220 叠白底。
  ⑥ **界面整体放大**：demo 默认缩放 1.25（720p 手持），可以用 kDefaultZoom / GUI_DEV_ZOOM 调。
- 已确认（Toast 通知系统）：新增 `component_view/Toast.{h,cpp}`（ToastManager + Toast + ToastStyle），
  Page 持有并在每帧 `Update` / `Draw`。**视觉完全复用 Button 的 Box**：把 `Widget::DrawBackground` 的
  画法抽成 `Draw::ComponentBox()`（Box / Button / Toast 共用），样式参数统一从 `Global::component_style`
  取（`Widget::ApplyComponentBoxStyle()` + `Global::ComponentBoxVisual()`），没有第二套样式；
  重构前后同一帧抓帧逐像素比对 918636/921600 相同（= 同二进制两次运行的噪声水平 918649/921600），
  按钮/Box 视觉零变化。
  逐帧验证（临时打点，已移除）：
  ① 单条：Entering 15 帧（slide 用 EaseOutCubic 0.19→1.00）→ Visible 3.0s → Exiting 0.25s → 删除；
  ② 3 条不同消息：y = 20 / 74 / 128，高 44、间距 10、宽 200（短消息取 min 宽）；
  ③ 顶部消失后两条上移：(target−current) 差 −20.9 → −10.6 → −5.2 → −2.5 → −1.3 → −0.6 → −0.1（平滑，不瞬移）；
  ④ 快速创建 4 条全部入队并排列；⑤ 同一帧里 A 在退出（slide 1.00→0.82→0.66→0.52）同时 B 在进入
  （slide 0.61→0.70→0.79→0.85），X/Y 两套动画互不影响；⑥ 长文本换行：宽撑到 320、高 44→50（2 行）；
  ⑦ 720p 抓帧：右边缘 1188（=1280−92，让开控制列）、上边缘 20；色条 (78,201,176)/(241,76,76)/(0,95,184)
  （蓝跟着主题走）、Toast 底色 (232,232,235) 与 Button 默认底色完全一致；Toast 显示期间焦点导航照常工作。
  另外新增 Material 图标 `check_circle` / `error_outline` / `info`（自检 16 + 42 全部就绪）。
- 已确认（机种徽标 Badge）：按现网 NanoVG 实现的几何做了 `component_view/components/Badge.{h,cpp}`，
  文字与颜色走 `PlatformBadgeInfoOf()` 查表（1..14 + 其它），四种变体（GridListDetail / IisuCover /
  GameDataView / GridItem）用 `setStyle()` 选。demo 里 3×5 摆出全部 14 个机种 + 「其它」，另加一行
  变体对照。
  抓帧核对（浅色主题、面板底色 232,232,235）：GBA 徽标 x 654.5~689（宽 36 = minWidth）、
  y 24.5~43（高 20），与规格的 `(654,24,36,20)` 一致；填充色 (125,98,197) 正好等于
  平台色 α220 叠在面板上（0.863×(108,77,191) + 0.137×面板）；文本墨迹中心 34.8 vs 徽标中心 34。
  变体：iisu 胶囊 46×17、GameDataView 宽 62 高 26 且底色 α205 固定蓝 (109,168,225)、
  GridItem「Arcade」宽 58（文本 >3 字符触发长文本 minWidth）；墙内同机种默认样式宽 42
  （= textW 26 + 2×padX 16 > minWidth 36，符合公式）。
- 已确认（右侧控制列扩到三个 + UI 缩放）：控制列现在是「浅色/深色主题 / 放大 / 缩小」（56px 一列、
  间距 12px，贴右边缘 20px，每帧按画布宽度重排）。缩放实现：`Backend::SetUiZoom()`（0.5~3.0）
  把用户倍率乘进自动缩放，逻辑画布 = drawable / (自动缩放 × 倍率)，倍率变化时递增 `DisplayGeneration`
  让字体按新密度重建；`UiContext::SetUiZoom/UiZoom` 转发，demo 里是 0.8~2.0 的台阶表。
  验证：以 `GUI_DEV_ZOOM=0.8 / 1.0 / 1.25` 启动抓帧，第二个按钮的物理高度 89 / 112 / 140 px，
  比值 1 : 1.258 : 1.573 与 scale 1.6 : 2.0 : 2.5 的比值 1 : 1.25 : 1.5625 一致；
  运行时点按钮的路径逐帧核对（缩小 ×2 → 倍率 0.9 / 0.8，ui_scale 2.0 → 1.8 → 1.6；
  放大 → 1.1 / ui_scale 2.2），大量连续缩放脚本跑 3 次退出码 0。
- 顺带修掉一个潜伏的框架 bug：`RefreshIfDisplayChanged()`（字体重建）原来在
  `backend_.NewImGuiFrame()` **之后**调用，等于在 `ImGui::NewFrame()` 之后 `ClearFonts()` 换掉图集，
  这不符合 imgui 1.92 的约束（分辨率变化/手持↔底座切换时会走到这条路）。现在它挪到
  `PollEvents()` 之后、`NewImGuiFrame()` 之前。
- 已确认（主题切换 + 右侧控制列）：调色板改成运行时可切的角色色，`Theme::SetMode(Light/Dark)` +
  `Theme::ApplyToImGui()` + `Page::RefreshTheme()`（递归 `Widget::OnThemeChanged()`）；组件默认色跟随主题，
  用 setter 显式设过的颜色固定。窗口最右侧加了从上往下排的控制列，第一个是「浅色 / 深色主题」图标按钮
  （太阳/月亮 + 按钮外说明行显示当前主题），默认浅色。
  抓帧核对：浅色 页面 (255,255,255) / 按钮面 (232,232,235) / 开关轨道 (190,190,196)；
  切深色后 页面 (30,30,30) / 按钮面 (45,45,48) / 开关轨道 (88,88,92)；两帧 57517/57600 采样像素不同；
  旋钮两套主题都是白色。顺带修掉两个过程中暴露的问题：
  ① 根节点是透明容器，切主题时 `Box::applyComponentStyle()` 会把边框/阴影重新打开 → 整页被自己的
  阴影压暗（实测深色页面 30 → 22），现在 `Page::RefreshTheme()` 在刷新后把根节点装饰复位；
  ② `GlyphExcludeRanges[]` 超过 imgui 的 64 项上限（加两个图标就崩），`CompressRanges()` 现在会合并
  「中间没有自己码位」的相邻区间。
- 已确认（Button 第五轮调整）：① LR 选择器的固定间隔 120px → **90px**（原来的 3/4）——抓帧核对
  `[L] 整数缩放 [R]`：L 字形 264 起、内容墨迹中心 333（间隔中心 336）、R 落在右端 390~407；
  ② `ToggleButton` 右侧的「开/关」文字换成**滑块开关**（轨道 + 旋钮，开=#007ACC、关=rgb(88,88,92)、
  旋钮白色带一点投影），切换时旋钮位置和轨道色一起做指数平滑动画：逐帧打点看到
  `mix: 0 → 0.209 → 0.372 → 0.504 → 0.606 → 0.689 → … → 1`（约 0.45s 收敛），
  抓帧核对关态旋钮在 x368~383、开态在 x388~398，轨道色 (88,88,92) → (3,121,201)；
  接口：`setSwitchSize(w,h)` / `setSwitchColors(on,off,knob)` / `knob_speed` / `knobMix()`。
- 顺带修掉一个框架输入 bug（长按功能的前提）：`PadState::held` 原来是"每帧清零"，SDL 只在按下那一刻
  发一次 KEYDOWN，所以按住不放时 `held` 只有第一帧为真、`Held()` 根本没法用（`Input.h` 注释里
  写的是电平语义）。现在后端把上一帧的按住状态继承下来再叠加本帧事件，`held` 变回真正的电平；
  `pressed` 也就真的等于"本帧刚按下"（原来系统按键重复会被当成连续按下，长按会疯狂触发）。
- 顺带修掉两个真 bug：
  1) **整页被自己的阴影压暗**：`Box` 构造里会 `applyComponentStyle()` 打开阴影，而软阴影的每一层
     都是**实心矩形**（靠多层低 alpha 叠出模糊），页面根 Box 是透明的 → 白底实测只有 `(186,186,186)`；
     现在 `Page::Root()` 显式关掉根节点的边框与阴影，白底恢复 `(255,255,255)`。
  2) **焦点控件吞掉页面级快捷键**：`Widget::UpdateInteraction` 以前对 `kDispatched` 里的按键
     **无条件** `MarkConsumed`，普通按钮会把 `Menu`(`+`/Tab) 吃掉，页面永远收不到；现在只有
     `OnPadAction()` 返回 true 才消费，与 B 键的处理方式一致。
  另外 `Draw::FlowingRing` 增加了 `radius` 参数（原来固定按 `thickness*2` 估），流光框现在用
  `按钮圆角 + 外扩量`，圆角与按钮轮廓平行。
- 已确认（720p 手持基准）：把字号/行高/间距/面板几何整体从「桌面比例」压到手持尺度
  （正文 22→17、标题 34→26、控件高 44→34、列表行 46→32、键盘键 42→30、HUD 46→36、
  左列 236→186）；
  mac Debug/Release 与 Switch 均编译通过，四个 demo `GUI_DEV_EXIT_AFTER` 退出码 0，
  960x720 / 1600x900 / 640x360 窗口均不崩。
- 已确认（Qt 风格信号槽）：`component_view/Object.h` 实现 `Signal<Args...>` + `connect/emit/disconnect`，
  接收者继承 `Object` 析构时自动断开；`tests/qt_signal_test.cpp`（10 项）覆盖成员函数槽、无参槽、
  context+lambda、手动断开、接收者先析构、发送者先析构、一信号多接收者，`ctest` 全绿；
  demo 内脚本化验证：BUTTON 页 `A:1 → clicked 1 次`、`X:2 + Y:1 → auxTriggered 3 次`。
- 顺带修掉一个真 bug：`InputAction::ActionX/ActionY/Minus` 之前在枚举里有、但没进 SDL 按键/手柄映射表
  （x / y / - / 手柄 X / Y / BACK 都不响应），现在映射补齐并逐个抓帧确认收到。
- 已确认（输入链路修复）：三个实测出来的输入 bug ——
  1) **Switch 触摸完全无效**：`SDL_FINGER*` 以前只记进 `InputFrame.touch`，从没喂给 ImGui，
     而所有命中测试读的都是 `io.MousePos`；现在在 `NewImGuiFrame()` 里（`ImGui::NewFrame()` 之前，
     否则会被 `imgui_impl_sdl2` 的 `UpdateMouseData` 覆盖）翻译成 `AddMousePosEvent/AddMouseButtonEvent`，
     坐标按归一化 × `io.DisplaySize` 换算（跨分辨率/Retina 都对），并关掉 SDL 的触摸合成鼠标 + 用
     `SDL_TOUCH_MOUSEID` 去重避免双触发。
  2) **导览页手柄进不去**：外壳窗口带了 `ImGuiWindowFlags_NoNavFocus`，`IsWindowNavFocusable()` 因此
     返回 false、`g.NavWindow` 恒为 NULL，内置导航永远不激活；去掉该标志并在首帧 `NavInitWindow`
     之后，手柄/键盘可以直接在页面里移动（实测 `NavActive=1 NavVisible=1 NavId≠0`）。
  3) **窗口非 720p 时鼠标整体偏移**：`imgui_impl_sdl2` 给的是窗口点数，而命中测试用 `io.DisplaySize`，
     窗口为 640x360 时坐标差 2 倍；现在在 `NewImGuiFrame()` 里按比例修正（640x360 窗口下点窗口坐标
     (281,95) 正确命中逻辑坐标 (562,190) 的按钮）。
  验证方式：脚本化注入 `SDL_FINGERDOWN/MOTION/UP` 与 `SDL_MOUSEMOTION/BUTTON`，抓帧核对
  「触摸取消勾选复选框」「触摸点击按钮计数 +1」「导航高亮 + NavId≠0」。
- 已知字体问题：`assets/font/MaterialIcons-Regular.ttf` 的码位表已按官方
  `MaterialIcons-Regular.codepoints` 全面核对并修好 6 个错项（`games` U+E021、
  `sports_esports` U+EA28、`videogame_asset` U+E338、`cloud_upload` U+E2C3、
  `cloud_download` U+E2C0、`install_app` 借用 `install_mobile` U+EB72）；其余码位正常。另外 `◀ ▶ ⌫`(U+25C0/U+25B6/U+232B) 这类符号在原字体里缺字形，
  已统一换成 ASCII 文案。
- 未验证：NRO 在实机/模拟器上的运行表现（含 HOS 共享字体与 NintendoExt 的实际字形、
  romfsInit 是否成功、Material 图标在实机上的渲染）；暂停菜单在实机上的手感与耗时
  （30/60/120FPS 的时间一致性由公式保证，但没有实机测帧）。
- 已知取舍：`assets/font/switch_font.ttf` 10.9MB 进了 git。仓库体积敏感的话建议转
  Git LFS 或按需本地放置（mac 端缺它会退回系统 CJK 字体，不影响 Switch）。
