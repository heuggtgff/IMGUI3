// Minimal stand-in for the NDK's <android/native_window_jni.h>. Host syntax
// checks only - see android/native_window.h next to this file.
#pragma once

#include <jni.h>
#include "native_window.h"

#ifdef __cplusplus
extern "C" {
#endif

ANativeWindow *ANativeWindow_fromSurface(JNIEnv *env, jobject surface);
void ANativeWindow_acquire(ANativeWindow *window);
void ANativeWindow_release(ANativeWindow *window);

#ifdef __cplusplus
}
#endif
