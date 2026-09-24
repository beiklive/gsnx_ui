// assets/ 下资源的路径解析。
//
// 开发期（mac）与打包后（Switch）资产位置不同，但调用方只写
// "img/border_gradient.png" 这样的相对路径。
#pragma once

#include <string>

namespace gui_dev {

// 解析 assets/ 下的相对路径；找不到返回空串。
// 桌面：按工作目录/可执行文件上级/源码 assets 目录依次查找。
// Switch：sdmc:/switch/GUI_DEV/assets/ 与 romfs:/。
std::string ResolveAssetPath(const char* relative_path);

} // namespace gui_dev
