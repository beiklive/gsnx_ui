#include "core/App.h"
#include "demo/widgets/WidgetDemo.h"

int main(int, char**) {
    gui_dev::demo::WidgetDemoApp app;
    return gui_dev::AppRunner(app).Run();
}
