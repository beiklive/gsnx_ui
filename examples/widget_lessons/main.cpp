#include "core/App.h"
#include "examples/widget_lessons/WidgetDemo.h"

int main(int, char**) {
    gui_dev::demo::WidgetDemoApp app;
    return gui_dev::AppRunner(app).Run();
}
