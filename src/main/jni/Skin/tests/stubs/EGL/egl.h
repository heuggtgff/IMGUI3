// Host stub - only what the header chain needs for a syntax check on desktop
// g++ (the real headers come from the NDK in the Android build). Constant
// values are the real EGL ones; the entry points are declared normally, since
// nothing at host-check time can call them.
#pragma once

typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLContext;
typedef void *EGLConfig;
typedef void *EGLNativeDisplayType;
typedef void *EGLNativeWindowType;
typedef void *EGLNativePixmapType;
typedef unsigned int EGLBoolean;
typedef int EGLint;
typedef unsigned int EGLenum;
typedef void (*__eglMustCastToProperFunctionPointerType)(void);

#define EGL_FALSE 0
#define EGL_TRUE  1
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_CONTEXT  ((EGLContext)0)
#define EGL_NO_SURFACE  ((EGLSurface)0)
#define EGL_NO_CONFIG   ((EGLConfig)0)
#define EGL_DEFAULT_DISPLAY ((EGLNativeDisplayType)0)

#define EGL_NONE                0x3038
#define EGL_ALPHA_SIZE          0x3021
#define EGL_BLUE_SIZE           0x3022
#define EGL_GREEN_SIZE          0x3023
#define EGL_RED_SIZE            0x3024
#define EGL_DEPTH_SIZE          0x3025
#define EGL_STENCIL_SIZE        0x3026
#define EGL_SURFACE_TYPE        0x3033
#define EGL_RENDERABLE_TYPE     0x3040
#define EGL_NATIVE_VISUAL_ID    0x302E
#define EGL_CONFIG_ID           0x3028
#define EGL_WIDTH               0x3057
#define EGL_HEIGHT              0x3056

#define EGL_PBUFFER_BIT         0x0001
#define EGL_WINDOW_BIT          0x0004
#define EGL_OPENGL_ES_BIT       0x0001
#define EGL_OPENGL_ES2_BIT      0x0004
#define EGL_OPENGL_ES3_BIT_KHR  0x0040

#define EGL_DRAW                0x3059
#define EGL_READ                0x305A
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#define EGL_OPENGL_ES_API       0x30A0

#define EGL_SUCCESS             0x3000
#define EGL_NOT_INITIALIZED     0x3001
#define EGL_BAD_ACCESS          0x3002
#define EGL_BAD_CONFIG          0x3005
#define EGL_BAD_SURFACE         0x300D
#define EGL_BAD_CONTEXT         0x3006
#define EGL_CONTEXT_LOST        0x300E

#ifdef __cplusplus
extern "C" {
#endif

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id);
EGLDisplay eglGetCurrentDisplay(void);
EGLDisplay eglGetPlatformDisplay(EGLenum platform, void *native_display, const EGLint *attrib_list);
EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor);
EGLBoolean eglTerminate(EGLDisplay dpy);
const char *eglQueryString(EGLDisplay dpy, EGLint name);
EGLint     eglGetError(void);
EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs,
                           EGLint config_size, EGLint *num_config);
EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size,
                         EGLint *num_config);
EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute,
                              EGLint *value);
EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                                  EGLNativeWindowType win, const EGLint *attrib_list);
EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
                                   const EGLint *attrib_list);
EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context,
                            const EGLint *attrib_list);
EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx);
EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface);
EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface);
EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval);
EGLContext eglGetCurrentContext(void);
EGLSurface eglGetCurrentSurface(EGLint readdraw);
EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
EGLBoolean eglBindAPI(EGLenum api);
EGLBoolean eglWaitClient(void);
void      *eglGetProcAddress(const char *procname);

#ifdef __cplusplus
}
#endif
