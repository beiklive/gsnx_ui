#include "core/App.h"
#include "examples/flow_box/DemoApp.h"

#if defined(GUI_DEV_PLATFORM_android) || defined(GUI_DEV_PLATFORM_ios)
// 移动端 SDL 用 SDL_main 当入口：Android 是 SDL_android_main.c（JNI），
// iOS 是 SDL_uikitappdelegate（UIApplicationMain）。这个头必须出现在 main 定义之前。
#include <SDL_main.h>
#endif

int main(int, char**) {
    gui_dev::demo::DemoApp app;
    return gui_dev::AppRunner(app).Run();
}
