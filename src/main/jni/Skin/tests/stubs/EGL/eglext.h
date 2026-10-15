// Host stub - only what the header chain needs for a syntax check on desktop
// g++ (the real headers come from the NDK in the Android build).
#pragma once

#include "egl.h"

typedef void *EGLImage;
typedef void *EGLImageKHR;
typedef void *EGLClientBuffer;
typedef void *EGLSync;
typedef void *EGLSyncKHR;
typedef unsigned long long EGLTime;
typedef long long EGLint64;
typedef unsigned long long EGLuint64;

#define EGL_NO_IMAGE        ((EGLImageKHR)0)
#define EGL_NO_IMAGE_KHR    ((EGLImageKHR)0)
#define EGL_NO_SYNC         ((EGLSyncKHR)0)
#define EGL_NO_SYNC_KHR     ((EGLSyncKHR)0)
#define EGL_WINDOW_BIT      0x0004
#define EGL_PBUFFER_BIT     0x0001
#define EGL_OPENGL_ES2_BIT  0x0004
#define EGL_IMAGE_PRESERVED_KHR 0x30D2
#define EGL_FOREVER         0xFFFFFFFFFFFFFFFFull
#define EGL_FOREVER_KHR     0xFFFFFFFFFFFFFFFFull

extern "C" {
EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                              EGLClientBuffer buffer, const EGLint *attrib_list);
EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image);
EGLBoolean eglPresentationTimeANDROID(EGLDisplay dpy, EGLSurface surface, EGLint64 time);
}
