#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="gui_dev_demo"
JOBS="${JOBS:-2}"

usage() {
    cat <<'EOF'
用法：scripts/build_switch.sh [--target <目标>] [--jobs <并行数>]
依赖：devkitPro（devkitA64、libnx、switch-sdl2、switch-pkg-config）、CMake、Ninja。
先设置 DEVKITPRO 指向 devkitPro 根目录；NRO 生成于 build/switch/dist/。
EOF
}
die() { printf '[switch-build] 错误：%s\n' "$*" >&2; exit 1; }

while (($#)); do
    case "$1" in
        --target) TARGET="${2:?--target 要有目标名}"; shift 2 ;;
        --jobs) JOBS="${2:?--jobs 要有并行数}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) die "未知参数：$1（使用 --help 查看用法）" ;;
    esac
done
[[ -n "${DEVKITPRO:-}" ]] || die '未设置 DEVKITPRO；请安装 devkitPro 并设置环境变量。'
[[ -f "${DEVKITPRO}/cmake/Switch.cmake" ]] || die "${DEVKITPRO} 下找不到 cmake/Switch.cmake"
for cmd in cmake ninja; do command -v "$cmd" >/dev/null || die "缺少 $cmd"; done

cd "$ROOT"
cmake --preset switch
cmake --build --preset switch --target "$TARGET" --parallel "$JOBS"
printf '[switch-build] 完成：build/switch/dist/%s.nro\n' "$TARGET"
