// 平台服务生命周期。
//
// Switch 上是 pl:u（HOS 共享字体）与 romfs（打包进 NRO 的资源）；
// 桌面上是空实现。必须在 CollectPlatformFontSources() 与任何 LoadTexture()
// 之前完成，在 Shutdown 时按相反顺序释放。
#pragma once

namespace gui_dev {

// 失败不致命：只表示共享字体/打包资源不可用，UI 仍应能起来。
bool PlatformServicesInit();
void PlatformServicesShutdown();

} // namespace gui_dev
