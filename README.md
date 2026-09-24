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
│   │   ├── Scene.{h,cpp}       # Scene 基类 + SceneStack（菜单栈）
│   │   └── Components.{h,cpp}  # 页头/页脚/列表/开关/进度/弹窗等基础件
│   ├── platform/               # ★ 平台层：唯一的平台接缝
│   │   ├── Backend.h           # 后端接口 + PlatformKind
│   │   ├── Fonts.h             # 字体来源接口（文本 / 按键图标）
│   │   ├── Input.h             # 抽象输入动作（Up/Confirm/...）
│   │   └── backends/sdl2/      # SDL2 后端 + 后端工厂 + 平台字体实现
│   ├── demo/                   # 示例 App（启动器风格界面）
│   └── main.cpp
├── third_party/imgui/          # submodule
├── assets/                     # Switch 图标等资源（可选）
└── build/                      # 构建产物（已 gitignore）
```

## 构建

### macOS（主要开发环境）

```bash
cmake --preset mac
cmake --build --preset mac
./build/mac/gui_dev_demo
```

依赖：`brew install sdl2`（或 `sdl2-compat`）。预设里显式指定了
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

## 按键图标（任天堂私用区）

图标字形按平台取，**码位一致**，所以 UI 代码不需要分支：

| 平台 | 字体来源 | 实现 |
|---|---|---|
| Switch | HOS 共享字体 `PlSharedFontType_NintendoExt`（`pl:u`） | `backends/sdl2/SwitchFonts.cpp` |
| macOS | `assets/font/switch_icons.ttf` | `backends/sdl2/DesktopFonts.cpp` |

```cpp
#include "ui/Icons.h"
ImGui::TextUnformatted(Icons::Glyph(Icons::Button::A));   // 字形
Components::Footer(ui, {{Icons::Glyph(Icons::Button::B), "返回"}});
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

- 已确认：mac（Debug/Release）与 Switch（NRO）均可编译通过；mac 端可运行不崩溃；
  mac 端运行时自检 `按键图标 16/16 全部就绪`（16 个码位在 `switch_icons.ttf` 的 cmap 中
  也已逐个核对存在）。
- 未验证：NRO 在实机/模拟器上的运行表现（含 HOS NintendoExt 的实际字形是否与上表一致）；
  Switch 端主字体仍回退到 imgui 内置字体，中文/箭头等非私用区字形会缺；Switch 端尚未接入
  libnx 分辨率切换（手持↔底座）与 HOME 键退出，钩子已留在 `Backend::DisplayGeneration()` 与
  `Backend::ShouldQuit()`。
