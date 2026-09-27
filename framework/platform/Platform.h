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

// 平台日志落地：把一行诊断同时写到「平台自己的日志位置」。
//   Switch      : 追加到 sdmc:/switch/GUI_DEV/gui_dev.log（掌机没有控制台，这个文件就是唯一的日志）
//   桌面 / Android: 空实现（桌面看 stderr；Android 的 stderr 会进 logcat）
// 宿主自己的日志也可以直接调它，不必各写一套。
void PlatformLogLine(const char* line);
// 平台日志文件路径（桌面/Android 返回空串）。启动时打印一次，方便知道去哪找日志。
const char* PlatformLogPath();

} // namespace gui_dev
