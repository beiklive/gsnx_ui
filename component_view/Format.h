// Format：通用文本格式化（组件库共用，不依赖任何平台接口）。
//
// 目前只有文件大小：1024 进制、单位自动切换，约定与 GBAStation 的 getFileSizeString 对齐
// （KB 以上保留 2 位小数）。文件列表按钮的右侧信息、图片浏览器的信息行都用它，
// 避免每个组件各写一份、单位/精度还不一致。
#pragma once

#include <cstdio>
#include <string>

namespace gui_dev::cv {

// bytes < 0 = 大小未知（返回空串，调用方自己决定画不画）；
// bytes == 0 = 真的 0 字节（返回 "0 B"）。
inline std::string FormatFileSize(long long bytes) {
    if (bytes < 0) {
        return {};
    }
    char buffer[32];
    if (bytes < 1024) {
        std::snprintf(buffer, sizeof(buffer), "%lld B", bytes);
        return buffer;
    }
    static const char* kUnits[] = {"KB", "MB", "GB", "TB", "PB"};
    double value = static_cast<double>(bytes) / 1024.0;
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    std::snprintf(buffer, sizeof(buffer), "%.2f %s", value, kUnits[unit]);
    return buffer;
}

} // namespace gui_dev::cv
