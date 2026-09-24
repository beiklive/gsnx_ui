# Switch (devkitA64 / libnx) 工具链入口。
#   cmake --preset switch
#   # 或手动：-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/Switch.cmake
# 需要 devkitPro（默认 /opt/devkitpro）；可用 DEVKITPRO 环境变量或
# -DDEVKITPRO=/path 覆盖。

if(NOT DEVKITPRO)
    if(DEFINED ENV{DEVKITPRO})
        set(DEVKITPRO "$ENV{DEVKITPRO}" CACHE PATH "devkitPro root" FORCE)
    else()
        set(DEVKITPRO "/opt/devkitpro" CACHE PATH "devkitPro root" FORCE)
    endif()
endif()

if(NOT EXISTS "${DEVKITPRO}/cmake/Switch.cmake")
    message(FATAL_ERROR
        "devkitPro not found at ${DEVKITPRO} (missing cmake/Switch.cmake). "
        "Install via https://devkitpro.org/wiki/Getting_Started or pass -DDEVKITPRO=<path>.")
endif()

include(${DEVKITPRO}/cmake/Switch.cmake)

# 必须锁定 devkitPro 的 binutils：macOS 宿主 /usr/bin/ar 是 llvm-ar，
# 它生成的归档（成员名带尾斜杠）会让 aarch64-none-elf-ld 解析不到成员符号，
# 表现为莫名其妙的 undefined reference。这两个变量必须在 project() 之前生效。
set(CMAKE_AR "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ar" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ranlib" CACHE FILEPATH "" FORCE)

set(GUI_DEV_PLATFORM "switch" CACHE STRING "" FORCE)
