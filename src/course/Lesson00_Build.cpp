// ============================================================================
// 课时 0 · 编译与运行（动手之前先把它跑起来）
// ============================================================================
//
// 【目标】
//   在自己的机器上把这套课程编译出来并跑起来，遇到报错知道查哪里。
//
// ---------------------------------------------------------------------------
// 一、依赖
// ---------------------------------------------------------------------------
//   共同：
//     · git submodule（imgui 源码）—— 必须拉，否则 CMake 会直接报
//       "third_party/imgui is empty"
//         git submodule update --init --recursive
//     · CMake >= 3.20、Ninja（预设里用的是 Ninja 生成器）
//
//   macOS：
//     · Xcode Command Line Tools（clang）
//     · brew install sdl2 libpng            # sdl2-compat 也可以
//   Switch：
//     · devkitPro（devkitA64 + libnx，默认 /opt/devkitpro）
//     · switch-sdl2、switch-pkg-config、switch-libpng（dkp-pacman 装）
//
//   注意：本项目**不用** SDL_image（mac 上没有），PNG 解码用 libpng（两端都有）。
//
// ---------------------------------------------------------------------------
// 二、编译与运行（三条命令）
// ---------------------------------------------------------------------------
//   macOS：
//       cmake --preset mac
//       cmake --build --preset mac
//       ./build/mac/gui_dev_course
//   只编课程这一个目标（改课时时最快）：
//       cmake --build --preset mac --target gui_dev_course
//
//   Switch：
//       cmake --preset switch
//       cmake --build --preset switch
//       # 产物：build/switch/dist/*.nro（4 个 demo 各一个）
//       # 拷贝：gui_dev_course.nro -> sdmc:/switch/，assets/img -> sdmc:/switch/GUI_DEV/assets/img
//
//   Release 版：cmake --preset mac-release（更快、更接近实机表现）
//
// ---------------------------------------------------------------------------
// 三、四个可执行目标分别是什么
// ---------------------------------------------------------------------------
//   gui_dev_demo         组件预览（可聚焦 Box / 流光边框 / 字体与图标）
//   gui_dev_pause_demo   暂停菜单 Demo（Persona 风格动态 UI，动作全 Mock）
//   gui_dev_widget_demo  自定义控件 8 个最小例子
//   gui_dev_course       本课程（L/R 切课，Esc 退出）
//   库目标：imgui / gui_dev_backend / gui_dev（UI 都在这三个里，demo 只是壳）
//
// ---------------------------------------------------------------------------
// 四、加一课要改哪几处（4 步）
// ---------------------------------------------------------------------------
//   1) 新建 src/course/LessonNN_Xxx.cpp：实现一个 Scene + 一个工厂函数
//   2) src/course/CourseLessons.h   声明工厂
//   3) src/course/CourseLessons.cpp 表里加一行 {标题, 工厂}
//   4) CMakeLists.txt 的 gui_dev_course 源文件列表里加一行
//   （课时之间互不影响：改坏一课不影响其他课编译）
//
// ---------------------------------------------------------------------------
// 五、编译常见问题（都是本项目真实踩过的）
// ---------------------------------------------------------------------------
//   报错：third_party/imgui is empty
//       原因：submodule 没拉。解决：git submodule update --init --recursive
//
//   报错：Checking for module 'sdl2' -> No package 'sdl2' found
//       原因：PATH 里 devkitPro 的 pkg-config 排在 homebrew 前面，而它只认 Switch portlibs。
//       解决：预设里已显式指定 PKG_CONFIG_EXECUTABLE=/opt/homebrew/bin/pkg-config。
//
//   报错（Switch）：undefined reference 到自己库里的符号（例如 Theme::Apply）
//       原因：macOS 宿主 /usr/bin/ar 是 llvm-ar，生成的归档让 GNU ld 解析不到成员。
//       解决：工具链已把 CMAKE_AR 锁到 devkitPro 的 aarch64-none-elf-ar。
//
//   报错（Switch）：undefined reference to waitpid / execvp
//       原因：imgui 默认 shell 处理用了 fork/execvp，libnx 没有。
//       解决：Switch 构建已定义 IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS。
//
//   运行期：字体/图标全是方块，或按键图标不显示
//       原因：资源没找到。mac 查 assets/ 是否存在；Switch 查 romfs 或
//             sdmc:/switch/GUI_DEV/assets/。启动时 stderr 有字形自检日志。
//
//   运行期：退出瞬间崩溃（开发中遇到过）
//       原因：对象比 Backend 活得久，析构时回调了已销毁的后端 -> 详见课时 2。
//
// ---------------------------------------------------------------------------
// 【练习】
//   1. 用 --target gui_dev_course 只编课程，改一行 Note 文字再编一次，
//      观察只重编了一个 .cpp（这就是把课时拆成独立文件的收益）。
//   2. 故意把 CMakeLists 里课时 13 那行注释掉再编，看链接错误长什么样，
//      再恢复 —— 体会"注册表 + 工厂"是怎么把课与课解耦的。
//   3. 跑 cmake --preset mac-release，对比 Debug 版的帧率（课时 12 的指标）。
// ---------------------------------------------------------------------------
// 【验收】
//   1. 不看文档写出 mac 与 Switch 的编译命令。
//   2. 加一课要改哪 4 处？
//   3. "third_party/imgui is empty" 是什么原因？怎么修？
// ============================================================================

#include <imgui.h>

