#include "core/App.h"
#include "examples/pause_menu/PauseDemo.h"

int main(int, char**) {
    gui_dev::demo::PauseDemoApp app;
    return gui_dev::AppRunner(app).Run();
}
