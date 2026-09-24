#include "core/App.h"
#include "demo/DemoApp.h"

int main(int, char**) {
    gui_dev::demo::DemoApp app;
    return gui_dev::AppRunner(app).Run();
}
