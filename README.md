# GUI_DEV

GBAStation 模拟器家族的**统一前端组件库**。各模拟器核心共用同一套 UI 代码与视觉规范，
避免每个核心各写一份界面。

- UI 框架：Dear ImGui（git submodule，锁定 `v1.92.9b`，master 稳定线）
- 窗口/渲染：SDL2（mac 与 Switch 共用同一份后端实现）
- 工具链：mac 用系统 clang + homebrew `sdl2`；Switch 用 `/opt/devkitpro`（devkitA64 + libnx）

## 目录结构

```text
GUI_DEV/
├── CMakeLists.txt              # 三块目标：imgui / gui_dev_backend / gui_dev + demo
├── CMakePresets.json           # mac / mac-release / switch 预设
├── cmake/toolchains/
│   └── Switch.cmake            # devkitA64 工具链入口（含 ar 修正）
├── src/
│   ├── core/
│   │   ├── App.h               # App 基类 + AppRunner 主循环
│   │   └── App.cpp
│   ├── ui/                     # ★ 组件层：只依赖 imgui，不含任何平台头文件
│   │   ├── UiContext.{h,cpp}   # 帧生命周期、字体（含图标字体合并）、字形自检
│   │   ├── Theme.{h,cpp}       # 统一配色/间距规范
│   │   ├── Icons.{h,cpp}       # 任天堂按键图标（私用区 U+E0xx / U+E1xx）
│   │   ├── Texture.{h,cpp}     # 图片纹理 RAII 句柄（走 Backend 加载）
│   │   ├── Scene.{h,cpp}       # Scene 基类 + SceneStack（菜单栈）
│   │   └── Components.{h,cpp}  # 页骨架 / 可聚焦 Box / 列表 / 开关 / 进度 / 弹窗
│   ├── platform/               # ★ 平台层：唯一的平台接缝
│   │   ├── Backend.h           # 后端接口 + PlatformKind + Texture
│   │   ├── Fonts.h             # 字体来源接口（文本 / 按键图标）
│   │   ├── AssetPaths.h        # assets/ 相对路径解析
│   │   ├── Input.h             # 抽象输入动作（Up/Confirm/...）
│   │   └── backends/sdl2/      # SDL2 后端 + 工厂 + 字体/资产/PNG 解码
│   ├── demo/                   # 示例 App
│   └── main.cpp
├── third_party/imgui/          # submodule
├── assets/
│   ├── font/                   # 字体（switch_icons.ttf 已入库）
│   └── img/                    # UI 图片（border_gradient.png）
└── build/                      # 构建产物（已 gitignore）
```

## 构建

### macOS（主要开发环境）

```bash
cmake --preset mac
cmake --build --preset mac
./build/mac/gui_dev_demo
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
main/demo  ->  gui_dev  ->  gui_dev_backend  ->  imgui
```

- `src/ui/` 与 `src/core/` **禁止** include SDL/GLFW/libnx 等平台头文件，平台能力一律走
  `gui_dev::Backend`。
- `gui_dev_backend` **禁止**调用 `gui_dev` 里的符号（`Theme::Apply()` 因此放在
  `AppRunner` 而不是后端里）。两个静态库互相引用会形成链接环：GNU ld 单遍扫描，
  先出现的一方必然解析失败。

**新增平台**：在 `src/platform/backends/<name>/` 实现 `Backend` + `CreatePlatformBackend()`，
然后在 CMake 的 `GUI_DEV_BACKEND` 分支里加一个选项。`src/ui` 一行不用改。

**新增界面**：继承 `Scene`，重写 `OnRender`，用 `Components::*` 拼装；
页面跳转用 `SceneStack::Push/Pop`。

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
| `flow_speed` | 0.25 | 每秒沿边框转几圈 |
| `flow_cycles` | 1.0 | 贴图沿周长平铺遍数 |
| `flow_dim_alpha` / `flow_peak_alpha` | 0.10 / 1.0 | 暗部 / 光斑不透明度 |
| `flow_texture` | 无 | 无效时退化为 `focus_fallback_color` 纯色边框 |

流光实现（`DrawFlowBorder`）：

1. 圆角矩形轮廓 → **等弧长重采样**（约 8px 一段），保证长直边也能逐段插值；
2. 每段取外法线，把渐变贴图当 band 贴上去，UV 的 u 按**累计弧长 / 周长**映射，
   再叠加 `GetTime() * flow_speed` 的相位 → 颜色沿边框滚动；
3. 同一相位上叠一个 `cos³` 包络控制透明度 → 一道光斑绕框跑；
4. 两遍绘制：宽而淡的外发光 + 细而亮的本体。

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
- Switch 构建会把 `assets/img/` 拷到 `dist/assets/img/`，随 NRO 一起丢到
  `sdmc:/switch/GUI_DEV/assets/` 即可。字体目录不进 romfs（10MB+ 会撑爆 NRO）。

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
- 字体重建后 `UiContext` 会做覆盖率自检，缺字形直接打到 stderr：`按键图标 16/16 全部就绪`
  或 `按键图标缺字形：X, Y（共 16 个）`。
- 图标字形走 imgui 1.92 的动态光栅化，**不要**传 glyph ranges，也不要手工 `Build()` 图集。

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

- 已确认：mac（Debug/Release）与 Switch（NRO）均可编译通过；mac 端连续运行无崩溃、无
  imgui 断言；`border_gradient.png` 加载为 512×4 纹理；按键图标运行时自检
  `16/16 全部就绪`；抓帧核对过：画布铺满 1280×720、8 个 Box 栅格排布、焦点 Box 的
  流光边框完整闭合、注入一次方向键右后焦点正确移到下一项。
- 未验证：NRO 在实机/模拟器上的运行表现（含 HOS NintendoExt 字形与 pl 共享字体）；
  Switch 端主字体仍回退到 imgui 内置字体，非私用区的中文会缺字形；Switch 端尚未接入
  libnx 分辨率切换（手持↔底座）与 HOME 键退出，钩子已留在
  `Backend::DisplayGeneration()` 与 `Backend::ShouldQuit()`。