#include "course/CourseLessons.h"
#include "course/CourseUi.h"
#include "platform/Backend.h"
#include "ui/Components.h"
#include "ui/UiContext.h"

namespace gui_dev::course {

class Lesson00BuildScene final : public Scene {
public:
    const char* Name() const override { return "lesson00"; }

    void OnRender(UiContext& ui) override {
        if (!Components::BeginPanel(ui, "课时 0 · 编译与运行",
                                    "先把课程跑起来：依赖 -> 编译 -> 运行 -> 排错")) {
            Components::EndPanel(ui);
            return;
        }

        // ---- 运行时实况（证明你已经编出来了）-------------------------------
        Section("一、你已经编译成功了（下面是本次运行的实况）");
        KeyValue("平台 / 驱动", "%s / %s", PlatformName(), ui.GetBackend().DriverName());
        KeyValue("编译期版本", "GUI_DEV_VERSION = %s", ui.AppVersion());
        KeyValue("构建类型", "%s", kBuildType);
        KeyValue("课时总数", "%d（L/R 切换）", static_cast<int>(CourseEntries().size()));
        Note("这些值来自编译期宏与后端接口，不是写死的字符串。");

        // ---- 依赖 ----------------------------------------------------------
        Section("二、依赖（缺一个就编不过）");
        Bullet("imgui：git submodule update --init --recursive（不拉会直接报 empty）");
        Bullet("mac：clang + cmake(>=3.20) + ninja + brew install sdl2 libpng");
        Bullet("Switch：devkitPro(devkitA64/libnx) + switch-sdl2 + switch-pkg-config + switch-libpng");
        Bullet("注意：本项目不用 SDL_image（mac 上没有），PNG 走 libpng（两端都有）");

        // ---- 编译命令 ------------------------------------------------------
        Section("三、编译与运行");
        Code("# macOS\n"
             "cmake --preset mac\n"
             "cmake --build --preset mac\n"
             "./build/mac/gui_dev_course\n"
             "\n"
             "# 只编课程这一个目标（改课时时最快）\n"
             "cmake --build --preset mac --target gui_dev_course\n"
             "\n"
             "# Switch：产物 build/switch/dist/*.nro\n"
             "cmake --preset switch && cmake --build --preset switch");

        // ---- 四个目标 ------------------------------------------------------
        Section("四、四个可执行目标");
        ImGui::TextUnformatted("gui_dev_demo         组件预览（Box / 流光 / 字体图标）");
        ImGui::TextUnformatted("gui_dev_pause_demo   暂停菜单 Demo（Persona 风格，动作 Mock）");
        ImGui::TextUnformatted("gui_dev_widget_demo  自定义控件 8 个最小例子");
        ImGui::TextUnformatted("gui_dev_course       本课程");
        Note("库目标 imgui / gui_dev_backend / gui_dev 才是 UI 本体，demo 只是壳。");

        // ---- 加一课 --------------------------------------------------------
        Section("五、加一课要改 4 处");
        Code("1) 新建 src/course/LessonNN_Xxx.cpp   （Scene + 工厂函数）\n"
             "2) CourseLessons.h    声明工厂\n"
             "3) CourseLessons.cpp  表里加一行 {标题, 工厂}\n"
             "4) CMakeLists.txt     gui_dev_course 源文件列表加一行");

        // ---- 排错 ----------------------------------------------------------
        Section("六、编译/运行常见问题（本项目真实踩过）");
        if (ImGui::BeginTable("##build_faq", 2,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("现象", ImGuiTableColumnFlags_WidthFixed, 340.0f);
            ImGui::TableSetupColumn("原因 / 解决");
            ImGui::TableHeadersRow();
            const struct {
                const char* symptom;
                const char* fix;
            } kFaq[] = {
                {"third_party/imgui is empty",
                 "submodule 没拉：git submodule update --init --recursive"},
                {"No package 'sdl2' found（mac）",
                 "PATH 里 devkitPro 的 pkg-config 在前：预设已指定 homebrew 的 pkg-config"},
                {"Switch: undefined reference 到自己库的符号",
                 "宿主 llvm-ar 归档格式问题：工具链已锁 devkitPro 的 ar"},
                {"Switch: undefined reference to waitpid/execvp",
                 "imgui 默认 shell 函数：已定义 IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS"},
                {"运行时字体/图标全是方块",
                 "资源没找到：mac 查 assets/，Switch 查 romfs 或 sdmc 路径；看 stderr 自检日志"},
                {"退出瞬间崩溃（开发中遇到）",
                 "对象比 Backend 活得久：详见课时 2 的析构顺序"},
            };
            for (const auto& row : kFaq) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(row.symptom);
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", row.fix);
            }
            ImGui::EndTable();
        }

        Section("七、练习与验收");
        Bullet("练习：--target gui_dev_course 改一行文字再编，看只重编一个 .cpp");
        Bullet("练习：把课时 13 那行源文件注释掉再编，看链接错误长什么样");
        Bullet("验收：写出 mac/Switch 编译命令；加一课改哪 4 处？");

        EndLesson(ui);
    }

private:
#if defined(NDEBUG)
    static constexpr const char* kBuildType = "Release";
#else
    static constexpr const char* kBuildType = "Debug";
#endif
};

std::unique_ptr<Scene> CreateLesson00Build() { return std::make_unique<Lesson00BuildScene>(); }

} // namespace gui_dev::course
