#!/usr/bin/env bash
# 把 GUI_DEV 的 iOS 构建产物打成 .ipa。
#
# 三条路：
#   * 未签名（默认，不需要开发者账号）：Payload/ 打包成 .ipa —— 用 AltStore / Sideloadly /
#     TrollStore / zsign 之类重签后安装。
#   * ad-hoc 签名（--adhoc）：codesign -s - 就地签名，越狱 / TrollStore 设备可直接装。
#   * 团队签名（--team XXXXXXXXXX [--method development|ad-hoc|app-store]）：
#     xcodebuild archive + -exportArchive，产出可直接分发/安装的 .ipa。
#
# 依赖：完整 Xcode（不是只有 Command Line Tools）。装了 Xcode 但 xcode-select 还指着
# CommandLineTools 的话，用 DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer 前缀跑。
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

TARGET="gui_dev_demo"
CONFIG="Release"
MODE="auto"           # auto | unsigned | adhoc
METHOD="development"  # development | ad-hoc | app-store | enterprise（团队签名时用）
TEAM=""
SIMULATOR=0
PACKAGE_ONLY=""
OUT_DIR="${ROOT}/dist/ios"

log() { printf '\033[1m[ios-ipa]\033[0m %s\n' "$*"; }
die() { printf '\033[1;31m[ios-ipa] 错误：\033[0m%s\n' "$*" >&2; exit 1; }

usage() {
    cat <<'EOF'
用法：scripts/build_ios_ipa.sh [选项]

  --target <名字>      要打包的目标（默认 gui_dev_demo）
  --config <Debug|Release>  构建配置（默认 Release）
  --out <目录>         产物目录（默认 dist/ios）
  --unsigned           强制产出未签名 IPA
  --adhoc              构建后就地 ad-hoc 签名（codesign -s -）
  --team <TEAM_ID>     用团队签名走 xcodebuild archive + -exportArchive
  --method <m>         导出方式：development | ad-hoc | app-store | enterprise
  --sim                只构建模拟器版本（不产出 IPA）
  --package-only <app> 跳过 Xcode，只把已有的 .app 打成 IPA（CI / 测试用）
  -h, --help           显示本帮助

例：
  scripts/build_ios_ipa.sh                              # 未签名 IPA
  scripts/build_ios_ipa.sh --adhoc                      # ad-hoc 签名 IPA
  scripts/build_ios_ipa.sh --team ABCDE12345 --method development
  DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer scripts/build_ios_ipa.sh
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --target) TARGET="${2:?--target 后面要跟目标名}"; shift 2 ;;
        --config) CONFIG="${2:?--config 后面要跟配置名}"; shift 2 ;;
        --out) OUT_DIR="${2:?--out 后面要跟目录}"; shift 2 ;;
        --unsigned) MODE="unsigned"; shift ;;
        --adhoc) MODE="adhoc"; shift ;;
        --team) TEAM="${2:?--team 后面要跟 TEAM_ID}"; shift 2 ;;
        --method) METHOD="${2:?--method 后面要跟导出方式}"; shift 2 ;;
        --sim) SIMULATOR=1; shift ;;
        --package-only) PACKAGE_ONLY="${2:?--package-only 后面要跟 .app 路径}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) die "不认识的参数：${1}（--help 看用法）" ;;
    esac
done

# 把一个 .app 打成 .ipa：Payload/<名字>.app 再 zip 成 .ipa（这是 .ipa 的全部结构）
package_ipa() {
    local app="$1" out="$2"
    [ -d "$app" ] || die "不是 .app 目录：$app"
    [ -f "$app/Info.plist" ] || die "$app 里没有 Info.plist，不像一个 app bundle"
    local tmp
    tmp="$(mktemp -d)"
    mkdir -p "${tmp}/Payload"
    cp -R "$app" "${tmp}/Payload/"
    mkdir -p "$(dirname "$out")"
    rm -f "$out"
    ( cd "$tmp" && zip -qry "$out" Payload )
    rm -rf "$tmp"
    local size
    size="$(du -h "$out" | cut -f1)"
    log "IPA：${out}（${size}）"
    log "结构：$(unzip -l "${out}" | grep -o "Payload/[^/]*\.app" | head -1)（共 $(unzip -l "${out}" | grep -c "Payload/") 项）"
}

# --package-only：完全不碰 Xcode，只做打包（CI 与本地测试都走这条）
if [ -n "${PACKAGE_ONLY}" ]; then
    package_ipa "${PACKAGE_ONLY}" "${OUT_DIR}/$(basename "${PACKAGE_ONLY}" .app).ipa"
    exit 0
fi

# ---- 1) 检查 Xcode / iOS SDK ----
DEVDIR="${DEVELOPER_DIR:-$(xcode-select -p 2>/dev/null || true)}"
case "${DEVDIR}" in
    *Xcode*.app/Contents/Developer) ;;
    *) die "当前开发者目录是 ${DEVDIR:-<空>}，不是完整 Xcode。
  Command Line Tools 不带 iOS SDK，装 Xcode 后二选一：
    sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
    DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer $0 $*
  装 Xcode：App Store，或 brew install --cask xcodes && xcodes install --latest" ;;
