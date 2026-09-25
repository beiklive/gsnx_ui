#!/usr/bin/env bash
# 校验 iOS .app / .ipa 的"最小可安装、可启动"条件。
#
# 用法:
#   scripts/verify_ios_app.sh <xxx.app|xxx.ipa> [--github] [--expect-signed]
#
# 为什么需要它：iOS 对装不进去/点不开的 app 往往只表现为桌面上一个图标
# （或名字旁边带下载箭头），不会给出明确报错。这里把已知会导致这种状态的
# 硬性条件逐条查一遍，出问题就明确指出是哪一条。
#
# --github  失败项输出 GitHub Actions ::error:: 注解，方便在 CI 页面直接看到
# --expect-signed  要求 app 必须已签名（默认允许未签名，未签名只给警告）
set -u

RAW="${1:-}"
GITHUB=0
EXPECT_SIGNED=0
for arg in "$@"; do
    case "${arg}" in
        --github) GITHUB=1 ;;
        --expect-signed) EXPECT_SIGNED=1 ;;
    esac
done

if [[ -z "${RAW}" || ! -e "${RAW}" ]]; then
    echo "用法: $0 <xxx.app|xxx.ipa> [--github] [--expect-signed]" >&2
    exit 2
fi

INPUT="$(cd "$(dirname "${RAW}")" && pwd)/$(basename "${RAW}")"
WORK=""
cleanup() {
    [[ -n "${WORK}" && -d "${WORK}" ]] && rm -rf "${WORK}"
    return 0
}
trap cleanup EXIT

# .ipa 先解开取出 Payload/*.app
APP=""
case "${INPUT}" in
    *.ipa)
        WORK="$(mktemp -d)"
        if ! unzip -qq "${INPUT}" -d "${WORK}"; then
            echo "::error::IPA 解压失败: ${INPUT}" 2>/dev/null || true
            echo "[FAIL] IPA 解压失败: ${INPUT}"
            exit 1
        fi
        APP="$(find "${WORK}/Payload" -maxdepth 1 -name '*.app' -print -quit 2>/dev/null)"
        if [[ -z "${APP}" ]]; then
            echo "[FAIL] IPA 里没有 Payload/*.app"
            exit 1
        fi
        ;;
    *.app) APP="${INPUT}" ;;
    *)
        echo "[FAIL] 只接受 .app 或 .ipa: ${INPUT}"
        exit 2
        ;;
esac

FAILED=0
WARNED=0

ok()   { printf '  [ OK ] %s\n' "$1"; }
warn() { printf '  [WARN] %s\n' "$1"; WARNED=$((WARNED + 1)); }
fail() {
    printf '  [FAIL] %s\n' "$1"
    [[ "${GITHUB}" == "1" ]] && printf '::error::%s\n' "$1"
    FAILED=$((FAILED + 1))
}

echo "== 校验 $(basename "${INPUT}") =="
echo "   app: ${APP}"
[[ -n "${WORK}" ]] && echo "   解包目录: ${WORK}"

# ---------------------------------------------------------------- 可执行文件
EXE_NAME="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "${APP}/Info.plist" 2>/dev/null || true)"
if [[ -z "${EXE_NAME}" ]]; then
    fail "Info.plist 缺少 CFBundleExecutable（app 没有入口，点开必然无效）"
else
    EXE="${APP}/${EXE_NAME}"
    if [[ ! -f "${EXE}" ]]; then
        fail "CFBundleExecutable=${EXE_NAME} 指向的文件不在包内（点开必然无效）"
    elif [[ ! -x "${EXE}" ]]; then
        fail "可执行文件缺少执行权限: ${EXE_NAME}"
    else
        ok "可执行文件存在且有执行权限: ${EXE_NAME}"
    fi
fi

