#include "core/App.h"
#include "examples/flow_box/DemoApp.h"

int main(int, char**) {
    gui_dev::demo::DemoApp app;
    return gui_dev::AppRunner(app).Run();
}
