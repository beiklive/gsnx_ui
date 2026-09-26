# iOS 产物诊断（run 8，success）

- commit: 65184277c278731be1c4516e4132ce28ef09a422
- app: build/ios/Release-iphoneos/gui_dev_demo.app

## .app 目录（bundle 根）
total 4528
drwxr-xr-x  10 runner  staff      320 Sep 26 10:22 .
drwxr-xr-x@  7 runner  staff      224 Sep 26 10:22 ..
-rw-r--r--   1 runner  staff     1876 Sep 26 10:22 AppIcon60x60@2x.png
-rw-r--r--   1 runner  staff     2836 Sep 26 10:22 AppIcon60x60@3x.png
-rw-r--r--   1 runner  staff     2371 Sep 26 10:22 AppIcon76x76@2x~ipad.png
-rw-r--r--   1 runner  staff     2388 Sep 26 10:22 AppIcon83.5x83.5@2x~ipad.png
drwxr-xr-x   4 runner  staff      128 Sep 26 10:22 assets
-rwxr-xr-x   1 runner  staff  2292488 Sep 26 10:22 gui_dev_demo
-rw-r--r--   1 runner  staff     1284 Sep 26 10:22 Info.plist
-rw-r--r--   1 runner  staff        8 Sep 26 10:22 PkgInfo

## Info.plist
{
  "BuildMachineOSBuild" => "24G830"
  "CFBundleDevelopmentRegion" => "zh_CN"
  "CFBundleDisplayName" => "gui_dev_demo"
  "CFBundleExecutable" => "gui_dev_demo"
  "CFBundleIcons" => {
    "CFBundlePrimaryIcon" => {
      "CFBundleIconFiles" => [
        0 => "AppIcon60x60"
      ]
    }
  }
  "CFBundleIcons~ipad" => {
    "CFBundlePrimaryIcon" => {
      "CFBundleIconFiles" => [
        0 => "AppIcon60x60"
        1 => "AppIcon76x76"
        2 => "AppIcon83.5x83.5"
      ]
    }
  }
  "CFBundleIdentifier" => "com.beiklive.gui-dev.gui-dev-demo"
  "CFBundleInfoDictionaryVersion" => "6.0"
  "CFBundleName" => "gui_dev_demo"
  "CFBundlePackageType" => "APPL"
  "CFBundleShortVersionString" => "0.1.0"
  "CFBundleSupportedPlatforms" => [
    0 => "iPhoneOS"
  ]
  "CFBundleVersion" => "0.1.0"
  "DTCompiler" => "com.apple.compilers.llvm.clang.1_0"
  "DTPlatformBuild" => "22F76"
  "DTPlatformName" => "iphoneos"
  "DTPlatformVersion" => "18.5"
  "DTSDKBuild" => "22F76"
  "DTSDKName" => "iphoneos18.5"
  "DTXcode" => "1640"
  "DTXcodeBuild" => "16F6"
  "ITSAppUsesNonExemptEncryption" => 0
  "LSRequiresIPhoneOS" => 1
  "MinimumOSVersion" => "13.0"
  "UIDeviceFamily" => [
    0 => 1
    1 => 2
  ]
  "UILaunchScreen" => {
  }
  "UIRequiredDeviceCapabilities" => [
    0 => "arm64"
  ]
  "UIRequiresFullScreen" => 1
  "UIStatusBarHidden" => 1
  "UISupportedInterfaceOrientations" => [
    0 => "UIInterfaceOrientationLandscapeLeft"
    1 => "UIInterfaceOrientationLandscapeRight"
  ]
  "UIViewControllerBasedStatusBarAppearance" => 0
}

## 可执行文件
CFBundleExecutable=gui_dev_demo
-rwxr-xr-x  1 runner  staff  2292488 Sep 26 10:22 build/ios/Release-iphoneos/gui_dev_demo.app/gui_dev_demo
build/ios/Release-iphoneos/gui_dev_demo.app/gui_dev_demo: Mach-O 64-bit executable arm64
build/ios/Release-iphoneos/gui_dev_demo.app/gui_dev_demo:
Load command 10
      cmd LC_BUILD_VERSION
  cmdsize 32
 platform IOS
    minos 13.0
      sdk 18.5
   ntools 1
     tool LD
  version 1167.5

## 签名
build/ios/Release-iphoneos/gui_dev_demo.app: code object is not signed at all

## IPA 结构（前 25 项）
Archive:  dist/ios/gui_dev_demo.ipa
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  09-26-2026 10:22   Payload/
        0  09-26-2026 10:22   Payload/gui_dev_demo.app/
     1876  09-26-2026 10:22   Payload/gui_dev_demo.app/AppIcon60x60@2x.png
     2836  09-26-2026 10:22   Payload/gui_dev_demo.app/AppIcon60x60@3x.png
     2371  09-26-2026 10:22   Payload/gui_dev_demo.app/AppIcon76x76@2x~ipad.png
     2388  09-26-2026 10:22   Payload/gui_dev_demo.app/AppIcon83.5x83.5@2x~ipad.png
  2292488  09-26-2026 10:22   Payload/gui_dev_demo.app/gui_dev_demo
        0  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/
        0  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/img/
   258322  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/img/wide.jpg
    65973  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/img/alpha_test.png
   148691  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/img/photo.jpg
   805396  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/img/test.png
      191  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/img/border_gradient.png
   148691  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/img/photo.jpeg
  1390855  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/img/image.png
        0  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/font/
 10968356  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/font/switch_font.ttf
   180236  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/font/switch_icons.ttf
   356840  09-26-2026 10:22   Payload/gui_dev_demo.app/assets/font/MaterialIcons-Regular.ttf
     1284  09-26-2026 10:22   Payload/gui_dev_demo.app/Info.plist
        8  09-26-2026 10:22   Payload/gui_dev_demo.app/PkgInfo