esac
command -v xcodebuild >/dev/null 2>&1 || die "PATH 里没有 xcodebuild（检查 Xcode 是否装全）"

if [ "${SIMULATOR}" = "1" ]; then SDK_NAME="iphonesimulator"; else SDK_NAME="iphoneos"; fi
SDK_PATH="$(xcrun --sdk "${SDK_NAME}" --show-sdk-path 2>/dev/null || true)"
[ -n "${SDK_PATH}" ] && [ -d "${SDK_PATH}" ] || die "找不到 ${SDK_NAME} SDK：${SDK_PATH:-<空>}"
log "Xcode：${DEVDIR}"
log "SDK：${SDK_NAME} → ${SDK_PATH}"

# ---- 2) 配置 + 构建 ----
BUILD_DIR="${ROOT}/build/ios$([ "${SIMULATOR}" = "1" ] && echo "-sim" || true)"
log "配置 + 构建 ${TARGET}（${CONFIG}，${BUILD_DIR}）"
if [ "${SIMULATOR}" = "1" ]; then
    cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Xcode \
        -DCMAKE_TOOLCHAIN_FILE="${ROOT}/cmake/toolchains/iOS.cmake" \
        -DGUI_DEV_PLATFORM=ios -DGUI_DEV_BACKEND=sdl2 -DGUI_DEV_DEPS_MODE=fetch \
        -DGUI_DEV_IOS_SIMULATOR=ON
else
    cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Xcode \
        -DCMAKE_TOOLCHAIN_FILE="${ROOT}/cmake/toolchains/iOS.cmake" \
        -DGUI_DEV_PLATFORM=ios -DGUI_DEV_BACKEND=sdl2 -DGUI_DEV_DEPS_MODE=fetch
fi
cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target "${TARGET}"

APP_PATH="$(find "${BUILD_DIR}" -maxdepth 4 -name "${TARGET}.app" -type d | head -1)"
[ -n "${APP_PATH}" ] || die "没找到 ${TARGET}.app（构建失败？）"
log "app：${APP_PATH}"

# ---- 3) 模拟器：不打包 ----
if [ "${SIMULATOR}" = "1" ]; then
    log "模拟器构建完成：${APP_PATH}"
    log "安装：xcrun simctl install booted \"${APP_PATH}\""
    exit 0
fi

# ---- 4) 团队签名：走 xcodebuild 归档导出 ----
if [ -n "${TEAM}" ]; then
    command -v xcodebuild >/dev/null 2>&1 || die "团队签名需要 xcodebuild"
    PROJECT="$(find "${BUILD_DIR}" -maxdepth 1 -name '*.xcodeproj' | head -1)"
    [ -n "${PROJECT}" ] || die "找不到 .xcodeproj"
    ARCHIVE="${OUT_DIR}/${TARGET}.xcarchive"
    EXPORT_DIR="${OUT_DIR}/export"
    PLIST="${OUT_DIR}/ExportOptions.plist"
    mkdir -p "${OUT_DIR}"
    cat > "${PLIST}" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
    <key>method</key><string>${METHOD}</string>
    <key>teamID</key><string>${TEAM}</string>
    <key>compileBitcode</key><false/>
    <key>stripSwiftSymbols</key><true/>
    <key>signingStyle</key><string>automatic</string>
</dict></plist>
EOF
    log "归档（team=${TEAM}）…"
    xcodebuild -project "${PROJECT}" -scheme "${TARGET}" -configuration "${CONFIG}" \
        -destination 'generic/platform=iOS' -archivePath "${ARCHIVE}" \
        DEVELOPMENT_TEAM="${TEAM}" CODE_SIGN_STYLE=Automatic archive
    log "导出 IPA（method=${METHOD}）…"
    xcodebuild -exportArchive -archivePath "${ARCHIVE}" \
        -exportOptionsPlist "${PLIST}" -exportPath "${EXPORT_DIR}"
    IPA="$(find "${EXPORT_DIR}" -name '*.ipa' | head -1)"
    [ -n "${IPA}" ] || die "导出没产出 .ipa（检查签名证书 / 描述文件）"
    log "IPA：${IPA}（$(du -h "${IPA}" | cut -f1)）"
    exit 0
fi

# ---- 5) 未签名 / ad-hoc：自己打 IPA ----
if [ "${MODE}" = "adhoc" ]; then
    if ! codesign --force --sign - "${APP_PATH}" 2>/dev/null; then
        die "ad-hoc 签名失败（codesign -s - 需要 Xcode 的 codesign 与可用 SDK）"
    fi
    log "已就地 ad-hoc 签名（codesign -s -）"
fi
package_ipa "${APP_PATH}" "${OUT_DIR}/${TARGET}.ipa"
log "安装提示：未签名 IPA 需要先用 AltStore / Sideloadly / TrollStore / zsign 重签；"
log "          越狱或 TrollStore 设备可直接装 --adhoc 版本。"
log "模拟器版：$0 --sim"