# 架构与平台（iOS 真机只认 arm64 且 platform 必须是 IOS）
if [[ -n "${EXE_NAME}" && -f "${APP}/${EXE_NAME}" ]]; then
    EXE="${APP}/${EXE_NAME}"
    ARCHS="$(lipo -archs "${EXE}" 2>/dev/null || true)"
    if [[ "${ARCHS}" != *arm64* ]]; then
        fail "可执行文件不含 arm64（当前: ${ARCHS:-未知}），iOS 真机装不了"
    else
        ok "架构: ${ARCHS}"
    fi
    PLATFORM="$(otool -l "${EXE}" 2>/dev/null | awk '/LC_BUILD_VERSION/{f=1} f&&/platform/{print $2; exit}' || true)"
    case "${PLATFORM}" in
        2) ok "Mach-O platform = IOS" ;;
        7) warn "Mach-O platform 是模拟器(IOSSIMULATOR)，真机装不了" ;;
        "") warn "读不到 LC_BUILD_VERSION，无法确认 Mach-O platform" ;;
        *) fail "Mach-O platform 不是 IOS（值: ${PLATFORM}），真机装不上/点不开" ;;
    esac
fi

# ------------------------------------------------------------------ Info.plist
PLIST_KEY() { /usr/libexec/PlistBuddy -c "Print :$1" "${APP}/Info.plist" 2>/dev/null || true; }

BUNDLE_ID="$(PLIST_KEY CFBundleIdentifier)"
if [[ -z "${BUNDLE_ID}" ]]; then
    fail "CFBundleIdentifier 为空（无法安装）"
elif [[ "${BUNDLE_ID}" =~ [^A-Za-z0-9.-] ]]; then
    fail "CFBundleIdentifier 含非法字符（只能字母/数字/点/连字符）: ${BUNDLE_ID}"
else
    ok "CFBundleIdentifier: ${BUNDLE_ID}"
fi

# 空 CFBundleSupportedPlatforms 会让 iOS 认为该包不支持当前平台，
# 装完就只剩一个点不开的占位图标 —— 这正是本项目踩过的坑。
SUPPORTED="$(PLIST_KEY CFBundleSupportedPlatforms:0)"
if [[ -z "${SUPPORTED}" ]]; then
    fail "CFBundleSupportedPlatforms 缺失或为空（iOS 会当成平台不支持，表现为只有图标、点不开）"
elif [[ "${SUPPORTED}" != "iPhoneOS" && "${SUPPORTED}" != "iPhoneSimulator" ]]; then
    fail "CFBundleSupportedPlatforms 值不合法: ${SUPPORTED}"
else
    ok "CFBundleSupportedPlatforms: ${SUPPORTED}"
fi

DT_PLATFORM="$(PLIST_KEY DTPlatformName)"
if [[ -z "${DT_PLATFORM}" ]]; then
    fail "DTPlatformName 为空（Xcode 未注入，包不完整）"
else
    ok "DTPlatformName: ${DT_PLATFORM}"
fi

MIN_OS="$(PLIST_KEY MinimumOSVersion)"
if [[ -z "${MIN_OS}" ]]; then
    fail "MinimumOSVersion 为空（安装时会被拒）"
else
    ok "MinimumOSVersion: ${MIN_OS}"
fi

if [[ "$(PLIST_KEY LSRequiresIPhoneOS)" == "true" ]]; then
    ok "LSRequiresIPhoneOS = true"
else
    fail "LSRequiresIPhoneOS 不是 true（会被当成 macOS app）"
fi

if [[ -z "$(PLIST_KEY UIDeviceFamily:0)" ]]; then
    fail "UIDeviceFamily 为空（Xcode 未注入）"
else
    ok "UIDeviceFamily: $(PLIST_KEY UIDeviceFamily | tr -d ' \n')"
fi

if [[ -z "$(PLIST_KEY UILaunchScreen 2>/dev/null)" && -z "$(PLIST_KEY UILaunchStoryboardName)" ]]; then
    fail "既没有 UILaunchScreen 也没有 UILaunchStoryboardName（iOS 14+ 会以兼容模式或被拒）"
