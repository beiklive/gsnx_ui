#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build/linux"
CONFIG="Release"
TARGET="gui_dev_demo"

usage() {
    cat <<'EOF'
用法：scripts/build_linux.sh [--build-dir <目录>] [--config <Debug|Release>] [--target <目标>]
依赖：CMake、Ninja、pkg-config、libsdl2-dev、libpng-dev。
产物：build/linux/gui_dev_demo
EOF
}
die() { printf '[linux-build] 错误：%s\n' "$*" >&2; exit 1; }

while (($#)); do
    case "$1" in
        --build-dir) BUILD_DIR="${2:?--build-dir 要有路径}"; shift 2 ;;
        --config) CONFIG="${2:?--config 要有配置名}"; shift 2 ;;
        --target) TARGET="${2:?--target 要有目标名}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) die "未知参数：$1（使用 --help 查看用法）" ;;
    esac
done
for cmd in cmake ninja pkg-config; do command -v "$cmd" >/dev/null || die "缺少 $cmd"; done
pkg-config --exists sdl2 || die '缺少 SDL2 开发包（例如 Debian/Ubuntu: libsdl2-dev）'
pkg-config --exists libpng || die '缺少 libpng 开发包（例如 Debian/Ubuntu: libpng-dev）'
[[ "$BUILD_DIR" = /* ]] || BUILD_DIR="${ROOT}/${BUILD_DIR}"

cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
    -DGUI_DEV_PLATFORM=linux -DGUI_DEV_BACKEND=sdl2 \
    -DGUI_DEV_DEPS_MODE=package -DCMAKE_BUILD_TYPE="$CONFIG"
cmake --build "$BUILD_DIR" --target "$TARGET" --parallel
printf '[linux-build] 完成：%s/%s\n' "$BUILD_DIR" "$TARGET"
