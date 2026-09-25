package com.beiklive.guidev;

import org.libsdl.app.SDLActivity;

/**
 * 入口 Activity：继承 SDL 的 SDLActivity，告诉它要加载哪些 native 库。
 *
 *   SDL2 —— SDL2 本体（fetch 模式下由 CMake 编出来）
 *   main —— 我们的 libmain.so（CMakeLists 里 GUI_DEV_PLATFORM=android 时建的那个 target）
 *
 * SDLActivity 会调用 libmain.so 里的 JNI 入口（SDL_android_main.c），
 * 后者再调到我们 demo 的 SDL_main。
 */
public class MainActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }
}
