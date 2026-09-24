#include "core/App.h"
#include "course/CourseApp.h"

int main(int, char**) {
    gui_dev::course::CourseApp app;
    return gui_dev::AppRunner(app).Run();
}
