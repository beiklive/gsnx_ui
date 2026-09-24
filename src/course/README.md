# 分课时课程 · 编译与运行

> 屏上版：`课时 0 · 编译与运行`（启动课程后第一课就是它）。

## 1. 依赖

| | 需要 |
|---|---|
| 共同 | git submodule（imgui）、CMake ≥ 3.20、Ninja |
| macOS | Xcode Command Line Tools（clang）、`brew install sdl2 libpng` |
| Switch | devkitPro（devkitA64 + libnx）、`switch-sdl2`、`switch-pkg-config`、`switch-libpng` |

```bash
git submodule update --init --recursive   # 不拉会直接报 third_party/imgui is empty
```

不用 SDL_image（mac 上没有）：PNG 解码走 **libpng**，两端都有。

## 2. 编译与运行

```bash
# macOS
cmake --preset mac
cmake --build --preset mac
./build/mac/gui_dev_course          # L/R 切课，Esc 退出

# 只编课程这一个目标（改课时时最快）
cmake --build --preset mac --target gui_dev_course

# Release（更接近实机表现）
cmake --preset mac-release && cmake --build --preset mac-release

# Switch：产物 build/switch/dist/*.nro
cmake --preset switch
cmake --build --preset switch
# 部署：gui_dev_course.nro -> sdmc:/switch/
#      build/switch/dist/assets/img -> sdmc:/switch/GUI_DEV/assets/img
```

## 3. 四个可执行目标

| target | 内容 |
|---|---|
| `gui_dev_demo` | 组件预览（可聚焦 Box / 流光边框 / 字体与图标） |
| `gui_dev_pause_demo` | 暂停菜单 Demo（Persona 风格动态 UI，动作全 Mock） |
| `gui_dev_widget_demo` | 自定义控件 8 个最小例子 |
| `gui_dev_course` | 本课程（14 课：0~13） |

库目标 `imgui` / `gui_dev_backend` / `gui_dev` 才是 UI 本体，demo 只是壳。

## 4. 加一课要改 4 处

```
1) 新建 src/course/LessonNN_Xxx.cpp    实现 Scene + 工厂函数
2) src/course/CourseLessons.h          声明工厂
3) src/course/CourseLessons.cpp        表里加一行 {标题, 工厂}
4) CMakeLists.txt                      gui_dev_course 源文件列表加一行
```

课时之间互不影响：改坏一课不影响其他课编译。

## 5. 编译/运行常见问题（都是本项目真实踩过的）

| 现象 | 原因 / 解决 |
|---|---|
| `third_party/imgui is empty` | submodule 没拉：`git submodule update --init --recursive` |
| `No package 'sdl2' found`（mac） | PATH 里 devkitPro 的 pkg-config 排在前面（它只认 Switch portlibs）；预设已显式指定 `/opt/homebrew/bin/pkg-config` |
| Switch：`undefined reference` 到自己库里的符号 | 宿主 `/usr/bin/ar` 是 llvm-ar，归档格式让 GNU ld 解析不到成员；工具链已把 `CMAKE_AR` 锁到 devkitPro 的 `aarch64-none-elf-ar` |
| Switch：`undefined reference to waitpid/execvp` | imgui 默认 shell 处理用了 fork/execvp，libnx 没有；Switch 构建已定义 `IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS` |
| 运行时字体/图标全是方块 | 资源没找到：mac 查 `assets/`，Switch 查 romfs 或 `sdmc:/switch/GUI_DEV/assets/`；启动 stderr 有字形自检日志 |
| 退出瞬间崩溃 | 对象比 `Backend` 活得久，析构时回调了已销毁的后端 —— 见课时 2 的析构顺序与 `BackendLiveness` |
| 帧率不是 60 | 主循环没有帧率上限，轻负载下实测 ~120 FPS（PRESENTVSYNC 未锁住）；见课时 12 |

## 6. 调试开关（各 demo 通用）

```bash
GUI_DEV_EXIT_AFTER=60   ./build/mac/gui_dev_course   # 跑满 60 帧后正常退出（验证退出路径）
GUI_DEV_WINDOW=960x720  ./build/mac/gui_dev_course   # 覆盖窗口尺寸（验证自适应）
GUI_DEV_NO_VSYNC=1      ./build/mac/gui_dev_course   # 关闭垂直同步（性能对照）
```
