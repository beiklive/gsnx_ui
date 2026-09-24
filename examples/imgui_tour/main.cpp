#include "core/App.h"
#include "examples/imgui_tour/ImGuiTourApp.h"

int main(int, char**) {
    gui_dev::tour::TourApp app;
    return gui_dev::AppRunner(app).Run();
}