else
    ok "有启动屏配置"
fi

# -------------------------------------------------------------------- 图标
ICON_ENTRIES="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIcons:CFBundlePrimaryIcon:CFBundleIconFiles' "${APP}/Info.plist" 2>/dev/null || true)"
if [[ -z "${ICON_ENTRIES}" ]]; then
    # 也允许 Xcode 注入的 CFBundleIcons~ipad 或老式 CFBundleIconFiles
    ICON_ENTRIES="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIconFiles' "${APP}/Info.plist" 2>/dev/null || true)"
fi
if [[ -z "${ICON_ENTRIES}" ]]; then
    warn "Info.plist 没声明图标（桌面会显示默认图标，但不影响能否打开）"
else
    ICON_MISSING=""
    while IFS= read -r entry; do
        entry="$(echo "${entry}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [[ -z "${entry}" ]] && continue
        case "${entry}" in Array|Dictionary|\{|\}|"") continue ;; esac
        if ! ls "${APP}/${entry}"*.png >/dev/null 2>&1; then
            ICON_MISSING="${ICON_MISSING} ${entry}"
        fi
    done <<< "$(echo "${ICON_ENTRIES}" | grep -v '^Array' || true)"
    if [[ -n "${ICON_MISSING}" ]]; then
        fail "Info.plist 声明的图标文件不在包内:${ICON_MISSING}"
    else
        ok "Info.plist 声明的图标文件都在包内"
    fi
fi

# ---------------------------------------------------------------- 运行期资源
# 缺字体/图片会导致启动即失败，同样表现为"点不开"。
ASSET_MISSING=""
for f in assets/font/switch_font.ttf assets/font/MaterialIcons-Regular.ttf; do
    [[ -f "${APP}/${f}" ]] || ASSET_MISSING="${ASSET_MISSING} ${f}"
done
[[ -d "${APP}/assets/img" ]] || ASSET_MISSING="${ASSET_MISSING} assets/img"
if [[ -n "${ASSET_MISSING}" ]]; then
    fail "包内缺少运行期资源:${ASSET_MISSING}（启动会失败）"
else
    ok "运行期资源齐全 (assets/font, assets/img)"
fi

# -------------------------------------------------------------------- 签名
# 只认"带证书的签名"。自签/adhoc 没有 Authority=，iOS 真机装上照样点不开。
SIGN_INFO="$(codesign -dv "${APP}" 2>&1 || true)"
SIGN_AUTH="$(echo "${SIGN_INFO}" | grep -m1 'Authority=' || true)"
if [[ -n "${SIGN_AUTH}" ]]; then
    ok "已签名: ${SIGN_AUTH}"
elif [[ "${EXPECT_SIGNED}" == "1" ]]; then
    fail "app 没有带证书的签名（需要 Apple 开发证书或 Ad Hoc 分发证书）"
else
    warn "app 未签名（CI 产出未签名包属正常）：安装前必须用自己的 Apple ID 重新签名并信任证书"
fi

# ---------------------------------------------------------------- 打包卫生
if [[ -f "${APP}/assets/.gitkeep" || -d "${APP}/assets/ios" ]]; then
    warn "包内混入了 .gitkeep/assets/ios（无害但没必要，建议收紧拷贝规则）"
fi

SIZE="$(du -sh "${INPUT}" | awk '{print $1}')"
echo "== 结果: FAIL=${FAILED} WARN=${WARNED} 体积=${SIZE} =="
if [[ "${FAILED}" -gt 0 ]]; then
    echo "=> 该包存在会导致『装上去只有图标、点不开』的问题，请修好后重试。"
    exit 1
fi
echo "=> 硬性检查全部通过。未签名包仍需自行重签名并在 设置→通用→VPN与设备管理 里信任证书。"
exit 0
