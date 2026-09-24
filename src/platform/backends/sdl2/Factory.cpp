// 后端工厂：每个后端目录各自实现一份，CMake 保证只有一个被编译。
#include "platform/Backend.h"
#include "platform/backends/sdl2/Sdl2Backend.h"

namespace gui_dev {

std::unique_ptr<Backend> CreatePlatformBackend() {
    return std::make_unique<Sdl2Backend>();
}

} // namespace gui_dev
