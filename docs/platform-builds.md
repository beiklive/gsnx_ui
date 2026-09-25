# 多平台构建（macOS / Switch / Windows / Android / iOS）

同一份 `CMakeLists.txt` 出五个平台。差异集中在三处，业务与组件代码不用改：

| 关注点 | 处理方式 |
|---|---|
| 依赖从哪来 | `cmake/Dependencies.cmake`：`GUI_DEV_DEPS_MODE=package`（系统包 / vcpkg）或 `fetch`（源码编译） |
| 平台服务 | `platform/backends/sdl2/` 下按平台各一份：`SwitchPlatform.cpp`（pl 共享字体 + romfs）、`AndroidPlatform.cpp`（APK assets）、`DesktopPlatform.cpp`（assets/font/*.ttf） |
| 目标形态 | `gui_dev_add_demo()`：桌面 = 可执行文件，iOS = `.app` bundle，Android = `libmain.so`（gradle 打包） |

---

## 1. 快速对照

| 平台 | preset | 生成物 | 需要先装 |
|---|---|---|---|
| macOS | `mac` / `mac-release` | 可执行文件 | `brew install sdl2`（libpng 一般已随系统依赖进来） |
| Nintendo Switch | `switch` | `.nro` | devkitPro（devkitA64 + libnx，见 README） |
| Windows x64 | `windows` | `.exe` | Visual Studio 2022（依赖默认源码编译；装了 vcpkg 可切 package 模式） |
| Android arm64 | `android` + gradle | `.apk` | Android SDK + NDK（`ANDROID_NDK_HOME`）、JDK 17 |
| iOS 真机 / 模拟器 | `ios` / `ios-sim` | `.app` | 完整 Xcode（不是只有 Command Line Tools） |

---

## 2. 依赖策略：package 还是 fetch

```bash
# 默认：先 pkg-config（mac / Switch / Linux 天然就有），再 find_package(CONFIG)（vcpkg / 手工装的 SDL2）
cmake --preset mac

# 源码编译：拉 SDL2 release-2.32.10 + libpng v1.6.58 + zlib v1.3.1
cmake -S . -B build/xxx -G Ninja -DGUI_DEV_DEPS_MODE=fetch
```

- **package 模式**：macOS 用 homebrew 的 `.pc`；Windows 用 vcpkg（`-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake -DGUI_DEV_DEPS_MODE=package`，包：`sdl2 libpng`）；Linux 用发行版包。
- **fetch 模式**：不需要任何系统包，但首次配置要联网（`FetchContent` 走 git）。**Android 必须用 fetch**（要 SDL 的 `SDL_android_main.c` 源码）。
- 出口统一成 `gui_dev::SDL2` / `gui_dev::PNG`，下游只链这两个。

---

## 3. 各平台步骤

### 3.1 macOS（现状，已长期验证）

```bash
brew install sdl2
cmake --preset mac && cmake --build --preset mac
ctest --test-dir build/mac
./build/mac/gui_dev_demo
```

### 3.2 Nintendo Switch

```bash
export DEVKITPRO=/opt/devkitpro
cmake --preset switch && cmake --build --preset switch
# 产物：build/switch/dist/*.nro，拷到 sdmc:/switch/
```

### 3.3 Windows x64

```powershell
:: 方式 A：什么都不装（推荐先跑通）——SDL2/libpng/zlib 从源码编译
cmake --preset windows
cmake --build --preset windows

:: 方式 B：用 vcpkg（编得快、体积小）
vcpkg install sdl2:x64-windows libpng:x64-windows
cmake -S . -B build/windows-vcpkg -G "Visual Studio 17 2022" -A x64 `
  -DGUI_DEV_PLATFORM=windows -DGUI_DEV_BACKEND=sdl2 -DGUI_DEV_DEPS_MODE=package `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/windows-vcpkg --config Release
```

- 资源查找顺序：`assets/` → `../assets/` → `../../assets/` → 源码 `GUI_DEV_ASSET_DIR` → **exe 所在目录**（`SDL_GetBasePath()`）。
  发布时把 `assets/` 拷到 exe 旁边即可。
- 默认留控制台窗口（方便看日志）；要 GUI 子系统可以自己加 `WIN32_EXECUTABLE`。

### 3.4 Android

```bash
export ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk/27.0.12077973
export ANDROID_HOME=$HOME/Library/Android/sdk

# 1) 先验证 native 侧能编出 libmain.so（不需要 gradle）
cmake --preset android && cmake --build --preset android

# 2) 打包 APK
cd android
echo "sdl2SourceDir=$PWD/../build/android/_deps/sdl2-src" >> gradle.properties  # SDL 的 Java 代码来源
./gradlew assembleDebug        # 或用 Android Studio 打开 android/ 目录
# 产物：android/app/build/outputs/apk/debug/app-debug.apk
```

- 结构：`android/app/build.gradle` 复用仓库根的 `CMakeLists.txt`，只编 target `main`
  （= `demo.cpp` + SDL 的 `SDL_android_main.c`）；`MainActivity` 继承 SDL 的 `SDLActivity`。
- 想换演示入口：`-DGUI_DEV_ANDROID_APP_SOURCE=<别的 main.cpp>`。
- 资源：`assets.srcDirs` 直接指向仓库 `assets/`；APK 内是 `assets/font/…`，由 `AndroidPlatform.cpp`
  用 `SDL_RWFile` 读进内存给 ImGui 当字体（`FontSource.data`）。
- 需要 `sdl2SourceDir`（gradle.properties）：SDL 的 Java 层不在本仓库，指向 SDL 源码即可。

### 3.5 iOS

```bash
cmake --preset ios-sim        # 模拟器（不签名，最好上手）
cmake --build --preset ios-sim

cmake --preset ios            # 真机：需要在 preset 里补签名团队
cmake --build --preset ios
```

- `cmake/toolchains/iOS.cmake` 设 `CMAKE_SYSTEM_NAME=iOS` + `iphoneos/iphonesimulator` + 部署目标（默认 13.0）。
- 每个演示都会生成 `.app`，`assets/` 被拷进 `Contents/Resources/assets`；
  运行时靠 `SDL_GetBasePath()` 找到（见 `AssetPaths.cpp`）。
- 入口：`SDL_main`（demo 的 `main` 通过 `#include <SDL_main.h>` 重定向，iOS 侧由 `SDL2main` 提供 `UIApplicationMain`）。
- 真机安装需要签名：把 preset 里的 `CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED` 去掉并补
  `DEVELOPMENT_TEAM`。

---

## 4. 本仓库做了哪些改动（本轮新增）

| 文件 | 作用 |
|---|---|
| `cmake/Dependencies.cmake` | 依赖解析（package / fetch 两种模式）+ 暴露 `gui_dev::SDL2` / `gui_dev::PNG` |
| `cmake/toolchains/Android.cmake` | NDK 工具链包装（ABI / API / STL 默认值 + 缺 NDK 时的明确报错） |
| `cmake/toolchains/iOS.cmake` | iOS 真机 / 模拟器工具链（sysroot、arch、部署目标、默认不签名） |
| `cmake/Info.plist.in` | iOS bundle 的 Info.plist（横屏、全屏、SDL 入口） |
| `CMakePresets.json` | 新增 `windows` / `android` / `ios` / `ios-sim` 配置与构建 preset |
| `framework/platform/backends/sdl2/AndroidPlatform.cpp` | Android 字体从 APK assets 读进内存 |
| `framework/platform/backends/sdl2/AssetPaths.cpp` | 增加 `SDL_GetBasePath()` 查找（Windows exe 旁、iOS bundle 内）；Android 明确走 APK 路径 |
| `framework/platform/Backend.h` | `PlatformKind` 增加 Android / iOS；`kIsHandheld` 覆盖移动端 |
| `android/` | gradle 打包骨架（app 模块、Manifest、MainActivity、资源、`sdl2SourceDir` 说明） |
| 各 `main.cpp` | 移动端加 `#include <SDL_main.h>`（Android / iOS 需要 `SDL_main` 入口） |
| `CMakeLists.txt` | `gui_dev_add_demo()`（桌面 exe / iOS bundle / Android 跳过）、Android 的 `libmain.so` target、按平台选平台服务实现 |

---

## 5. 验证状态（重要）

| 平台 | 状态 |
|---|---|
| macOS | ✅ 全量构建 + `ctest` 通过（本轮改动后回归验证过） |
| Switch | ✅ 配置 + 构建通过（devkitA64） |
| 依赖 `fetch` 模式 | ✅ 本机实测：从 GitHub 拉 SDL2 2.32.10 + libpng 1.6.58（zlib 用系统自带）编出 `gui_dev_demo` 并运行正常 |
| Windows | ⚠️ **未在真机验证**（本机没有 MSVC）。构建文件按标准做法写好：preset / 依赖两种模式 / 资源查找 |
| Android | ⚠️ **未验证**（本机没有 NDK/CDK）：`cmake --preset android` 会停在「找不到 Android NDK」并给出安装提示；native/gradle 侧没有实际跑过 |
| iOS | ⚠️ **未验证**（本机只有 Command Line Tools，没有完整 Xcode，也没有 iOS SDK）：`cmake --preset ios` 因缺少 Xcode 生成器而失败；xcframework/SDK 相关未实测 |

**已知限制（Android）**

1. **纹理读不到**：`LoadTexture()` 走 libpng + 文件路径，而 APK 里的 `assets/` 没有文件系统路径 →
   `imgui_tour` / `flow_box` 里的贴图在 Android 上会退化成占位（字体不受影响）。修法是加一个
   `SDL_RWops` 读进内存再用 libpng 的 `png_set_read_fn` 解码。
2. 音频/震动等多模态权限没接（当前只有 `VIBRATE` 声明）。
3. 只配了 `arm64-v8a`；其它 ABI 改 `GUI_DEV_ANDROID_ABI`。

**已知限制（iOS）**

1. 真机需要签名团队；默认关闭签名只适合模拟器。
2. 未做启动图/图标（`UILaunchScreen` 空字典 = 纯黑，可用）。

---

## 6. 加新平台时要动哪里

1. `CMakeLists.txt` 的平台判定里加分支 + `add_compile_definitions(GUI_DEV_PLATFORM_xxx)`；
2. `framework/platform/Backend.h` 的 `PlatformKind` / `PlatformName()` 加一项；
3. 平台服务：在 `framework/platform/backends/sdl2/` 加一份 `XxxPlatform.cpp` 并在 CMake 里选进去；
4. `AssetPaths.cpp` 加资源查找顺序；
5. `CMakePresets.json` 加 preset（工具链/生成器/依赖模式）。
