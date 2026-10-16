#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "ImGui/Call_ImGui.h"
#include "ImGui/menu_layout.h"
#include "IL2CppSDKGenerator/BasicStructs/Call_BasicStructs.h"
#include "IL2CppSDKGenerator/IL2Cpp/Call_IL2Cpp.h"
#include "Hacks/Hacks.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "AstralPrtctn/AntiHook.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Includes/Jni_Stuff.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string>
#include <functional>
#include <cstring>
#include <cfloat>
#include <jni.h>
#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "oxorany/source/oxorany.h"
#include "oxorany/source/oxorany.cpp"
#include "oxorany/source/oxorany_include.h"

#include "FeedbackService.h"
// NewFeatures must be visible before MainFeatureIncludes.h: the existing
// Feats.h firing-timing hooks (Firerate / No Sprint Fire Delay / No Fire
// Delay) call into NewFeatures::NoFireDelayActive() /
// NoSprintFireDelayActive() so that each dump address is hooked exactly once.
#include "Hacks/Feature/NewFeatures.h"
#include "Hacks/Feature/Crosshair.h"
#include "MainFeatureIncludes.h"

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>
#include <sys/system_properties.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>

static auto lastScreenshotTime = std::chrono::steady_clock::now();
constexpr std::chrono::seconds screenshotInterval(680);

namespace android::anwcreator::detail::types
{
    enum class SurfaceControlFlags : uint32_t
    {
        eHidden          = 0x00000004,
        eSkipScreenshot  = 0x00000040,
        eSecure          = 0x00000080,
        eNonPremultiplied= 0x00000100,
        eOpaque          = 0x00000400,
        eNoColorFill     = 0x00004000,
    };
    enum class DisplayRotation : int32_t
    { Rotation0 = 0, Rotation90 = 1, Rotation180 = 2, Rotation270 = 3 };

    template <typename enum_t, typename = std::enable_if_t<std::is_enum_v<enum_t>>>
    constexpr enum_t operator|(enum_t lhs, enum_t rhs) {
        using U = std::underlying_type_t<enum_t>;
        return static_cast<enum_t>(static_cast<U>(lhs) | static_cast<U>(rhs));
    }
    template <typename enum_t, typename = std::enable_if_t<std::is_enum_v<enum_t>>>
    constexpr enum_t operator|=(enum_t &lhs, enum_t rhs) { return (lhs = lhs | rhs); }
}

namespace android::anwcreator::detail::jni
{
    struct JNIEnvironment {
        JNIEnv *env; JavaVM *vm;
        JNIEnvironment(JavaVM *javaVM) : env(nullptr), vm(javaVM) { if (vm) vm->AttachCurrentThread(&env, nullptr); }
        ~JNIEnvironment() {}
        inline bool IsValid() const { return env != nullptr; }
        inline operator JNIEnv *() const { return env; }
        inline JNIEnv *operator->() const { return env; }
        inline bool CheckException(const char * = nullptr) {
            if (!env || !env->ExceptionCheck()) return false;
            env->ExceptionDescribe(); env->ExceptionClear(); return true;
        }
    };
    struct LocalRef {
        JNIEnv *env; jobject obj;
        LocalRef(JNIEnv *e, jobject o) : env(e), obj(o) {}
        ~LocalRef() { if (env && obj) env->DeleteLocalRef(obj); }
        LocalRef(const LocalRef &) = delete;
        LocalRef &operator=(const LocalRef &) = delete;
        LocalRef(LocalRef &&o) noexcept : env(o.env), obj(o.obj) { o.obj = nullptr; }
        inline operator jobject() const { return obj; }
        inline operator jclass() const { return reinterpret_cast<jclass>(obj); }
        inline jobject get() const { return obj; }
        inline jclass getClass() const { return reinterpret_cast<jclass>(obj); }
        inline bool IsValid() const { return obj != nullptr; }
        inline explicit operator bool() const { return obj != nullptr; }
    };
    struct GlobalRef {
        JNIEnv *env; jobject obj;
        GlobalRef(JNIEnv *e, jobject o) : env(e), obj(nullptr) { if (e && o) obj = e->NewGlobalRef(o); }
        ~GlobalRef() { Release(); }
        void Release() { if (env && obj) { env->DeleteGlobalRef(obj); obj = nullptr; } }
        GlobalRef(const GlobalRef &) = delete;
        GlobalRef &operator=(const GlobalRef &) = delete;
        GlobalRef(GlobalRef &&o) noexcept : env(o.env), obj(o.obj) { o.obj = nullptr; }
        inline operator jobject() const { return obj; }
        inline jobject get() const { return obj; }
        inline bool IsValid() const { return obj != nullptr; }
        inline explicit operator bool() const { return obj != nullptr; }
    };
}

namespace android::anwcreator::detail::framework
{
    struct DisplayMetrics { int32_t widthPixels, heightPixels; float density; int32_t densityDpi; };
    struct DisplayInfo { int32_t width, height; types::DisplayRotation rotation; float refreshRate; };

    class Display {
    public:
        static DisplayInfo GetDisplayInfo(JNIEnv *env, ANativeActivity *activity) {
            DisplayInfo info{};
            if (!env || !activity || !activity->clazz) return info;
            jni::LocalRef activityCls(env, env->GetObjectClass(activity->clazz));
            if (!activityCls) return info;
            jmethodID getWindowManager = env->GetMethodID(activityCls, "getWindowManager", "()Landroid/view/WindowManager;");
            if (!getWindowManager) return info;
            jni::LocalRef windowManager(env, env->CallObjectMethod(activity->clazz, getWindowManager));
            if (!windowManager || jni::JNIEnvironment(activity->vm).CheckException()) return info;
            jni::LocalRef windowManagerCls(env, env->GetObjectClass(windowManager));
            jmethodID getDefaultDisplay = env->GetMethodID(windowManagerCls, "getDefaultDisplay", "()Landroid/view/Display;");
            if (!getDefaultDisplay) return info;
            jni::LocalRef display(env, env->CallObjectMethod(windowManager, getDefaultDisplay));
            if (!display || jni::JNIEnvironment(activity->vm).CheckException()) return info;
            jni::LocalRef displayMetricsCls(env, env->FindClass("android/util/DisplayMetrics"));
            jmethodID ctor = env->GetMethodID(displayMetricsCls, "<init>", "()V");
            jni::LocalRef displayMetrics(env, env->NewObject(displayMetricsCls, ctor));
            jni::LocalRef displayCls(env, env->GetObjectClass(display));
            jmethodID getRealMetrics = env->GetMethodID(displayCls, "getRealMetrics", "(Landroid/util/DisplayMetrics;)V");
            env->CallVoidMethod(display, getRealMetrics, displayMetrics.get());
            jfieldID widthPixelsField  = env->GetFieldID(displayMetricsCls, "widthPixels", "I");
            jfieldID heightPixelsField = env->GetFieldID(displayMetricsCls, "heightPixels", "I");
            info.width  = env->GetIntField(displayMetrics, widthPixelsField);
            info.height = env->GetIntField(displayMetrics, heightPixelsField);
            jmethodID getRotation = env->GetMethodID(displayCls, "getRotation", "()I");
            info.rotation = static_cast<types::DisplayRotation>(env->CallIntMethod(display, getRotation));
            jmethodID getRefreshRate = env->GetMethodID(displayCls, "getRefreshRate", "()F");
            if (getRefreshRate) info.refreshRate = env->CallFloatMethod(display, getRefreshRate);
            return info;
        }
    };

    class SurfaceControl {
    public:
        struct Builder {
            JNIEnv *env; jobject builder; jclass builderClass;
            Builder(JNIEnv *e) : env(e), builder(nullptr), builderClass(nullptr) {
                builderClass = env->FindClass("android/view/SurfaceControl$Builder");
                if (!builderClass) return;
                jmethodID ctor = env->GetMethodID(builderClass, "<init>", "()V");
                builder = env->NewObject(builderClass, ctor);
            }
            ~Builder() {
                if (env && builder) env->DeleteLocalRef(builder);
                if (env && builderClass) env->DeleteLocalRef(builderClass);
            }
            Builder &SetName(const char *name) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setName", "(Ljava/lang/String;)Landroid/view/SurfaceControl$Builder;");
                if (m) { jni::LocalRef jName(env, env->NewStringUTF(name)); builder = env->CallObjectMethod(builder, m, jName.get()); }
                return *this;
            }
            Builder &SetParent(jobject parent) {
                if (!builder || !parent) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setParent", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, parent);
                return *this;
            }
            Builder &SetBufferSize(int32_t w, int32_t h) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setBufferSize", "(II)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, w, h);
                return *this;
            }
            Builder &SetFormat(int32_t f) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setFormat", "(I)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, (jint)f);
                return *this;
            }
            Builder &SetOpaque(bool o) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setOpaque", "(Z)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, (jboolean)o);
                return *this;
            }
            Builder &SetFlags(uint32_t f, uint32_t mask) {
                if (!builder) return *this;
                jmethodID m = env->GetMethodID(builderClass, "setFlags", "(II)Landroid/view/SurfaceControl$Builder;");
                if (m) builder = env->CallObjectMethod(builder, m, (jint)f, (jint)mask);
                return *this;
            }
            // SurfaceControl.SKIP_SCREENSHOT (0x40, AOSP API 31+) makes the
            // layer and its children invisible to screenshots, mirroring and
            // screen recording. It is only reachable through the builder's
            // private setFlags(int, int) helper (see AOSP SurfaceControl.java),
            // which JNI can call. Returns true only when the call really
            // reached the framework, so a caller can refuse to build a
            // "hidden" overlay that is not hidden at all.
            bool SetSkipScreenshot(bool skip) {
                if (!skip || !builder || !builderClass) return false;
                constexpr int32_t FLAG_SKIP_SCREENSHOT = 0x40;
                jmethodID m = env->GetMethodID(builderClass, "setFlags",
                    "(II)Landroid/view/SurfaceControl$Builder;");
                if (!m) { if (env->ExceptionCheck()) env->ExceptionClear(); return false; }
                jobject r = env->CallObjectMethod(builder, m,
                    (jint)FLAG_SKIP_SCREENSHOT, (jint)FLAG_SKIP_SCREENSHOT);
                if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }
                if (r) builder = r;
                return true;
            }
            jobject Build() {
                if (!builder) return nullptr;
                jmethodID m = env->GetMethodID(builderClass, "build", "()Landroid/view/SurfaceControl;");
                if (!m) return nullptr;
                return env->CallObjectMethod(builder, m);
            }
            inline bool IsValid() const { return builder != nullptr; }
        };

        class Transaction {
        public:
            JNIEnv *env; jobject transaction; jclass transactionClass;
            Transaction(JNIEnv *e) : env(e), transaction(nullptr), transactionClass(nullptr) {
                transactionClass = env->FindClass("android/view/SurfaceControl$Transaction");
                if (!transactionClass) return;
                jmethodID ctor = env->GetMethodID(transactionClass, "<init>", "()V");
                transaction = env->NewObject(transactionClass, ctor);
            }
            ~Transaction() {
                if (env && transaction) env->DeleteLocalRef(transaction);
                if (env && transactionClass) env->DeleteLocalRef(transactionClass);
            }
            Transaction &SetAlpha(jobject sc, float a) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setAlpha", "(Landroid/view/SurfaceControl;F)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc, a);
                return *this;
            }
            Transaction &SetLayer(jobject sc, int32_t z) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setLayer", "(Landroid/view/SurfaceControl;I)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc, z);
                return *this;
            }
            Transaction &SetPosition(jobject sc, float x, float y) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setPosition", "(Landroid/view/SurfaceControl;FF)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc, x, y);
                return *this;
            }
            Transaction &SetMatrix(jobject sc, float a, float b, float c, float d) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setMatrix", "(Landroid/view/SurfaceControl;FFFF)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc, a, b, c, d);
                return *this;
            }
            Transaction &Show(jobject sc) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "show", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc);
                return *this;
            }
            Transaction &Hide(jobject sc) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "hide", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc);
                return *this;
            }
            Transaction &SetParent(jobject c, jobject p) {
                if (!transaction || !c || !p) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "setParent", "(Landroid/view/SurfaceControl;Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, c, p);
                return *this;
            }
            // Public @hide SurfaceControl.Transaction#setSkipScreenshot
            // (API 31+): the framework's own way of flagging an existing layer
            // as screenshot / screen-recording invisible. Returns true when the
            // method exists on this Android version.
            bool SetSkipScreenshot(jobject sc, bool skip) {
                if (!transaction || !sc) return false;
                jmethodID m = env->GetMethodID(transactionClass, "setSkipScreenshot",
                    "(Landroid/view/SurfaceControl;Z)Landroid/view/SurfaceControl$Transaction;");
                if (!m) { if (env->ExceptionCheck()) env->ExceptionClear(); return false; }
                jobject r = env->CallObjectMethod(transaction, m, sc, (jboolean)skip);
                if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }
                if (r) transaction = r;
                return true;
            }
            Transaction &Remove(jobject sc) {
                if (!transaction || !sc) return *this;
                jmethodID m = env->GetMethodID(transactionClass, "remove", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction, m, sc);
                return *this;
            }
            void Apply() {
                if (!transaction) return;
                jmethodID m = env->GetMethodID(transactionClass, "apply", "()V");
                if (m) env->CallVoidMethod(transaction, m);
            }
            inline bool IsValid() const { return transaction != nullptr; }
        };

        static jobject GetParentSurfaceControl(JNIEnv *env, ANativeActivity *activity) {
            if (!env || !activity || !activity->clazz) return nullptr;
            jni::LocalRef activityCls(env, env->GetObjectClass(activity->clazz));
            jmethodID getWindow = env->GetMethodID(activityCls, "getWindow", "()Landroid/view/Window;");
            if (!getWindow) return nullptr;
            jni::LocalRef window(env, env->CallObjectMethod(activity->clazz, getWindow));
            if (!window) return nullptr;
            jni::LocalRef windowCls(env, env->GetObjectClass(window));
            jmethodID getDecorView = env->GetMethodID(windowCls, "getDecorView", "()Landroid/view/View;");
            if (!getDecorView) return nullptr;
            jni::LocalRef decorView(env, env->CallObjectMethod(window, getDecorView));
            if (!decorView) return nullptr;
            jni::LocalRef viewCls(env, env->GetObjectClass(decorView));
            jmethodID getViewRootImpl = env->GetMethodID(viewCls, "getViewRootImpl", "()Landroid/view/ViewRootImpl;");
            if (!getViewRootImpl) return nullptr;
            jni::LocalRef viewRootImpl(env, env->CallObjectMethod(decorView, getViewRootImpl));
            if (!viewRootImpl) return nullptr;
            jni::LocalRef vriCls(env, env->GetObjectClass(viewRootImpl));
            jmethodID getSC = env->GetMethodID(vriCls, "getSurfaceControl", "()Landroid/view/SurfaceControl;");
            if (!getSC) return nullptr;
            return env->CallObjectMethod(viewRootImpl, getSC);
        }
    };

    class Surface {
    public:
        static jobject CreateFromSurfaceControl(JNIEnv *env, jobject sc) {
            if (!env || !sc) return nullptr;
            jni::LocalRef cls(env, env->FindClass("android/view/Surface"));
            if (!cls) return nullptr;
            jmethodID ctor = env->GetMethodID(cls, "<init>", "(Landroid/view/SurfaceControl;)V");
            if (!ctor) return nullptr;
            return env->NewObject(cls, ctor, sc);
        }
    };
}

namespace android::anwcreator::detail
{
    struct WindowContext {
        std::unique_ptr<jni::GlobalRef> surfaceControl, surface, parentSurfaceControl;
        ANativeWindow *nativeWindow; int32_t width, height;
        bool skipScreenshot, skipScreenshotApplied;
        WindowContext() : surfaceControl(nullptr), surface(nullptr), parentSurfaceControl(nullptr),
                          nativeWindow(nullptr), width(0), height(0), skipScreenshot(false),
                          skipScreenshotApplied(false) {}
        ~WindowContext() { Release(); }
        void Release() {
            if (nativeWindow) { ANativeWindow_release(nativeWindow); nativeWindow = nullptr; }
            surface.reset(); surfaceControl.reset(); parentSurfaceControl.reset();
        }
    };
}

namespace android
{
    class ANwCreator {
    public:
        struct DisplayInfo { int32_t theta, width, height; float refreshRate;
            DisplayInfo() : theta(0), width(0), height(0), refreshRate(60.0f) {} };
        struct CreateOptions { const char *name; int32_t width, height; bool skipScreenshot;
            CreateOptions() : name(""), width(-1), height(-1), skipScreenshot(false) {} };

        static DisplayInfo GetDisplayInfo(ANativeActivity *a) {
            DisplayInfo r{};
            if (!a || !a->vm || !a->clazz) return r;
            anwcreator::detail::jni::JNIEnvironment je(a->vm);
            if (!je.IsValid()) return r;
            auto d = anwcreator::detail::framework::Display::GetDisplayInfo(je, a);
            r.width = d.width; r.height = d.height;
            r.theta = 90 * static_cast<int32_t>(d.rotation);
            r.refreshRate = d.refreshRate;
            return r;
        }

        static ANativeWindow *Create(ANativeActivity *a, const CreateOptions &o = CreateOptions()) {
            if (!a || !a->vm || !a->clazz) return nullptr;
            anwcreator::detail::jni::JNIEnvironment je(a->vm);
            if (!je.IsValid()) return nullptr;
            int32_t w = o.width, h = o.height;
            if (w <= 0 || h <= 0) {
                auto d = anwcreator::detail::framework::Display::GetDisplayInfo(je, a);
                w = d.width; h = d.height;
            }
            jobject parentSC = anwcreator::detail::framework::SurfaceControl::GetParentSurfaceControl(je, a);
            if (!parentSC && je.CheckException()) return nullptr;
            anwcreator::detail::framework::SurfaceControl::Builder b(je);
            if (!b.IsValid()) return nullptr;
            // Format 1 == WINDOW_FORMAT_RGBA_8888 == it can carry per-pixel
            // alpha, which the overlay needs to stay see-through.
            b.SetName(o.name).SetBufferSize(w, h).SetFormat(1).SetOpaque(false);
            const bool skipViaBuilder = b.SetSkipScreenshot(o.skipScreenshot);
            if (parentSC) b.SetParent(parentSC);
            jobject lsc = b.Build();
            if (!lsc || je.CheckException()) { if (parentSC) je->DeleteLocalRef(parentSC); return nullptr; }
            auto ctx = std::make_unique<anwcreator::detail::WindowContext>();
            ctx->width = w; ctx->height = h; ctx->skipScreenshot = o.skipScreenshot;
            bool skipViaTransaction = false;
            ctx->surfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(je, lsc);
            if (parentSC) ctx->parentSurfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(je, parentSC);
            {
                anwcreator::detail::framework::SurfaceControl::Transaction t(je);
                if (t.IsValid() && o.skipScreenshot)
                    skipViaTransaction = t.SetSkipScreenshot(ctx->surfaceControl->get(), true);
                if (t.IsValid())
                    t.SetAlpha(ctx->surfaceControl->get(), 1.0f)
                     .SetLayer(ctx->surfaceControl->get(), 0x7FFFFFFE)
                     .SetPosition(ctx->surfaceControl->get(), 0.0f, 0.0f)
                     .SetMatrix(ctx->surfaceControl->get(), 1.0f, 0.0f, 0.0f, 1.0f)
                     .Show(ctx->surfaceControl->get()).Apply();
            }
            jobject ls = anwcreator::detail::framework::Surface::CreateFromSurfaceControl(je, ctx->surfaceControl->get());
            if (!ls || je.CheckException()) {
                je->DeleteLocalRef(lsc); if (parentSC) je->DeleteLocalRef(parentSC);
                return nullptr;
            }
            ctx->surface = std::make_unique<anwcreator::detail::jni::GlobalRef>(je, ls);
            ctx->nativeWindow = ANativeWindow_fromSurface(je, ctx->surface->get());
            if (!ctx->nativeWindow) {
                je->DeleteLocalRef(ls); je->DeleteLocalRef(lsc); if (parentSC) je->DeleteLocalRef(parentSC);
                return nullptr;
            }
            ctx->skipScreenshotApplied = o.skipScreenshot && (skipViaBuilder || skipViaTransaction);
            ANativeWindow *result = ctx->nativeWindow;
            m_windowContexts.emplace(result, std::move(ctx));
            je->DeleteLocalRef(ls); je->DeleteLocalRef(lsc); if (parentSC) je->DeleteLocalRef(parentSC);
            return result;
        }

        static void Destroy(ANativeActivity *a, ANativeWindow *nw) {
            if (!nw) return;
            auto it = m_windowContexts.find(nw);
            if (it == m_windowContexts.end()) { ANativeWindow_release(nw); return; }
            auto &ctx = it->second;
            if (a && a->vm && ctx->surfaceControl && ctx->surfaceControl->IsValid()) {
                anwcreator::detail::jni::JNIEnvironment je(a->vm);
                if (je.IsValid()) {
                    anwcreator::detail::framework::SurfaceControl::Transaction t(je);
                    if (t.IsValid()) t.Remove(ctx->surfaceControl->get()).Apply();
                }
            }
            ctx->Release();
            m_windowContexts.erase(it);
        }

        static bool IsValid(ANativeWindow *nw) { return nw && m_windowContexts.count(nw) > 0; }

        static bool GetWindowSize(ANativeWindow *nw, int32_t *w, int32_t *h) {
            auto it = m_windowContexts.find(nw);
            if (it == m_windowContexts.end()) return false;
            if (w) *w = it->second->width;
            if (h) *h = it->second->height;
            return true;
        }

        // True only when the layer really carries SurfaceControl.SKIP_SCREENSHOT.
        static bool SkipScreenshotApplied(ANativeWindow *nw) {
            auto it = m_windowContexts.find(nw);
            return it != m_windowContexts.end() && it->second->skipScreenshotApplied;
        }

        static void EnsureVisible(ANativeActivity *a, ANativeWindow *nw) {
            if (!a || !a->vm || !nw) return;
            auto it = m_windowContexts.find(nw);
            if (it == m_windowContexts.end()) return;
            auto &ctx = it->second;
            if (!ctx || !ctx->surfaceControl || !ctx->surfaceControl->IsValid()) return;
            anwcreator::detail::jni::JNIEnvironment je(a->vm);
            if (!je.IsValid()) return;
            anwcreator::detail::framework::SurfaceControl::Transaction t(je);
            if (!t.IsValid()) return;
            jobject cp = anwcreator::detail::framework::SurfaceControl::GetParentSurfaceControl(je, a);
            bool needReparent = false;
            if (cp && ctx->parentSurfaceControl && ctx->parentSurfaceControl->IsValid()) {
                if (!je->IsSameObject(cp, ctx->parentSurfaceControl->get())) needReparent = true;
            } else if (cp && !ctx->parentSurfaceControl) needReparent = true;
            if (needReparent && cp) {
                t.SetParent(ctx->surfaceControl->get(), cp);
                ctx->parentSurfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(je, cp);
            }
            float sx = 1.0f, sy = 1.0f;
            auto d = anwcreator::detail::framework::Display::GetDisplayInfo(je, a);
            if (ctx->width > 0 && ctx->height > 0 && d.width > 0 && d.height > 0) {
                int32_t tw = d.width, th = d.height;
                const bool bL = ctx->width >= ctx->height, tL = tw >= th;
                if (bL != tL) std::swap(tw, th);
                sx = (float)tw / (float)ctx->width;
                sy = (float)th / (float)ctx->height;
                if (!(sx > 0.0f) || !(sy > 0.0f) || sx > 4.0f || sy > 4.0f) sx = sy = 1.0f;
            }
            t.SetAlpha(ctx->surfaceControl->get(), 1.0f)
             .SetLayer(ctx->surfaceControl->get(), 0x7FFFFFFE)
             .SetPosition(ctx->surfaceControl->get(), 0.0f, 0.0f)
             .SetMatrix(ctx->surfaceControl->get(), sx, 0.0f, 0.0f, sy)
             .Show(ctx->surfaceControl->get()).Apply();
            if (cp) je->DeleteLocalRef(cp);
        }

    private:
        inline static std::unordered_map<ANativeWindow *, std::unique_ptr<anwcreator::detail::WindowContext>> m_windowContexts;
    };
}

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <android/native_activity.h>

namespace android {
class HideRecAImGui {
public:
    struct Options { ANativeActivity* activity=nullptr; bool skipScreenshot=true; };
    using SwapBuffersFn=EGLBoolean (*)(EGLDisplay,EGLSurface);
    HideRecAImGui():HideRecAImGui(Options{}){}
    explicit HideRecAImGui(const Options& options);
    ~HideRecAImGui();
    bool RenderDrawData(ImDrawData*,EGLDisplay,EGLSurface,EGLContext,SwapBuffersFn);
    void Shutdown();
    constexpr operator bool() const{return m_state;}
private:
    bool EnsureEnvironment(EGLDisplay,EGLContext,int32_t,int32_t);
    bool InitEnvironment(EGLDisplay,EGLContext,int32_t,int32_t);
    void UnInitEnvironment();
    EGLConfig FindConfig(EGLDisplay,EGLContext);
    bool m_state=false;
    int32_t m_screenWidth=-1,m_screenHeight=-1;
    Options m_options{};
    ANativeWindow* m_nativeWindow=nullptr;
    EGLDisplay m_display=EGL_NO_DISPLAY;
    EGLSurface m_surface=EGL_NO_SURFACE;
    EGLContext m_overlayContext=EGL_NO_CONTEXT;
    EGLContext m_gameContext=EGL_NO_CONTEXT;
};
}

#include <EGL/eglext.h>
#include <cmath>

namespace android {
HideRecAImGui::HideRecAImGui(const Options& o):m_options(o){}
HideRecAImGui::~HideRecAImGui(){UnInitEnvironment();}

// WINDOW_FORMAT_RGBA_8888 == AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM == 1;
// no other Android window format carries an alpha channel.
static bool HideRecConfigIsTranslucent(EGLDisplay d, EGLConfig cfg) {
    if (d == EGL_NO_DISPLAY || !cfg) return false;
    EGLint alpha = 0, visual = 0;
    if (eglGetConfigAttrib(d, cfg, EGL_ALPHA_SIZE, &alpha) != EGL_TRUE || alpha < 8) return false;
    if (eglGetConfigAttrib(d, cfg, EGL_NATIVE_VISUAL_ID, &visual) != EGL_TRUE) return false;
    return visual == 1;
}

EGLConfig HideRecAImGui::FindConfig(EGLDisplay d, EGLContext gc) {
    (void)gc;
    // The overlay is composited ON TOP of the game, so every pixel ImGui does
    // not draw stays (0,0,0,0) and has to blend through to the game. A config
    // without an alpha channel turns those pixels into opaque black and the
    // overlay blacks the whole display out - that is the "Hide Stream black
    // screen". Do NOT inherit the game's EGLConfig here: the game draws to an
    // opaque window, so its config usually has EGL_ALPHA_SIZE == 0.
    if (d == EGL_NO_DISPLAY) return nullptr;
    const EGLint attrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, 0x00000040,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig cfg = nullptr; EGLint count = 0;
    if (eglChooseConfig(d, attrs, &cfg, 1, &count) == EGL_TRUE && count > 0 && cfg &&
        HideRecConfigIsTranslucent(d, cfg)) return cfg;
    EGLint total = 0;
    if (eglGetConfigs(d, nullptr, 0, &total) != EGL_TRUE || total <= 0) return nullptr;
    std::vector<EGLConfig> cfgs((size_t)total);
    if (eglGetConfigs(d, cfgs.data(), total, &total) != EGL_TRUE) return nullptr;
    for (int i = 0; i < total; i++) {
        EGLint st = 0, rt = 0;
        eglGetConfigAttrib(d, cfgs[i], EGL_SURFACE_TYPE, &st);
        if (!(st & EGL_WINDOW_BIT)) continue;
        eglGetConfigAttrib(d, cfgs[i], EGL_RENDERABLE_TYPE, &rt);
        if (!(rt & 0x00000040)) continue;
        if (HideRecConfigIsTranslucent(d, cfgs[i])) return cfgs[i];
    }
    return nullptr;
}

bool HideRecAImGui::InitEnvironment(EGLDisplay d, EGLContext gc, int32_t w, int32_t h) {
    if (!m_options.activity || d == EGL_NO_DISPLAY || gc == EGL_NO_CONTEXT || w <= 0 || h <= 0) return false;
    ANwCreator::CreateOptions o;
    o.name = "HideRecSecure"; o.width = w; o.height = h; o.skipScreenshot = m_options.skipScreenshot;
    m_nativeWindow = ANwCreator::Create(m_options.activity, o);
    if (!m_nativeWindow) return false;
    // SKIP_SCREENSHOT needs Android 12 (API 31) or newer. Without it the
    // overlay hides nothing and would only sit above the game, so bail out and
    // let the caller draw the menu normally instead.
    if (m_options.skipScreenshot && !ANwCreator::SkipScreenshotApplied(m_nativeWindow)) {
        LOGW("Hide Stream: SKIP_SCREENSHOT unavailable (needs Android 12+); drawing the menu normally");
        UnInitEnvironment();
        return false;
    }
    m_display = d; m_gameContext = gc;
    EGLConfig cfg = FindConfig(m_display, gc);
    if (!cfg) {
        LOGW("Hide Stream: no translucent GLES3 EGL config; drawing the menu normally");
        UnInitEnvironment();
        return false;
    }
    EGLint fmt = 0;
    if (eglGetConfigAttrib(m_display, cfg, EGL_NATIVE_VISUAL_ID, &fmt) != EGL_TRUE) { UnInitEnvironment(); return false; }
    ANativeWindow_setBuffersGeometry(m_nativeWindow, w, h, fmt);
    // Last line of defence against the black screen: never present a window
    // that cannot carry alpha, whatever the driver claims about the config.
    if (fmt != 1 || ANativeWindow_getFormat(m_nativeWindow) != 1) {
        LOGW("Hide Stream: overlay window format %d/%d is not RGBA_8888; drawing the menu normally",
             (int)fmt, (int)ANativeWindow_getFormat(m_nativeWindow));
        UnInitEnvironment();
        return false;
    }
    m_surface = eglCreateWindowSurface(m_display, cfg, m_nativeWindow, nullptr);
    if (m_surface == EGL_NO_SURFACE) { UnInitEnvironment(); return false; }
    // The overlay context MUST live in the game context's share group: ImGui's
    // shader program, font texture and vertex/index buffers were created on the
    // game context, and GL object handles are not portable between unshared
    // contexts. Rendering there without sharing draws nothing at all (the
    // silently empty overlay), so a failed share is a hard failure.
    const EGLint ctxAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    m_overlayContext = eglCreateContext(m_display, cfg, gc, ctxAttrs);
    if (m_overlayContext == EGL_NO_CONTEXT) {
        LOGW("Hide Stream: could not share the EGL context with the game; drawing the menu normally");
        UnInitEnvironment();
        return false;
    }
    EGLint rw = 0, rh = 0;
    if (eglQuerySurface(m_display, m_surface, EGL_WIDTH, &rw) == EGL_TRUE && rw > 0) m_screenWidth = rw; else m_screenWidth = w;
    if (eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &rh) == EGL_TRUE && rh > 0) m_screenHeight = rh; else m_screenHeight = h;
    m_state = true;
    ANwCreator::EnsureVisible(m_options.activity, m_nativeWindow);
    return true;
}

bool HideRecAImGui::EnsureEnvironment(EGLDisplay d, EGLContext gc, int32_t w, int32_t h) {
    if (d == EGL_NO_DISPLAY || gc == EGL_NO_CONTEXT || w <= 0 || h <= 0) return false;
    if (m_state && (m_display != d || m_gameContext != gc || m_screenWidth != w || m_screenHeight != h)) UnInitEnvironment();
    if (!m_state && !InitEnvironment(d, gc, w, h)) return false;
    return true;
}

bool HideRecAImGui::RenderDrawData(ImDrawData* dd, EGLDisplay gd, EGLSurface gs, EGLContext gc, SwapBuffersFn sb) {
    if (!dd || !sb || gs == EGL_NO_SURFACE) return false;
    int32_t ww = (int32_t)std::lround(dd->DisplaySize.x);
    int32_t wh = (int32_t)std::lround(dd->DisplaySize.y);
    if (ww <= 0 || wh <= 0) return false;
    if (!EnsureEnvironment(gd, gc, ww, wh)) return false;
    EGLDisplay od = eglGetCurrentDisplay();
    EGLSurface odr = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface ore = eglGetCurrentSurface(EGL_READ);
    EGLContext oc = eglGetCurrentContext();
    if (m_overlayContext == EGL_NO_CONTEXT || eglMakeCurrent(m_display, m_surface, m_surface, m_overlayContext) != EGL_TRUE) return false;
    EGLint vw = 0, vh = 0;
    if (eglQuerySurface(m_display, m_surface, EGL_WIDTH, &vw) != EGL_TRUE || vw <= 0) vw = m_screenWidth;
    if (eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &vh) != EGL_TRUE || vh <= 0) vh = m_screenHeight;
    glViewport(0, 0, vw, vh);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(dd);
    // Diagnostic: an error here means the overlay context cannot use ImGui's
    // GL objects (share group refused by the driver, ...). Report it once
    // instead of silently presenting an empty overlay.
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        static bool loggedOnce = false;
        if (!loggedOnce) { loggedOnce = true; LOGW("Hide Stream: overlay draw error 0x%X", (unsigned)glErr); }
    }
    EGLBoolean ok = sb(m_display, m_surface);
    if (od != EGL_NO_DISPLAY && oc != EGL_NO_CONTEXT) eglMakeCurrent(od, odr, ore, oc);
    else eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return ok == EGL_TRUE;
}

void HideRecAImGui::Shutdown() { UnInitEnvironment(); }
void HideRecAImGui::UnInitEnvironment() {
    m_state = false;
    if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE) {
        if (eglGetCurrentSurface(EGL_DRAW) == m_surface)
            eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(m_display, m_surface);
    }
    if (m_display != EGL_NO_DISPLAY && m_overlayContext != EGL_NO_CONTEXT)
        eglDestroyContext(m_display, m_overlayContext);
    m_surface = EGL_NO_SURFACE; m_overlayContext = EGL_NO_CONTEXT;
    if (m_nativeWindow) { ANwCreator::Destroy(m_options.activity, m_nativeWindow); m_nativeWindow = nullptr; }
    m_display = EGL_NO_DISPLAY; m_gameContext = EGL_NO_CONTEXT;
    m_screenWidth = m_screenHeight = -1;
}
}

#include <jni.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <cstring>

static ANativeActivity g_nativeActivity{};
static bool g_initialized = false;
static JavaVM* g_vm = nullptr;
extern "C" void SetANativeActivityVM(JavaVM* vm) { g_vm = vm; }

static jobject GetCurrentActivity(JNIEnv* env) {
    if (!env) return nullptr;
    jclass atc = env->FindClass("android/app/ActivityThread");
    if (!atc) { env->ExceptionClear(); return nullptr; }
    jmethodID cat = env->GetStaticMethodID(atc, "currentActivityThread", "()Landroid/app/ActivityThread;");
    if (!cat) { env->ExceptionClear(); env->DeleteLocalRef(atc); return nullptr; }
    jobject at = env->CallStaticObjectMethod(atc, cat);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!at) { env->DeleteLocalRef(atc); return nullptr; }
    jfieldID maf = env->GetFieldID(atc, "mActivities", "Landroid/util/ArrayMap;");
    if (!maf) { env->ExceptionClear(); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jobject ma = env->GetObjectField(at, maf);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!ma) { env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jclass amc = env->GetObjectClass(ma);
    jmethodID vm = env->GetMethodID(amc, "values", "()Ljava/util/Collection;");
    if (!vm) { env->ExceptionClear(); env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jobject vals = env->CallObjectMethod(ma, vm);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!vals) { env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jclass cc = env->GetObjectClass(vals);
    jmethodID im = env->GetMethodID(cc, "iterator", "()Ljava/util/Iterator;");
    if (!im) { env->ExceptionClear(); env->DeleteLocalRef(cc); env->DeleteLocalRef(vals); env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jobject it = env->CallObjectMethod(vals, im);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!it) { env->DeleteLocalRef(cc); env->DeleteLocalRef(vals); env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jclass ic = env->GetObjectClass(it);
    jmethodID hn = env->GetMethodID(ic, "hasNext", "()Z");
    jmethodID nx = env->GetMethodID(ic, "next", "()Ljava/lang/Object;");
    if (!hn || !nx) { env->ExceptionClear(); env->DeleteLocalRef(ic); env->DeleteLocalRef(it); env->DeleteLocalRef(cc); env->DeleteLocalRef(vals); env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc); return nullptr; }
    jobject res = nullptr;
    while (env->CallBooleanMethod(it, hn)) {
        if (env->ExceptionCheck()) { env->ExceptionClear(); break; }
        jobject rec = env->CallObjectMethod(it, nx);
        if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
        if (!rec) continue;
        jclass rc = env->GetObjectClass(rec);
        jfieldID af = env->GetFieldID(rc, "activity", "Landroid/app/Activity;");
        if (af) {
            jobject ac = env->GetObjectField(rec, af);
            if (env->ExceptionCheck()) env->ExceptionClear();
            if (ac) { res = ac; env->DeleteLocalRef(rc); env->DeleteLocalRef(rec); break; }
        } else env->ExceptionClear();
        env->DeleteLocalRef(rc); env->DeleteLocalRef(rec);
    }
    env->DeleteLocalRef(ic); env->DeleteLocalRef(it); env->DeleteLocalRef(cc); env->DeleteLocalRef(vals);
    env->DeleteLocalRef(amc); env->DeleteLocalRef(ma); env->DeleteLocalRef(at); env->DeleteLocalRef(atc);
    return res;
}

extern "C"
ANativeActivity* GetANativeActivity() {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lk(mtx);
    if (g_initialized && g_nativeActivity.clazz) return &g_nativeActivity;
    if (!g_vm) return nullptr;
    JNIEnv* env = nullptr; bool att = false;
    jint g = g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (g == JNI_EDETACHED) { if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr; att = true; }
    else if (g != JNI_OK || !env) return nullptr;
    jobject a = GetCurrentActivity(env);
    if (!a) { if (att) g_vm->DetachCurrentThread(); return nullptr; }
    memset(&g_nativeActivity, 0, sizeof(g_nativeActivity));
    g_nativeActivity.vm = g_vm; g_nativeActivity.env = env;
    g_nativeActivity.clazz = env->NewGlobalRef(a);
    env->DeleteLocalRef(a);
    if (!g_nativeActivity.clazz) { if (att) g_vm->DetachCurrentThread(); memset(&g_nativeActivity, 0, sizeof(g_nativeActivity)); return nullptr; }
    g_initialized = true;
    return &g_nativeActivity;
}

#include <jni.h>
#include <android/native_activity.h>
#include <EGL/egl.h>

extern "C" ANativeActivity* GetANativeActivity();
extern "C" void SetANativeActivityVM(JavaVM* vm);

static android::HideRecAImGui* g_zel_imgui = nullptr;
static JavaVM* g_zel_vm = nullptr;

inline void ZEL_SetVM(JavaVM* vm) { g_zel_vm = vm; SetANativeActivityVM(vm); }
inline bool ZEL_Render(ImDrawData* dd, EGLDisplay d, EGLSurface s, EGLContext c, android::HideRecAImGui::SwapBuffersFn sb) {
    if (!g_zel_imgui) {
        ANativeActivity* a = GetANativeActivity();
        if (!a) return false;
        android::HideRecAImGui::Options o; o.activity = a; o.skipScreenshot = true;
        g_zel_imgui = new android::HideRecAImGui(o);
    }
    return g_zel_imgui->RenderDrawData(dd, d, s, c, sb);
}
inline void ZEL_Shutdown() { if (g_zel_imgui) { delete g_zel_imgui; g_zel_imgui = nullptr; } }
inline bool ZEL_IsReady() { return g_zel_imgui && (bool)*g_zel_imgui; }

class _BYTE; class _BOOL4; class _BOOL8; class _WORD; class _DWORD; class _QWORD;

#define CREATE_COLOR(r, g, b, a) new float[4] {(float)(r) / 255.0f, (float)(g) / 255.0f, (float)(b) / 255.0f, (float)(a) / 255.0f}
bool g_clearMousePos = false;
bool ClearDisplay = false;
bool ShowFPS;
bool SnowB = false;
bool HIDEESP = false;
bool UnlimitedFly = false;   // <-- Unlimited Fly toggle
float SnowBsize = 0.0f;
bool isSpeedHackEnabled = false;
float speedHackMultiplier = 1.0f;
bool RedWallhackShow = false;
bool isJumpAdjustmentEnabled = false;
float jumpHeightMultiplier = 1.0f;
char logintext[4096];
float menu[4] = {188.0f / 255.0f, 110.0f / 255.0f, 1.0f, 1.0f};

float g_LastLogoOpacity = 1.0f;
float g_LastLogoSize = 1.0f;
int g_LogoHideDelayFrames = 0;
int g_LogoHideDelay = 40;

#define _BYTE uint8_t
#define _WORD  uint8_t
#define _DWORD uint64_t
#define _QWORD uint64_t
#define _BOOL4 uint8_t

#include <fstream>
using namespace std;

#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
ImFont* SOCIAL = nullptr;
ImFont* Bold = nullptr;
JavaVM* jvm = nullptr;
JavaVM* VM = nullptr;

namespace font {
    ImFont* icomoon_logo = nullptr;
    ImFont* inter_semibold = nullptr;
    ImFont* icomoon_page = nullptr;
}

static int g_GlWidth, g_GlHeight;
static bool g_App = false;
static std::mutex g_imguiInitMutex;
static EGLDisplay g_imguiDisplay = EGL_NO_DISPLAY;
static EGLContext g_imguiContext = EGL_NO_CONTEXT;
static EGLSurface g_imguiSurface = EGL_NO_SURFACE;
static bool g_imguiBackendReady = false;    struct My_Patches {
    MemoryPatch A1, grap, NoCrouch, SpeedhackX, SpeedhackX1, NoWingsuit, fpss, frame;
    MemoryPatch NoHitFlinch, NoDrift;
    // 8-byte leaf getters - they can never be inline-hooked, see Feats.h /
    // NewFeatures.h: A64 hooking writes a 16-byte stub in place and would take
    // the *next* function in libunity.so with it.
    MemoryPatch Em3Eye, KineticArmor;
    MemoryPatch NoHipfireUseGetReadyTime, NoHipfireSpringFireGetReady;
    // No Slide Fatigue: get_UseSlideTackleLimit is an 8-byte leaf getter whose
    // next function is LimitSlideTackle(), so it must be a byte patch, never an
    // inline hook (see the NewFeatures.h note). Pve = the arcade/PVE override.
    MemoryPatch NoSlideFatigue, NoSlideFatiguePve;
} Patches;

float AVIWA = 119.167f;
bool wallh;
bool active = false;
float AimSmooth = 1.0f;
bool showKeyboard = false;
static bool g_RuntimeClearDisplayInit = false;

struct ClearDisplayDefaultInit {
    ClearDisplayDefaultInit() { Config.ExtraMenu.ClearDisplay = true; }
} g_ClearDisplayDefaultInit;

struct sRegion { uintptr_t start, end; };

std::chrono::steady_clock::time_point appStartTime = std::chrono::steady_clock::now();

static float veh_min = 0.1f;
static float veh_max = 5.0f;
static bool windowCollapsed = false;
static double collapseBarLastActiveTime = 0.0;
static float collapseBarOpacityAnim = 1.0f;
static float collapseBarPressAnim = 0.0f;
static float collapseBarEnterAnim = 1.0f;
static float collapseBarRestoreAnim = 1.0f;
static bool collapseBarRestoreActive = false;
static bool collapseBarWasCollapsed = false;
static float uncollapseOpenAnim = 1.0f;
static bool dark = true;
static float tabAlpha = 0.0f;
static float tabAdd = 0.0f;
static int page = 1;
static int activeTab = 1;
bool g_LogoPreviewMode = false;
static bool isLogin = false;
static std::string err;
static std::string storedKey = "";
static char s[256];
static bool g_LoginTextLoaded = false;



std::vector<sRegion> trapRegions;
uintptr_t address = 0;
std::string md5(std::string s);
uintptr_t g_il2cpp;
static bool isMenuVisible = true;

int TABG = 1;

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);

static void DrawEspPreview(ImDrawList* draw, ImVec2 pos, float width, float height) {
    draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(10, 12, 18, 245));
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(70, 70, 82, 180), 8.0f, 0, 1.0f);

    static float animTime = 0.0f;
    animTime += ImGui::GetIO().DeltaTime;
    float sway     = sinf(animTime * 1.1f) * 4.0f;
    float hairSway = sinf(animTime * 1.5f) * 3.0f;
    float blink    = fmodf(animTime, 4.5f);
    bool  eyesShut = blink > 4.25f && blink < 4.38f;

    ImVec2 center = ImVec2(pos.x + width * 0.5f + sway, pos.y + height * 0.50f + height * 0.02f);
    float s = std::min(width, height) * 0.72f;

    auto P = [&](float nx, float ny) -> ImVec2 { return ImVec2(center.x + nx * s, center.y + ny * s); };

    ImU32 skinShade = IM_COL32(232, 200, 178, 255);
    ImU32 skinBase  = IM_COL32(248, 222, 200, 255);
    ImU32 skinHigh  = IM_COL32(255, 236, 220, 255);
    ImU32 hairDark  = IM_COL32(78, 60, 130, 255);
    ImU32 hairBase  = IM_COL32(140, 110, 200, 255);
    ImU32 hairHigh  = IM_COL32(200, 170, 250, 255);
    ImU32 hairTip   = IM_COL32(232, 130, 220, 255);
    ImU32 coatDark  = IM_COL32(28, 22, 48, 255);
    ImU32 coatBase  = IM_COL32(48, 38, 82, 255);
    ImU32 coatTrim  = IM_COL32(160, 80, 255, 255);
    ImU32 pantsCol  = IM_COL32(24, 20, 40, 255);
    ImU32 bootCol   = IM_COL32(14, 12, 24, 255);
    ImU32 bootHigh  = IM_COL32(90, 60, 160, 255);
    ImU32 eyeWhite  = IM_COL32(250, 248, 255, 255);
    ImU32 eyeIris   = IM_COL32(150, 90, 230, 255);
    ImU32 eyePupil  = IM_COL32(28, 12, 50, 255);
    ImU32 outline   = IM_COL32(8, 4, 20, 220);

    auto drawLimb = [&](ImVec2 a, ImVec2 b, float w, ImU32 col) {
        draw->AddLine(a, b, col, w);
        draw->AddCircleFilled(a, w * 0.5f, col, 12);
        draw->AddCircleFilled(b, w * 0.5f, col, 12);
    };

    ImVec2 headC  = P(0.00f, -0.72f);
    float  headR  = s * 0.155f;
    ImVec2 chin   = P(0.00f, -0.55f);
    ImVec2 neck   = P(0.00f, -0.52f);
    ImVec2 shL    = P(-0.19f, -0.42f);
    ImVec2 shR    = P( 0.19f, -0.42f);
    ImVec2 chest  = P(0.00f, -0.24f);
    ImVec2 waist  = P(0.00f, -0.05f);
    ImVec2 hipL   = P(-0.13f,  0.05f);
    ImVec2 hipR   = P( 0.13f,  0.05f);
    ImVec2 elbL   = P(-0.30f, -0.10f);
    ImVec2 elbR   = P( 0.30f, -0.10f);
    ImVec2 wrL    = P(-0.31f,  0.20f);
    ImVec2 wrR    = P( 0.31f,  0.20f);
    ImVec2 knL    = P(-0.12f,  0.38f);
    ImVec2 knR    = P( 0.12f,  0.38f);
    ImVec2 anL    = P(-0.12f,  0.70f);
    ImVec2 anR    = P( 0.12f,  0.70f);

    {
        ImVec2 hw = ImVec2(hairSway * 0.5f, 0);
        ImVec2 backHair[] = {
            ImVec2(headC.x - headR * 0.95f + hw.x, headC.y - headR * 0.4f),
            ImVec2(headC.x - headR * 1.15f + hw.x, headC.y + headR * 0.6f),
            ImVec2(headC.x - headR * 1.05f + hw.x, headC.y + headR * 1.8f),
            ImVec2(waist.x - s * 0.16f + hw.x,     waist.y + s * 0.05f),
            ImVec2(waist.x - s * 0.10f + hw.x,     waist.y + s * 0.22f),
            ImVec2(waist.x + s * 0.10f + hw.x,     waist.y + s * 0.22f),
            ImVec2(waist.x + s * 0.16f + hw.x,     waist.y + s * 0.05f),
            ImVec2(headC.x + headR * 1.05f + hw.x, headC.y + headR * 1.8f),
            ImVec2(headC.x + headR * 1.15f + hw.x, headC.y + headR * 0.6f),
            ImVec2(headC.x + headR * 0.95f + hw.x, headC.y - headR * 0.4f),
        };
        draw->AddConvexPolyFilled(backHair, 10, hairDark);
        ImVec2 tipPts[] = {
            ImVec2(headC.x - headR * 1.05f + hw.x, headC.y + headR * 1.8f),
            ImVec2(waist.x - s * 0.16f + hw.x,     waist.y + s * 0.05f),
            ImVec2(waist.x - s * 0.10f + hw.x,     waist.y + s * 0.22f),
            ImVec2(waist.x + s * 0.10f + hw.x,     waist.y + s * 0.22f),
            ImVec2(waist.x + s * 0.16f + hw.x,     waist.y + s * 0.05f),
            ImVec2(headC.x + headR * 1.05f + hw.x, headC.y + headR * 1.8f),
        };
        draw->AddConvexPolyFilled(tipPts, 6, hairTip);
    }

    drawLimb(hipR, knR, s * 0.100f, pantsCol);
    drawLimb(knR, anR, s * 0.085f, bootCol);
    drawLimb(hipL, knL, s * 0.100f, pantsCol);
    drawLimb(knL, anL, s * 0.085f, bootCol);

    draw->AddLine(P(0.075f, 0.52f), P(0.14f, 0.60f), bootHigh, 1.4f);
    draw->AddLine(P(-0.14f, 0.52f), P(-0.075f, 0.60f), bootHigh, 1.4f);

    draw->AddRectFilled(P(-0.185f, 0.70f), P(-0.070f, 0.78f), bootCol, 2.5f);
    draw->AddRectFilled(P( 0.070f, 0.70f), P( 0.185f, 0.78f), bootCol, 2.5f);
    draw->AddRect(P(-0.185f, 0.70f), P(-0.070f, 0.78f), outline, 2.5f, 0, 1.0f);
    draw->AddRect(P( 0.070f, 0.70f), P( 0.185f, 0.78f), outline, 2.5f, 0, 1.0f);

    drawLimb(shR, elbR, s * 0.078f, coatBase);
    drawLimb(elbR, wrR, s * 0.062f, skinBase);
    draw->AddCircleFilled(wrR, s * 0.038f, skinHigh, 14);

    ImVec2 coat[] = {
        ImVec2(shL.x + s * 0.01f, shL.y - s * 0.02f),
        ImVec2(waist.x - s * 0.11f, waist.y),
        ImVec2(waist.x - s * 0.14f, waist.y + s * 0.15f),
        ImVec2(waist.x - s * 0.10f, waist.y + s * 0.22f),
        ImVec2(waist.x + s * 0.10f, waist.y + s * 0.22f),
        ImVec2(waist.x + s * 0.14f, waist.y + s * 0.15f),
        ImVec2(waist.x + s * 0.11f, waist.y),
        ImVec2(shR.x - s * 0.01f, shR.y - s * 0.02f),
    };
    draw->AddConvexPolyFilled(coat, 8, coatBase);

    draw->AddLine(ImVec2(chest.x - s * 0.01f, shL.y), ImVec2(waist.x, waist.y + s * 0.20f), coatTrim, 1.8f);
    draw->AddLine(ImVec2(chest.x + s * 0.01f, shL.y), ImVec2(waist.x, waist.y + s * 0.20f), coatTrim, 1.8f);
    draw->AddLine(shL, shR, coatDark, 2.5f);
    draw->AddLine(ImVec2(shL.x, shL.y), ImVec2(chest.x, chest.y - s * 0.02f), coatTrim, 1.2f);
    draw->AddLine(ImVec2(shR.x, shR.y), ImVec2(chest.x, chest.y - s * 0.02f), coatTrim, 1.2f);

    for (int i = 0; i < 8; i++) {
        ImVec2 a = coat[i], b = coat[(i + 1) % 8];
        draw->AddLine(a, b, outline, 1.0f);
    }

    drawLimb(shL, elbL, s * 0.078f, coatBase);
    drawLimb(elbL, wrL, s * 0.062f, skinBase);
    draw->AddCircleFilled(wrL, s * 0.038f, skinHigh, 14);

    drawLimb(neck, ImVec2(neck.x, chest.y - s * 0.02f), s * 0.055f, skinBase);

    ImVec2 headBounds[] = {
        ImVec2(headC.x,                 headC.y - headR * 1.05f),
        ImVec2(headC.x + headR * 0.75f, headC.y - headR * 0.95f),
        ImVec2(headC.x + headR * 1.00f, headC.y - headR * 0.40f),
        ImVec2(headC.x + headR * 0.90f, headC.y + headR * 0.30f),
        ImVec2(headC.x + headR * 0.55f, headC.y + headR * 0.90f),
        ImVec2(chin.x,                  chin.y),
        ImVec2(headC.x - headR * 0.55f, headC.y + headR * 0.90f),
        ImVec2(headC.x - headR * 0.90f, headC.y + headR * 0.30f),
        ImVec2(headC.x - headR * 1.00f, headC.y - headR * 0.40f),
        ImVec2(headC.x - headR * 0.75f, headC.y - headR * 0.95f),
    };
    draw->AddConvexPolyFilled(headBounds, 10, skinBase);

    draw->AddCircleFilled(ImVec2(headC.x - headR * 0.98f, headC.y + headR * 0.12f), headR * 0.16f, skinBase, 16);
    draw->AddCircleFilled(ImVec2(headC.x + headR * 0.98f, headC.y + headR * 0.12f), headR * 0.16f, skinBase, 16);
    draw->AddLine(ImVec2(headC.x - headR * 0.99f, headC.y + headR * 0.12f),
                  ImVec2(headC.x - headR * 0.91f, headC.y + headR * 0.20f), skinShade, 1.0f);
    draw->AddLine(ImVec2(headC.x + headR * 0.99f, headC.y + headR * 0.12f),
                  ImVec2(headC.x + headR * 0.91f, headC.y + headR * 0.20f), skinShade, 1.0f);

    draw->AddCircleFilled(ImVec2(headC.x - headR * 0.55f, headC.y + headR * 0.25f), headR * 0.18f, IM_COL32(255, 150, 180, 90), 20);
    draw->AddCircleFilled(ImVec2(headC.x + headR * 0.55f, headC.y + headR * 0.25f), headR * 0.18f, IM_COL32(255, 150, 180, 90), 20);

    {
        float browY = headC.y - headR * 0.43f;
        float browSpacing = headR * 0.48f;
        float browW = headR * 0.27f;
        draw->AddLine(ImVec2(headC.x - browSpacing - browW, browY + headR * 0.02f),
                      ImVec2(headC.x - browSpacing + browW, browY - headR * 0.02f), outline, 1.6f);
        draw->AddLine(ImVec2(headC.x + browSpacing - browW, browY - headR * 0.02f),
                      ImVec2(headC.x + browSpacing + browW, browY + headR * 0.02f), outline, 1.6f);
    }

    if (!eyesShut) {
        float eyeY = headC.y - headR * 0.10f;
        float eyeSpacing = headR * 0.48f;
        float eyeW = headR * 0.34f;
        float eyeH = headR * 0.46f;

        auto drawEye = [&](float x) {
            ImVec2 eC(x, eyeY);
            draw->AddEllipseFilled(eC, eyeW, eyeH, eyeWhite, 0.0f, 24);
            draw->AddEllipseFilled(eC, eyeW * 0.72f, eyeH * 0.85f, eyeIris, 0.0f, 24);
            draw->AddEllipseFilled(eC, eyeW * 0.32f, eyeH * 0.42f, eyePupil, 0.0f, 20);
            draw->PathClear();
            draw->PathArcTo(ImVec2(eC.x, eC.y - eyeH * 0.10f), eyeW, IM_PI + 0.25f, -0.25f, 20);
            draw->PathStroke(outline, false, 1.8f);
            draw->AddCircleFilled(ImVec2(eC.x - eyeW * 0.28f, eC.y - eyeH * 0.35f), eyeW * 0.22f, IM_COL32(255,255,255,240), 12);
            draw->AddCircleFilled(ImVec2(eC.x + eyeW * 0.25f, eC.y + eyeH * 0.30f), eyeW * 0.10f, IM_COL32(255,255,255,180), 10);
        };
        drawEye(headC.x - eyeSpacing);
        drawEye(headC.x + eyeSpacing);
    } else {
        float eyeY = headC.y - headR * 0.10f;
        float eyeSpacing = headR * 0.45f;
        float eyeW = headR * 0.38f;
        draw->PathClear();
        draw->PathArcTo(ImVec2(headC.x - eyeSpacing, eyeY), eyeW, IM_PI + 0.2f, -0.2f, 20);
        draw->PathStroke(outline, false, 1.8f);
        draw->PathClear();
        draw->PathArcTo(ImVec2(headC.x + eyeSpacing, eyeY), eyeW, IM_PI + 0.2f, -0.2f, 20);
        draw->PathStroke(outline, false, 1.8f);
    }

    draw->AddCircleFilled(ImVec2(headC.x + headR * 0.10f, headC.y + headR * 0.22f), headR * 0.03f, IM_COL32(200, 150, 130, 200), 8);
    draw->PathClear();
    draw->PathArcTo(ImVec2(headC.x, headC.y + headR * 0.42f), headR * 0.10f, 0.15f, IM_PI - 0.15f, 12);
    draw->PathStroke(IM_COL32(180, 90, 110, 220), false, 1.2f);

    {
        ImVec2 hw = ImVec2(hairSway, 0);
        ImVec2 bangs[] = {
            ImVec2(headC.x - headR * 1.05f + hw.x, headC.y - headR * 0.35f),
            ImVec2(headC.x - headR * 0.85f + hw.x, headC.y - headR * 1.10f),
            ImVec2(headC.x - headR * 0.30f + hw.x, headC.y - headR * 1.30f),
            ImVec2(headC.x + headR * 0.30f + hw.x, headC.y - headR * 1.30f),
            ImVec2(headC.x + headR * 0.85f + hw.x, headC.y - headR * 1.10f),
            ImVec2(headC.x + headR * 1.05f + hw.x, headC.y - headR * 0.35f),
            ImVec2(headC.x + headR * 0.55f + hw.x, headC.y - headR * 0.55f),
            ImVec2(headC.x + headR * 0.15f + hw.x, headC.y - headR * 0.85f),
            ImVec2(headC.x - headR * 0.10f + hw.x, headC.y - headR * 0.55f),
            ImVec2(headC.x - headR * 0.45f + hw.x, headC.y - headR * 0.85f),
        };
        draw->AddConvexPolyFilled(bangs, 10, hairBase);

        draw->AddLine(ImVec2(headC.x - headR * 0.55f, headC.y - headR * 0.95f),
                      ImVec2(headC.x - headR * 0.30f, headC.y - headR * 0.55f), hairHigh, 1.2f);
        draw->AddLine(ImVec2(headC.x + headR * 0.15f, headC.y - headR * 1.10f),
                      ImVec2(headC.x + headR * 0.30f, headC.y - headR * 0.55f), hairHigh, 1.2f);

        ImVec2 sideL[] = {
            ImVec2(headC.x - headR * 1.00f + hw.x, headC.y - headR * 0.30f),
            ImVec2(headC.x - headR * 1.15f + hw.x, headC.y + headR * 0.40f),
            ImVec2(headC.x - headR * 1.00f + hw.x, headC.y + headR * 1.40f),
            ImVec2(headC.x - headR * 0.85f + hw.x, headC.y + headR * 0.55f),
            ImVec2(headC.x - headR * 0.85f + hw.x, headC.y - headR * 0.20f),
        };
        draw->AddConvexPolyFilled(sideL, 5, hairBase);
        ImVec2 sideR[] = {
            ImVec2(headC.x + headR * 1.00f + hw.x, headC.y - headR * 0.30f),
            ImVec2(headC.x + headR * 1.15f + hw.x, headC.y + headR * 0.40f),
            ImVec2(headC.x + headR * 1.00f + hw.x, headC.y + headR * 1.40f),
            ImVec2(headC.x + headR * 0.85f + hw.x, headC.y + headR * 0.55f),
            ImVec2(headC.x + headR * 0.85f + hw.x, headC.y - headR * 0.20f),
        };
        draw->AddConvexPolyFilled(sideR, 5, hairBase);
    }

    float minX = headC.x - headR * 1.20f;
    float maxX = headC.x + headR * 1.20f;
    float minY = headC.y - headR * 1.35f;
    float maxY = anR.y + s * 0.12f;

    auto safeColor = [](float* col) -> ImU32 {
        if (!col) return IM_COL32(255, 255, 255, 255);
        return IM_COL32((int)(col[0]*255), (int)(col[1]*255), (int)(col[2]*255), (int)(col[3]*255));
    };

    if (Config.ESPMenu.Box) {
        ImU32 bc = safeColor(Config.sColorsESPPLAYER.BoxPLAYER);
        float bx = minX - 6, by = minY - 6, bw = (maxX - minX) + 12, bh = (maxY - minY) + 12;
        if (Config.ESPMenu.BoxType == EspBoxType::Outline) {
            draw->AddRect(ImVec2(bx, by), ImVec2(bx + bw, by + bh), bc, 0.0f, 0, 2.0f);
        } else if (Config.ESPMenu.BoxType == EspBoxType::Fill) {
            ImVec4 c = ImGui::ColorConvertU32ToFloat4(bc);
            draw->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + bh), ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 0.15f)));
            draw->AddRect(ImVec2(bx, by), ImVec2(bx + bw, by + bh), bc, 0.0f, 0, 2.0f);
        } else if (Config.ESPMenu.BoxType == EspBoxType::Corner) {
            float iw = bw / 4.0f, ih = bh / 4.0f;
            draw->AddLine(ImVec2(bx, by), ImVec2(bx + iw, by), bc, 2.0f);
            draw->AddLine(ImVec2(bx, by), ImVec2(bx, by + ih), bc, 2.0f);
            draw->AddLine(ImVec2(bx + bw - iw, by), ImVec2(bx + bw, by), bc, 2.0f);
            draw->AddLine(ImVec2(bx + bw, by), ImVec2(bx + bw, by + ih), bc, 2.0f);
            draw->AddLine(ImVec2(bx, by + bh - ih), ImVec2(bx, by + bh), bc, 2.0f);
            draw->AddLine(ImVec2(bx, by + bh), ImVec2(bx + iw, by + bh), bc, 2.0f);
            draw->AddLine(ImVec2(bx + bw - iw, by + bh), ImVec2(bx + bw, by + bh), bc, 2.0f);
            draw->AddLine(ImVec2(bx + bw, by + bh - ih), ImVec2(bx + bw, by + bh), bc, 2.0f);
        }
    }

    if (Config.ESPMenu.isPlayerLine) {
        ImU32 lc = safeColor(Config.sColorsESPPLAYER.LinePLAYER);
        ImVec2 from;
        if (Config.ESPMenu.Target == LineTarget::Bottom) from = ImVec2(pos.x + width * 0.5f, pos.y + height);
        else if (Config.ESPMenu.Target == LineTarget::Center) from = ImVec2(pos.x + width * 0.5f, pos.y + height * 0.5f);
        else from = ImVec2(pos.x + width * 0.5f, pos.y + 10);
        draw->AddLine(from, ImVec2(center.x, minY), lc, 1.6f);
    }

    if (Config.ESPMenu.Skeleton) {
        ImU32 sc = safeColor(Config.sColorsESPPLAYER.SkeletonPLAYER);
        draw->AddLine(headC, neck, sc, 1.6f);
        draw->AddLine(neck, shL, sc, 1.6f);
        draw->AddLine(neck, shR, sc, 1.6f);
        draw->AddLine(shL, elbL, sc, 1.6f);
        draw->AddLine(shR, elbR, sc, 1.6f);
        draw->AddLine(elbL, wrL, sc, 1.6f);
        draw->AddLine(elbR, wrR, sc, 1.6f);
        draw->AddLine(chest, waist, sc, 1.6f);
        draw->AddLine(waist, hipL, sc, 1.6f);
        draw->AddLine(waist, hipR, sc, 1.6f);
        draw->AddLine(hipL, knL, sc, 1.6f);
        draw->AddLine(hipR, knR, sc, 1.6f);
        draw->AddLine(knL, anL, sc, 1.6f);
        draw->AddLine(knR, anR, sc, 1.6f);
    }

    if (Config.ESPMenu.Health) {
        float hp = 0.72f;
        int r = (int)(255 * (1.0f - hp)), g = (int)(255 * hp);
        ImU32 hpc = IM_COL32(r, g, 0, 255);
        float bx = minX - 14, by = minY - 6, bw = 4.5f, bh = (maxY - minY) + 12;
        draw->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + bh), IM_COL32(0, 0, 0, 120));
        draw->AddRect(ImVec2(bx, by), ImVec2(bx + bw, by + bh), IM_COL32(0, 0, 0, 220), 0.0f, 0, 1.0f);
        draw->AddRectFilled(ImVec2(bx, by + bh * (1.0f - hp)), ImVec2(bx + bw, by + bh), hpc);
    }

    if (Config.ESPMenu.Name) {
        ImU32 nc = safeColor(Config.sColorsESPPLAYER.NamePLAYER);
        const char* name = "PLAYER";
        ImVec2 ts = ImGui::CalcTextSize(name);
        draw->AddText(ImVec2(center.x - ts.x * 0.5f, minY - ts.y - 8.0f), nc, name);
    }

    if (Config.ESPMenu.Distance) {
        ImU32 dc = safeColor(Config.sColorsESPPLAYER.DistancePLAYER);
        const char* dist = "50m";
        ImVec2 ts = ImGui::CalcTextSize(dist);
        draw->AddText(ImVec2(center.x - ts.x * 0.5f, maxY + 8.0f), dc, dist);
    }

    if (Config.ESPMenu.ShowFov) {
        float radius = Config.Aim.Cross * 0.6f;
        if (radius > 0)
            draw->AddCircle(ImVec2(pos.x + width * 0.5f, pos.y + height * 0.5f), radius, IM_COL32(160, 80, 255, 100), 0, 1.5f);
    }
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    if (!old_eglSwapBuffers) return EGL_FALSE;
    if (dpy == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE) return old_eglSwapBuffers(dpy, surface);
    EGLContext currentContext = eglGetCurrentContext();
    if (currentContext == EGL_NO_CONTEXT) return old_eglSwapBuffers(dpy, surface);
    EGLint qw = 0, qh = 0;
    if (eglQuerySurface(dpy, surface, EGL_WIDTH, &qw) != EGL_TRUE ||
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &qh) != EGL_TRUE || qw <= 0 || qh <= 0)
        return old_eglSwapBuffers(dpy, surface);
    g_GlWidth = qw; g_GlHeight = qh;

    if (!g_App) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL; io.LogFilename = NULL;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.MouseDoubleClickTime = 0.3f; io.MouseDragThreshold = 2.f;
        g_imguiBackendReady = ImGui_ImplOpenGL3_Init("#version 300 es");
        if (!g_imguiBackendReady) { ImGui::DestroyContext(); return old_eglSwapBuffers(dpy, surface); }
        g_imguiDisplay = dpy; g_imguiContext = currentContext; g_imguiSurface = surface;

        ImFontConfig ilc; ilc.MergeMode = false; ilc.PixelSnapH = true; ilc.FontDataOwnedByAtlas = false;
        font::icomoon_logo = io.Fonts->AddFontFromMemoryTTF((void*)icomoon_page, sizeof(icomoon_page), 20.f, &ilc);

        ImFontConfig ic; ic.MergeMode = false; ic.PixelSnapH = true; ic.FontDataOwnedByAtlas = false;
        font::inter_semibold = io.Fonts->AddFontFromMemoryTTF((void*)inter_semibold, sizeof(inter_semibold), 16.f, &ic);

        ImFontConfig pc; pc.MergeMode = false; pc.PixelSnapH = true; pc.FontDataOwnedByAtlas = false;
        font::icomoon_page = io.Fonts->AddFontFromMemoryTTF((void*)icomoon_page, sizeof(icomoon_page), 18.f, &pc);

        static const ImWchar icons_ranges[] = { 0xe000, 0xf8ff, 0 };
        ImFontConfig iconsCfg; iconsCfg.MergeMode = true; iconsCfg.PixelSnapH = true;
        iconsCfg.OversampleH = 2.5f; iconsCfg.OversampleV = 2.5f; iconsCfg.FontDataOwnedByAtlas = false;
        F107 = io.Fonts->AddFontFromMemoryCompressedTTF((void*)font_awesome_data1, (int)font_awesome_size1, 25.0f, &iconsCfg, icons_ranges);
        F50 = io.Fonts->AddFontFromMemoryTTF((void *)F50_data, F50_size, 30.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        if (!F107) F107 = font::inter_semibold;
        if (font::inter_semibold) io.FontDefault = font::inter_semibold;
        io.Fonts->Build();
        ImGui_ImplOpenGL3_CreateFontsTexture();

        memset(&Config, 0, sizeof(sConfig));
        Config.sColorsESPPLAYER.LinePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.BoxPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.NamePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.DistancePLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.HealthPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPPLAYER.SkeletonPLAYER = CREATE_COLOR(255, 0, 0, 255);
        Config.sColorsESPBOT.LineBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.BoxBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.NameBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.HealthBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.DistanceBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.SkeletonBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPOTHERS.PovOTHERS = CREATE_COLOR(225, 0, 0, 180);
        Config.ExtraMenu.NoGravityScale = 1.0f;
        // Mythic Attachments is ON by default: an applied skin behaves as if it
        // were really owned, so the weapon's attachments (the mythic's own ones
        // included) are not locked. Every other ExtraMenu option stays off; the
        // checkbox that turns this one off is in the HOME tab, directly
        // above No Slide Fatigue; its live counters are in the SKINS tab, next to
        // Unlock Blueprints. See System/Hooks/MythicAttachments.h.
        Config.ExtraMenu.MythicAttachments = true;
        Config.Aim.AimAssistSize = 0.0f;
        Config.Aim.Cross = 45.0f;
        Config.Aim.Target = EAimTarget::Heads;
        Config.Aim.Trigger = EAimTrigger::None;
        Config.Aim.By = EAim::Distance;
        Config.Bline = 2.0f;
        Config.Pline = 2.0f;
        g_App = true;
    }

    ImGuiIO *io = &ImGui::GetIO();
    screenWidth = (float)g_GlWidth;
    screenHeight = (float)g_GlHeight;
    io->DisplaySize = ImVec2((float)g_GlWidth, (float)g_GlHeight);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImDrawList *draw = ImGui::GetBackgroundDrawList();
    DrawESP(ImGui::GetBackgroundDrawList(), screenWidth, screenHeight, get_dpi());
    // Own dot crosshair, on the same draw list as the ESP (over the game, under
    // the menu). It is drawn at the exact point the triggerbot aims through.
    DrawDotCrosshair(ImGui::GetBackgroundDrawList(), screenWidth, screenHeight);
    // Mythic kill icon for the LOCAL player's held weapon. The in-match HUD
    // reads it from the weapon's InventoryInfo record (Weapon : Inventory,
    // m_InventoryInfo @0x98), so filling that in is what makes an applied mythic
    // skin show its kill icon - independently of whether the game calls (and
    // trusts) the getter hooks. Runs once per rendered frame and only writes
    // when the value actually differs.
    KillDecoration_ApplyLocalInventoryInfo();
    floating_info::Render(draw, screenWidth, screenHeight);

    if (windowCollapsed) {
        if (!collapseBarWasCollapsed) {
            collapseBarEnterAnim = 0.0f;
            collapseBarRestoreAnim = 1.0f;
            collapseBarRestoreActive = false;
            collapseBarOpacityAnim = 1.0f;
            collapseBarLastActiveTime = ImGui::GetTime();
        }
        collapseBarWasCollapsed = true;
        const float dt = ImGui::GetIO().DeltaTime;
        collapseBarEnterAnim = ImClamp(collapseBarEnterAnim + dt * 6.0f, 0.0f, 1.0f);
        if (collapseBarRestoreActive) collapseBarRestoreAnim = ImClamp(collapseBarRestoreAnim + dt * 8.0f, 0.0f, 1.0f);
        const float enterEase = collapseBarEnterAnim * collapseBarEnterAnim * (3.0f - 2.0f * collapseBarEnterAnim);
        const float restoreEase = collapseBarRestoreAnim * collapseBarRestoreAnim * (3.0f - 2.0f * collapseBarRestoreAnim);
        const float cAlpha = ImClamp(GetLogoOpacity(), 0.0f, 1.0f);
        const float cScale = ImClamp(GetLogoSizeMultiplier(), 0.1f, 2.0f);
        const ImVec2 disp = ImGui::GetIO().DisplaySize;
        const float baseLineW = 120.0f * c::scale * cScale;
        const float line_w = collapseBarRestoreActive ? ImLerp(baseLineW * 1.28f, baseLineW, restoreEase) : ImLerp(baseLineW * 1.28f, baseLineW, enterEase);
        const float line_h = collapseBarRestoreActive ? ImLerp(10.0f, 6.0f, restoreEase) : ImLerp(6.0f, 10.0f, enterEase);
        const float lineH = line_h * c::scale * cScale;
        const float click_w = line_w + 72.0f * c::scale;
        const float click_h = ImMax(28.0f * c::scale, lineH + 20.0f * c::scale);
        const float startY = disp.y - 10.0f * c::scale - lineH;
        const float endY = disp.y - 10.0f * c::scale - lineH;
        const float handleY = collapseBarRestoreActive ? ImLerp(startY, endY, restoreEase) : ImLerp(endY, startY, enterEase);
        const ImVec2 handlePos((disp.x - line_w) * 0.5f, handleY);
        const ImVec2 clickPos(handlePos.x - (click_w - line_w) * 0.5f, handlePos.y - (click_h - lineH) * 0.5f);

        ImGui::SetNextWindowPos(clickPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(click_w, click_h), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        if (ImGui::Begin("##indicator", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings)) {
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::InvisibleButton("##restoreclick", ImVec2(click_w, click_h));
            bool barHeld = ImGui::IsItemActive();
            bool barHovered = ImGui::IsItemHovered();
            if (barHovered || barHeld) collapseBarLastActiveTime = ImGui::GetTime();
            const bool idle = (ImGui::GetTime() - collapseBarLastActiveTime) > 5.0;
            const float targetOpacity = idle ? 0.20f : 1.0f;
            collapseBarOpacityAnim = ImLerp(collapseBarOpacityAnim, targetOpacity, ImGui::GetIO().DeltaTime * 9.0f);
            collapseBarPressAnim = ImLerp(collapseBarPressAnim, barHeld ? 1.0f : 0.0f, ImGui::GetIO().DeltaTime * 22.0f);
            if (ImGui::IsItemClicked()) {
                collapseBarRestoreActive = true;
                collapseBarRestoreAnim = 0.0f;
                collapseBarLastActiveTime = ImGui::GetTime();
            }
            float drawAlpha = ImClamp(collapseBarOpacityAnim * cAlpha, 0.0f, 1.0f);
            ImDrawList* ind = ImGui::GetWindowDrawList();
            const ImVec2 lineMin(windowPos.x + (click_w - line_w) * 0.5f, windowPos.y + (click_h - lineH) * 0.5f);
            const ImVec2 lineMax(lineMin.x + line_w, lineMin.y + lineH);
            ind->AddRectFilled(lineMin, lineMax, IM_COL32(255, 255, 255, (int)((225.0f + 30.0f * collapseBarPressAnim) * drawAlpha)), lineH * 0.5f);
            if (collapseBarRestoreActive && collapseBarRestoreAnim >= 1.0f) {
                windowCollapsed = false; isMenuVisible = true;
                uncollapseOpenAnim = 0.0f;
                collapseBarRestoreActive = false;
                collapseBarEnterAnim = 1.0f;
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    } else {
        collapseBarWasCollapsed = false;
        collapseBarEnterAnim = 1.0f;
    }

    ImVec2 ioDisplaySize = ImGui::GetIO().DisplaySize;

    if (isMenuVisible && !windowCollapsed) {
        if (!g_RuntimeClearDisplayInit) { Config.ExtraMenu.ClearDisplay = true; g_RuntimeClearDisplayInit = true; }
        if (!g_LoginTextLoaded && VM != nullptr) {
            if (LoadTextFromFile() && logintext[0] != '\0') {
                strncpy(s, logintext, sizeof(s) - 1);
                s[sizeof(s) - 1] = '\0';
                g_LoginTextLoaded = true;
            }
        }
        runtime_preview_menu::EnsureTexturesLoaded();
        main_runtime_theme::ApplyThemeState();

        ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (!isLogin && !ui_loading::IsActive()) {
            const float WW = ImMin(440.0f, ioDisplaySize.x * 0.92f);
            const float WH = ImMin(380.0f, ioDisplaySize.y * 0.92f);
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();

            ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});
            ImGui::SetNextWindowSize({WW, WH}, ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 10, 35, 240));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(160, 80, 255, 180));

            if (ImGui::Begin("##premium_login", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove)) {

                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 wp = ImGui::GetWindowPos();
                ImFont* titleFont = F50 ? F50 : ImGui::GetFont();
                ImFont* bodyFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();

                dl->AddRectFilled(wp, {wp.x + WW, wp.y + WH}, IM_COL32(20, 10, 35, 240), 20.0f);
                dl->AddRect(wp, {wp.x + WW, wp.y + WH}, IM_COL32(160, 80, 255, 100), 20.0f, 0, 1.5f);
                dl->AddRect({wp.x + 6, wp.y + 6}, {wp.x + WW - 6, wp.y + WH - 6}, IM_COL32(160, 80, 255, 30), 16.0f, 0, 1.0f);

                const char* brand = "ETHNIR NOIR";
                ImVec2 brandSize = titleFont->CalcTextSizeA(32.0f, FLT_MAX, 0.0f, brand);
                dl->AddText(titleFont, 32.0f, {wp.x + (WW - brandSize.x) * 0.5f, wp.y + 40.0f}, IM_COL32(160, 80, 255, 255), brand);

                const char* sub = "AUTHENTICATION";
                ImVec2 subSize = bodyFont->CalcTextSizeA(12.0f, FLT_MAX, 0.0f, sub);
                dl->AddText(bodyFont, 12.0f, {wp.x + (WW - subSize.x) * 0.5f, wp.y + 88.0f}, IM_COL32(180, 180, 200, 200), sub);

                dl->AddLine({wp.x + 60, wp.y + 110}, {wp.x + WW - 60, wp.y + 110}, IM_COL32(160, 80, 255, 60), 1.0f);

                const char* prompt = "Enter your license key to continue";
                ImVec2 promptSize = bodyFont->CalcTextSizeA(10.5f, FLT_MAX, 0.0f, prompt);
                dl->AddText(bodyFont, 10.5f, {wp.x + (WW - promptSize.x) * 0.5f, wp.y + 128.0f}, IM_COL32(160, 160, 180, 220), prompt);

                const float inputW = WW - 60.0f;
                const float inputX = wp.x + 30.0f;
                const float inputY = wp.y + 156.0f;
                const float inputH = 44.0f;

                dl->AddRectFilled({inputX, inputY}, {inputX + inputW, inputY + inputH}, IM_COL32(30, 15, 50, 220), 10.0f);
                static bool keyFocused = false;
                ImU32 bc = keyFocused ? IM_COL32(160, 80, 255, 255) : IM_COL32(160, 80, 255, 80);
                dl->AddRect({inputX, inputY}, {inputX + inputW, inputY + inputH}, bc, 10.0f, 0, keyFocused ? 2.0f : 1.0f);

                ImGui::SetCursorScreenPos({inputX + 12.0f, inputY + 10.0f});
                ImGui::PushItemWidth(inputW);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 240, 255));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
                ImGui::InputText("##license_key", s, sizeof(s), ImGuiInputTextFlags_CharsNoBlank);
                keyFocused = ImGui::IsItemActive() || ImGui::IsItemHovered();
                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar(2);
                ImGui::PopItemWidth();

                float btnY = inputY + inputH + 16.0f;
                ImGui::SetCursorScreenPos({inputX, btnY});
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(160, 80, 255, 220));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(190, 110, 255, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(120, 50, 200, 255));
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(15, 12, 25, 255));
                if (ImGui::Button("AUTHENTICATE", {inputW, 44.0f})) {
                    err = Login(s);
                    if (err == "OK") {
                        strncpy(logintext, s, sizeof(logintext) - 1);
                        logintext[sizeof(logintext) - 1] = '\0';
                        SaveLoginTextToFile(s);
                        g_LoginTextLoaded = true;
                        err.clear();
                        ui_loading::Start(7.0f);
                        
                    }
                }
                ImGui::PopStyleColor(4);

                float pasteBtnY = btnY + 44.0f + 8.0f;
                ImGui::SetCursorScreenPos({inputX, pasteBtnY});
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(160, 80, 255, 40));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(160, 80, 255, 80));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(160, 80, 255, 120));
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 80, 255, 220));
                if (ImGui::Button("PASTE FROM CLIPBOARD", {inputW, 36.0f})) {
                    std::string clip = getClipboard();
                    strncpy(s, clip.c_str(), sizeof(s) - 1);
                    s[sizeof(s) - 1] = '\0';
                }
                ImGui::PopStyleColor(4);

                if (!err.empty() && err != "OK") {
                    float errY = pasteBtnY + 36.0f + 8.0f;
                    ImGui::SetCursorScreenPos({inputX, errY});
                    ImVec2 errPos = ImGui::GetCursorScreenPos();
                    dl->AddRectFilled(errPos, {errPos.x + inputW, errPos.y + 30.0f}, IM_COL32(200, 50, 50, 40), 6.0f);
                    dl->AddRect(errPos, {errPos.x + inputW, errPos.y + 30.0f}, IM_COL32(200, 50, 150, 150), 6.0f, 0, 1.0f);
                    ImGui::SetCursorScreenPos({errPos.x + 12.0f, errPos.y + 8.0f});
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
                    ImGui::Text("%s", err.c_str());
                    ImGui::PopStyleColor();
                }

                const char* footer = "Secure connection  -  All data encrypted";
                ImVec2 footSize = bodyFont->CalcTextSizeA(9.0f, FLT_MAX, 0.0f, footer);
                dl->AddText(bodyFont, 9.0f, {wp.x + (WW - footSize.x) * 0.5f, wp.y + WH - 28.0f}, IM_COL32(120, 120, 140, 160), footer);
            }
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
        }
        else if (!isLogin && ui_loading::IsActive()) {
            if (ui_loading::RenderWindow((ImTextureID)(intptr_t)runtime_preview_menu::g_menuBackground.id)) {
                isLogin = true;
            }
        }
        else {
            uncollapseOpenAnim = ImClamp(uncollapseOpenAnim + ImGui::GetIO().DeltaTime * 5.0f, 0.0f, 1.0f);
            float openEase = uncollapseOpenAnim * uncollapseOpenAnim * (3.0f - 2.0f * uncollapseOpenAnim);
            float openAlpha = 0.2f + 0.8f * openEase;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, openAlpha);

            // Position and size both come from the display, so the window fits
            // whatever the render surface is (ImGui/menu_layout.h).
            const MenuLayout menuLayout = ComputeMenuLayout(ioDisplaySize);
            const float menuW = menuLayout.menuW;
            const float menuH = menuLayout.menuH;
            ImVec2 menuPos = menuLayout.menuPos;

            ImGui::SetNextWindowPos(menuPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f);

            ImGui::Begin("@Shaiaintshyy", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBackground);
            {
                runtime_preview_menu::StateRefs runtimeState{
                    dark, tabAlpha, tabAdd, page, activeTab,
                    windowCollapsed, isMenuVisible,
                    collapseBarLastActiveTime, collapseBarOpacityAnim, collapseBarPressAnim
                };
                {
                    using namespace runtime_preview_menu;
                    main_runtime_theme::ApplyAccentFromHue();

                    ImGuiStyle *runtimeStyle = &ImGui::GetStyle();
                    c::ApplyMainWindowStyle(*runtimeStyle);
                    c::UpdateTheme(runtimeState.dark, menu, ImGui::GetIO().DeltaTime);
                    main_runtime_theme::ApplyThemeState();

                    const ImVec2 runtimeWindowSize = ImGui::GetWindowSize();
                    const ImVec2 runtimeWindowPos = ImGui::GetWindowPos();
                    ImDrawList *runtimeDrawList = ImGui::GetWindowDrawList();

                    // Flat body panel. The wobbling 22px glow that used to outline
                    // this window is gone, so the window draws one plain rounded
                    // rectangle with the same 1px accent border the header uses -
                    // no wobble, no halo, no animation.
                    {
                        const ImVec2 bodyMin = runtimeWindowPos;
                        const ImVec2 bodyMax(runtimeWindowPos.x + runtimeWindowSize.x,
                                             runtimeWindowPos.y + runtimeWindowSize.y);
                        runtimeDrawList->AddRectFilled(bodyMin, bodyMax, IM_COL32(7, 10, 20, 236), 16.0f);
                        runtimeDrawList->AddRect(bodyMin, bodyMax,
                                                 main_runtime_theme::GetAccentTintU32(0.70f, 0.70f),
                                                 16.0f, 0, 1.0f);
                    }

                    const float outerPad = 14.0f;
                    const float layoutGap = 8.0f;
                    const float contentPadding = 10.0f;
                    const float columnGap = 10.0f;
                    const float headerHeight = 64.0f;
                    const float closeHeaderWidth = 72.0f;

                    const float contentLeft = runtimeWindowPos.x + outerPad;
                    const float contentRight = runtimeWindowPos.x + runtimeWindowSize.x - outerPad;
                    const float headerTop = runtimeWindowPos.y + outerPad;
                    const float headerWidth = contentRight - contentLeft;
                    const float headerMainWidth = ImMax(0.0f, headerWidth - closeHeaderWidth - layoutGap);
                    const ImVec2 headerMin(contentLeft, headerTop);
                    const ImVec2 headerMax(contentLeft + headerMainWidth, headerTop + headerHeight);
                    const ImVec2 closeCardMin(headerMax.x + layoutGap, headerTop);
                    const ImVec2 closeCardSize(closeHeaderWidth, headerHeight);

                    const ImVec2 hostMin(contentLeft, headerTop + headerHeight + layoutGap);
                    const ImVec2 hostMax(contentRight, runtimeWindowPos.y + runtimeWindowSize.y - outerPad);
                    // Top left of the content area inside the shell; the sidebar
                    // below splits it into the rail, the panel and the tab host.
                    const ImVec2 contentInnerMin(hostMin.x + contentPadding, hostMin.y + contentPadding);

                    runtimeState.page = ImClamp(runtimeState.page, 1, 6);
                    runtimeState.activeTab = ImClamp(runtimeState.activeTab, 1, 6);

                    runtimeDrawList->AddRectFilled(headerMin, headerMax, IM_COL32(7, 10, 20, 236), 11.0f);
                    runtimeDrawList->AddRect(headerMin, headerMax, main_runtime_theme::GetAccentTintU32(0.70f, 0.70f), 11.0f, 0, 1.0f);

                    const ImVec2 flameCenter(headerMin.x + 28.0f, headerMin.y + headerHeight * 0.5f);
                    // No glow halo behind the icon any more - the accent circle used
                    // to be a soft light spill ("neon"); the icon itself stays.
                    {
                        ImFont *iconFont = custom::shell::GetIconFont();
                        const char *logoIcon = ICON_FA_FIRE;
                        const ImVec2 ls = custom::shell::MeasureText(iconFont, 17.0f, logoIcon);
                        runtimeDrawList->AddText(iconFont, 17.0f,
                            ImVec2(flameCenter.x - ls.x * 0.5f, flameCenter.y - ls.y * 0.5f),
                            main_runtime_theme::GetAccentU32(), logoIcon);
                    }

                    ImFont *runtimeTitleFont = custom::shell::GetTitleFont();
                    const float runtimeTitleSize = 22.0f;
                    const char *titleA = "ETHNIR NOIR";
                    const char *titleB = " V3";
                    const ImVec2 titleASize = runtimeTitleFont->CalcTextSizeA(runtimeTitleSize, FLT_MAX, 0.0f, titleA);
                    runtimeDrawList->AddText(runtimeTitleFont, runtimeTitleSize,
                        ImVec2(headerMin.x + 56.0f, headerMin.y + 11.0f), IM_COL32(245, 245, 250, 255), titleA);
                    runtimeDrawList->AddText(runtimeTitleFont, runtimeTitleSize,
                        ImVec2(headerMin.x + 56.0f + titleASize.x, headerMin.y + 11.0f), main_runtime_theme::GetAccentU32(), titleB);

                    const ImVec2 subtitlePos(headerMin.x + 57.0f, headerMin.y + 38.0f);
                    runtimeDrawList->AddText(runtimeTitleFont, 10.0f, subtitlePos, IM_COL32(135, 143, 165, 220), "PLAY LIKE A PLAYER  |  NOT A CHEATER");

                    const ImVec2 trackMin(headerMin.x + headerMainWidth * 0.58f, headerMin.y + 28.0f);
                    const ImVec2 trackMax(headerMax.x - 18.0f, trackMin.y + 8.0f);
                    ImGui::SetCursorScreenPos(ImVec2(trackMin.x - 4.0f, trackMin.y - 7.0f));
                    ImGui::InvisibleButton("##main_runtime_menu_color_slider_new", ImVec2((trackMax.x - trackMin.x) + 8.0f, 22.0f));
                    if (ImGui::IsItemActive()) {
                        main_runtime_theme::g_menuHue = custom::shell::Normalize(ImGui::GetIO().MousePos.x, trackMin.x, trackMax.x);
                        main_runtime_theme::ApplyAccentFromHue();
                        c::UpdateTheme(runtimeState.dark, menu, ImGui::GetIO().DeltaTime);
                        main_runtime_theme::ApplyThemeState();
                    }
                    const float segments = 8.0f;
                    const float segmentWidth = (trackMax.x - trackMin.x) / segments;
                    for (int i = 0; i < (int)segments; ++i) {
                        float r0, g0, b0, r1, g1, b1;
                        ImGui::ColorConvertHSVtoRGB((float)i / segments, 0.90f, 1.0f, r0, g0, b0);
                        ImGui::ColorConvertHSVtoRGB((float)(i + 1) / segments, 0.90f, 1.0f, r1, g1, b1);
                        const ImVec2 a(trackMin.x + segmentWidth * i, trackMin.y);
                        const ImVec2 b((i == (int)segments - 1) ? trackMax.x : a.x + segmentWidth + 1.0f, trackMax.y);
                        runtimeDrawList->AddRectFilledMultiColor(a, b,
                            ImGui::ColorConvertFloat4ToU32(ImVec4(r0,g0,b0,1)),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(r1,g1,b1,1)),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(r1,g1,b1,1)),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(r0,g0,b0,1)));
                    }
                    const float hueX = trackMin.x + ImClamp(main_runtime_theme::g_menuHue, 0.0f, 1.0f) * (trackMax.x - trackMin.x);
                    runtimeDrawList->AddCircleFilled(ImVec2(hueX, trackMin.y + 4.0f), 5.0f, IM_COL32(245,245,250,255), 20);
                    runtimeDrawList->AddCircle(ImVec2(hueX, trackMin.y + 4.0f), 6.5f, main_runtime_theme::GetAccentU32(), 20, 1.5f);

                    if (custom::shell::DrawCloseCard(closeCardMin, closeCardSize)) {
                        runtime_preview_menu::CollapseMenu(runtimeState);
                    }

                    runtimeState.tabAlpha = ImClamp(runtimeState.tabAlpha + (4.0f * ImGui::GetIO().DeltaTime * (runtimeState.page == runtimeState.activeTab ? 1.0f : -1.0f)), 0.0f, 1.0f);
                    if (runtimeState.tabAlpha == 0.0f && runtimeState.tabAdd == 0.0f) runtimeState.activeTab = runtimeState.page;

                    runtimeDrawList->AddRectFilled(hostMin, hostMax, ImGui::GetColorU32(main_runtime_theme::GetSidebarShellBackgroundColor()), 11.0f);

                    // ---- Sidebar: the tab switcher -------------------------
                    // Tabs used to be picked from a floating radial wheel beside
                    // the window. They live in a sidebar inside the window now,
                    // with a hamburger at the top of the rail that folds it
                    // away. The sizes are pure functions of the host area
                    // (ImGui/menu_layout.h), so the rail, the panel and the tab
                    // content can never overlap whatever the surface is.
                    static bool  g_sidebarOpen = true;
                    static float g_sidebarAnim = 1.0f;
                    const float  sidebarTarget = g_sidebarOpen ? 1.0f : 0.0f;
                    g_sidebarAnim += (sidebarTarget - g_sidebarAnim) *
                                     ImMin(1.0f, ImGui::GetIO().DeltaTime * 12.0f);
                    if (g_sidebarAnim > 0.998f)      g_sidebarAnim = 1.0f;
                    else if (g_sidebarAnim < 0.002f) g_sidebarAnim = 0.0f;

                    const ImVec2 hostSize(ImMax(1.0f, hostMax.x - hostMin.x),
                                          ImMax(1.0f, hostMax.y - hostMin.y));
                    const SidebarLayout sidebar = ComputeSidebarLayout(hostSize, g_sidebarAnim);

                    const float  hostInnerH = ImMax(1.0f, (hostMax.y - contentPadding) - contentInnerMin.y);
                    const ImVec2 railMin(contentInnerMin.x, contentInnerMin.y);
                    const ImVec2 sidebarMin(railMin.x + sidebar.railW, contentInnerMin.y);
                    const ImVec2 sidebarMax(sidebarMin.x + sidebar.panelW, contentInnerMin.y + hostInnerH);
                    const ImVec2 contentMin(sidebarMin.x + sidebar.panelW + sidebar.gap, contentInnerMin.y);
                    const ImVec2 contentSize(ImMax(1.0f, (hostMax.x - contentPadding) - contentMin.x),
                                             hostInnerH);

                    // The rail never goes away, so a folded sidebar can always
                    // be brought back.
                    if (custom::shell::DrawSidebarButton("##menu_sidebar_toggle", ICON_FA_BARS,
                                                         g_sidebarOpen, railMin,
                                                         ImVec2(sidebar.railW, sidebar.railW)))
                        g_sidebarOpen = !g_sidebarOpen;
                    if (sidebar.panelW > 1.0f) {
                        runtimeDrawList->AddRectFilled(sidebarMin, sidebarMax,
                            ImGui::GetColorU32(main_runtime_theme::GetActiveTabBackgroundColor()), 9.0f);
                        runtimeDrawList->AddRect(sidebarMin, sidebarMax,
                            main_runtime_theme::GetAccentTintU32(0.22f, 0.35f), 9.0f, 0, 1.0f);

                        // Six pages, labelled with what they really draw: the
                        // tab the wheel called HOME is the switch page (2), and
                        // the one it called EXTRA is the changelog (5). Same
                        // order the tab strip on main used, plus the HOME entry
                        // the user asks for by name.
                        static const char *sbIcons[] = { ICON_FA_HOME, ICON_FA_EYE, ICON_FA_MICROCHIP,
                                                         ICON_FA_STAR, ICON_FA_SLIDERS_H, ICON_FA_COG };
                        static const char *sbNames[] = { "HOME", "ESP", "AIM", "SKINS", "MISC", "SETTINGS" };
                        static const int   sbIds[]   = { 2, 1, 3, 4, 5, 6 };

                        ImFont *sbIconFont = custom::shell::GetIconFont();
                        ImFont *sbTextFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();

                        // Clipped to the panel: while the fold animates the rows
                        // slide away, and they must not paint over the content.
                        ImGui::PushClipRect(sidebarMin, sidebarMax, true);
                        float rowY = sidebarMin.y + sidebar.topPad;
                        for (int i = 0; i < sidebar.rows; ++i) {
                            const ImVec2 rMin(sidebarMin.x + sidebar.sidePad, rowY);
                            const ImVec2 rSize(ImMax(1.0f, sidebar.panelW - sidebar.sidePad * 2.0f), sidebar.rowH);
                            const ImVec2 rMax(rMin.x + rSize.x, rMin.y + rSize.y);

                            ImGui::SetCursorScreenPos(rMin);
                            char rId[48];
                            snprintf(rId, sizeof(rId), "##menu_sidebar_%d", sbIds[i]);
                            ImGui::InvisibleButton(rId, rSize);
                            const bool hov = ImGui::IsItemHovered();
                            const bool act = (runtimeState.page == sbIds[i]);

                            if (act || hov) {
                                runtimeDrawList->AddRectFilled(rMin, rMax,
                                    act ? ImGui::GetColorU32(main_runtime_theme::GetActiveTabBackgroundColor())
                                        : IM_COL32(18, 20, 29, 150), 8.0f);
                            }
                            if (act) {
                                runtimeDrawList->AddRectFilled(rMin, ImVec2(rMin.x + 3.0f, rMax.y),
                                    main_runtime_theme::GetAccentU32(), 2.0f);
                            }

                            if (sbIconFont) {
                                runtimeDrawList->AddText(sbIconFont, 16.0f,
                                    ImVec2(rMin.x + 10.0f, rMin.y + (rSize.y - 16.0f) * 0.5f),
                                    act ? main_runtime_theme::GetAccentTintU32(0.98f, 1.0f)
                                        : IM_COL32(154, 160, 182, hov ? 245 : 220),
                                    sbIcons[i]);
                            }

                            if (rMax.x - 6.0f > rMin.x + 34.0f) {
                                runtimeDrawList->AddText(sbTextFont, 12.5f,
                                    ImVec2(rMin.x + 34.0f, rMin.y + (rSize.y - 12.5f) * 0.5f),
                                    act ? IM_COL32(245, 245, 250, 255)
                                        : IM_COL32(154, 160, 182, hov ? 245 : 205),
                                    sbNames[i]);
                            }

                            if (ImGui::IsItemClicked()) {
                                runtimeState.page = sbIds[i];
                                runtimeState.activeTab = sbIds[i];
                                g_LogoPreviewMode = false;
                            }
                            rowY += sidebar.rowH + sidebar.rowGap;
                        }
                        ImGui::PopClipRect();
                    }

                    ImGui::SetCursorScreenPos(contentMin);
                    ImGui::BeginChild("##RuntimeContentHost", contentSize, false, ImGuiWindowFlags_NoBackground);
                    {
                        const bool pushedContentFont = (font::inter_semibold != nullptr);
                        if (pushedContentFont) ImGui::PushFont(font::inter_semibold);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, runtimeState.tabAlpha * runtimeStyle->Alpha);

                        const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                        const float childHeight = ImMax(0.0f, contentRegion.y);
                        const float childWidth = ImMax(0.0f, (contentRegion.x - columnGap) * 0.5f);

                        if (runtimeState.activeTab == 2) {
                            custom::BeginGroup();
                            {
                                const ChildFrame left = BeginContentChild("EXTRA", ImVec2(childWidth, childHeight));
                                custom::Checkbox("Hitbox", &Config.ExtraMenu.Hit);
                                custom::Checkbox("No Recoil", &Config.ExtraMenu.Recoil);
                                custom::Checkbox("No Spread", &Config.ExtraMenu.Spread);
                                custom::Checkbox("No Shake", &Config.ExtraMenu.Shake);
                                custom::Checkbox("No Overheat", &Config.ExtraMenu.Rpd);
                                custom::Checkbox("No Parachute", &Config.ExtraMenu.Parachute);
                                custom::Checkbox("Anti Flashbang", &Config.ExtraMenu.Flash);
                                custom::Checkbox("Firerate", &Config.ExtraMenu.Fire);
                                custom::Checkbox("Unlimited Fly", &UnlimitedFly);
                                custom::Checkbox("Fast Dive", &Config.ExtraMenu.Diving);
                                custom::Checkbox("Fast Reload", &Config.ExtraMenu.Reload);
                                custom::Checkbox("Fast Scope", &Config.ExtraMenu.Scope);
                                custom::Checkbox("Quick Switch", &Config.ExtraMenu.Switch);
                                custom::Checkbox("Weapon Kinetic", &Config.ExtraMenu.Kinetic);
                                // === NEW FEATURES: Home ===
                                // No Slide Fatigue: remove the slide limit/fatigue system
                                // (SlideTackleComponent::get_UseSlideTackleLimit -> false,
                                // local pawn only). Unlimited slides, no idle-time decay.
                                // Mythic Attachments: an applied weapon skin behaves as
                                // if it were really owned, so its attachments (the
                                // mythic's own included) stop reading as locked.
                                // ON by default; the live counters sit in the SKINS
                                // tab next to Unlock Blueprints.
                                custom::Checkbox("Mythic Attachments", &Config.ExtraMenu.MythicAttachments);
                                custom::Checkbox("No Slide Fatigue", &Config.NewFeaturesMenu.NoSlideFatigue);
                                custom::Separator();
                                custom::Separator();
                                // === NEW FEATURES: Misc ===
                                custom::Checkbox("No Crouch", &Config.NewFeaturesMenu.NoCrouch);
                                custom::Checkbox("No Hit Flinch", &Config.NewFeaturesMenu.NoHitFlinch);
                                // === NEW FEATURES: Sniper ===
                                // No Drift = breathing sway only (m_AimmingBreatheCountDown @0x774)
                                // NOTE: the old broken disable came from Stable Scope's
                                // always-zero breathe hook (removed with that feature).
                                custom::Checkbox("No Drift (Hardscope)", &Config.NewFeaturesMenu.NoDrift);
                                // === NEW FEATURES: Firing timing ===
                                // (No Fire Delay and Stable Scope removed per request.)
                                custom::Checkbox("No Sprint Fire Delay", &Config.NewFeaturesMenu.NoSprintToFireDelay);
                                if (Config.NewFeaturesMenu.NoSprintToFireDelay) {
                                    // The option is applied inside the game's own
                                    // Weapon.get_DelaySprintFire, and that hook body only
                                    // compares the instance against a cache (it may not
                                    // call into the game - the native call it used to
                                    // make is what crashed this option). This line is
                                    // that cache: an empty one means the local player's
                                    // pawn tick has not run yet, so nothing is overridden.
                                    ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1), "  %s",
                                        NewFeatures::g_noSprintFireDelayWeapon.load() != nullptr
                                            ? "armed: local weapon cached"
                                            : "waiting for the local pawn tick");
                                }
                                custom::Checkbox("No Hipfire Delay", &Config.NewFeaturesMenu.NoHipfireDelay);
                                custom::Separator();
                                // Dev / Debug options
                                custom::Separator();
                                custom::Separator();

                                EndContentChild(left);
                            }
                            custom::EndGroup();
                            ImGui::SameLine(0.0f, 10.0f);
                            custom::BeginGroup();
                            {
                                const ChildFrame right = BeginContentChild("EXTRA OPTIONS", ImVec2(childWidth, childHeight));
                                custom::SliderFloat("Snowboard Speed", &SnowBsize, 0.0f, 100.0f, "%.1f");
                                custom::SliderFloat("Slide Distance", &SlideRange, 0.0f, 30.0f, "%.1f");
                                custom::SliderFloat("SpeedHack", &speedHackMultiplier, 0.5f, 2.0f, "%.1fx");
                                custom::SliderFloat("High Jump", &jumpHeightMultiplier, 0.5f, 5.0f, "%.2fx");
                                EndContentChild(right);
                            }
                            custom::EndGroup();
                        }

                        if (runtimeState.activeTab == 1) {
                            const float espGap = 8.0f;
                            const float espAvailW = ImMax(1.0f, contentRegion.x - espGap * 2.0f);
                            const float espSettingsW = ImFloor(espAvailW * 0.40f);
                            const float espOptionsW = ImFloor(espAvailW * 0.32f);
                            const float espPreviewW = ImMax(1.0f, espAvailW - espSettingsW - espOptionsW);

                            custom::BeginGroup();
                            {
                                const ChildFrame settings = BeginContentChild("ESP SETTINGS##RUNTIME_ESP", ImVec2(espSettingsW, childHeight));
                                custom::Checkbox("Hide Stream", &HIDEESP);
                                custom::Checkbox("ESP Line", &Config.ESPMenu.isPlayerLine);
                                custom::Checkbox("ESP Box", &Config.ESPMenu.Box);
                                custom::Checkbox("ESP Skeleton", &Config.ESPMenu.Skeleton);
                                custom::Checkbox("ESP Health", &Config.ESPMenu.Health);
                                custom::Checkbox("ESP Name", &Config.ESPMenu.Name);
                                custom::Checkbox("ESP Distance", &Config.ESPMenu.Distance);
                                custom::Checkbox("ESP Count", &Config.ESPMenu.Count);
                                custom::Checkbox("360 Alert", &Config.ESPMenu.Alert);
                                custom::Checkbox("Show AimLine", &Config.ESPMenu.Aimline);
                                custom::Checkbox("Yellow Wallhack", &Config.ExtraMenu.WallHack);
                                custom::Checkbox("Red Wallhack", &Config.ExtraMenu.RedWallhack);
                                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                                ImGui::TextColored(main_runtime_theme::GetAccentVec4(), "ESP COLORS");
                                runtime_preview_menu::DrawRuntimeEspColorRow("Player ESP Color", Config.sColorsESPPLAYER.LinePLAYER);
                                runtime_preview_menu::CopyLinkedEspColors(
                                    Config.sColorsESPPLAYER.LinePLAYER, Config.sColorsESPPLAYER.BoxPLAYER,
                                    Config.sColorsESPPLAYER.NamePLAYER, Config.sColorsESPPLAYER.HealthPLAYER,
                                    Config.sColorsESPPLAYER.DistancePLAYER, Config.sColorsESPPLAYER.SkeletonPLAYER);
                                runtime_preview_menu::DrawRuntimeEspColorRow("Bot ESP Color", Config.sColorsESPBOT.LineBOT);
                                runtime_preview_menu::CopyLinkedEspColors(
                                    Config.sColorsESPBOT.LineBOT, Config.sColorsESPBOT.BoxBOT,
                                    Config.sColorsESPBOT.NameBOT, Config.sColorsESPBOT.HealthBOT,
                                    Config.sColorsESPBOT.DistanceBOT, Config.sColorsESPBOT.SkeletonBOT);
                                EndContentChild(settings);
                            }
                            custom::EndGroup();
                            ImGui::SameLine(0.0f, espGap);
                            custom::BeginGroup();
                            {
                                const ChildFrame options = BeginContentChild("DISPLAY OPTIONS##RUNTIME_DISPLAY_OPTIONS", ImVec2(espOptionsW, childHeight));
                                static const char *boxTypes[] = {"Fill", "Outline", "Corner", "3D"};
                                static const char *linePositions[] = {"Top", "Mid", "Bottom"};
                                static const char *healthPositions[] = {"Top", "Side"};
                                static const char *espStyles[] = {"None", "3D Sphere", "Player Signal"};
                                custom::Combo("Box Type", (int *)&Config.ESPMenu.BoxType, boxTypes, IM_ARRAYSIZE(boxTypes), -1);
                                custom::Combo("Line Position", (int *)&Config.ESPMenu.Target, linePositions, IM_ARRAYSIZE(linePositions), -1);
                                custom::Combo("Health Position", (int *)&Config.ESPMenu.HealthPosition, healthPositions, IM_ARRAYSIZE(healthPositions), -1);
                                custom::Combo("ESP Style", (int *)&Config.ESPMenu.EspStyle, espStyles, IM_ARRAYSIZE(espStyles), -1);
                                ImGui::Dummy(ImVec2(0.0f, 8.0f));
                                ImGui::TextColored(main_runtime_theme::GetAccentVec4(), "EXTRA OPTIONS");
                                ImGui::TextUnformatted("Player ESP Color");
                                ImGui::TextUnformatted("Bot ESP Color");
                                EndContentChild(options);
                            }
                            custom::EndGroup();
                            ImGui::SameLine(0.0f, espGap);
                            ImGui::BeginChild("##CustomRealisticEspPreview", ImVec2(espPreviewW, childHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
                            {
                                ImVec2 previewPos = ImGui::GetCursorScreenPos();
                                ImVec2 previewSize = ImGui::GetContentRegionAvail();
                                DrawEspPreview(ImGui::GetWindowDrawList(), previewPos, previewSize.x, previewSize.y);
                            }
                            ImGui::EndChild();
                        }

if (runtimeState.activeTab == 4) {
    const ChildFrame skinChild = BeginContentChild("SKINS", ImVec2(contentRegion.x, childHeight));

    custom::Checkbox("Unlock Blueprints", &Config.ExtraMenu.Blueprints);
    // Attachments counted as unlocked for the weapon wearing an applied skin -
    // the mythic's own set included - so the loadout stops drawing them locked.
    // Client-side answers only; the server still knows the real inventory.
    // The "Mythic Attachments" switch itself lives in the HOME tab, directly
    // above No Slide Fatigue; its live counters stay here next to Unlock
    // Blueprints, which is the tab that owns the weapon skins.
    // Live instrument: how often the loadout / skin UI asked each of the four
    // unlock questions and how often this feature answered YES. That UI asks them
    // by itself, so non-zero counters are the on-device proof that the hooks are
    // installed at the dump's addresses (see System/Hooks/MythicAttachments.h).
    {
        namespace ma = mythic_attachments;
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1),
            "Unlock gates asked/granted: att %d/%d  skin %d/%d  loadout %d/%d  confirm %d/%d",
            ma::g_diag.asked[ma::GateAttachment], ma::g_diag.granted[ma::GateAttachment],
            ma::g_diag.asked[ma::GateSkin],       ma::g_diag.granted[ma::GateSkin],
            ma::g_diag.asked[ma::GateLoadout],    ma::g_diag.granted[ma::GateLoadout],
            ma::g_diag.asked[ma::GateConfirm],    ma::g_diag.granted[ma::GateConfirm]);
        // The three DATA-side writes (the part that behaves like the skin: the
        // client's own objects are rewritten, no answer function has to be asked
        // for the lock to disappear).
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1),
            "Client-side writes: list %d/%d  item %d/%d  cell %d/%d",
            ma::g_diag.asked[ma::GateList], ma::g_diag.granted[ma::GateList],
            ma::g_diag.asked[ma::GateItem], ma::g_diag.granted[ma::GateItem],
            ma::g_diag.asked[ma::GateCell], ma::g_diag.granted[ma::GateCell]);
        // Did the hook itself land? A 0 here means the address never took the
        // hook (bad dump / failed install), which is why the counters above stay
        // at 0/0 - it is the first thing to check on a game update.
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1),
            "Hooks landed: att %d skin %d loadout %d confirm %d | list %d item %d cell %d",
            ma::g_landed[ma::GateAttachment] ? 1 : 0, ma::g_landed[ma::GateSkin] ? 1 : 0,
            ma::g_landed[ma::GateLoadout] ? 1 : 0,    ma::g_landed[ma::GateConfirm] ? 1 : 0,
            ma::g_landed[ma::GateList] ? 1 : 0,       ma::g_landed[ma::GateItem] ? 1 : 0,
            ma::g_landed[ma::GateCell] ? 1 : 0);
    }

    RenderSkinCategoryContent(skinSubTab, true);
    EndContentChild(skinChild);
}

                        if (runtimeState.activeTab == 3) {
                            custom::BeginGroup();
                            {
                                const ChildFrame left = BeginContentChild("AIMBOT", ImVec2(childWidth, childHeight));
                                custom::Checkbox("Aimbot 360", &Config.Aim.Aimbot360);
                                custom::Checkbox("Bullet Track", &Config.Aim.AimSilent);
                                custom::SliderFloat("Aim Assist Size", &Config.Aim.AimAssistSize, 0.0f, 100.0f, "%.0f");
                                // Triggerbot (Combat)
                                custom::Separator();
                                custom::Checkbox("Triggerbot", &Config.NewFeaturesMenu.Triggerbot);
                                if (Config.NewFeaturesMenu.Triggerbot) {
                                    static const char* triggerModes[] = { "Always", "ADS Only", "Firing Only" };
                                    custom::Combo("Mode", &Config.NewFeaturesMenu.TriggerbotMode, triggerModes, IM_ARRAYSIZE(triggerModes));
                                    // No "Delay" slider: the trigger fires the frame the
                                    // target is under the crosshair, and the re-press rate
                                    // is the weapon's OWN fire interval (Weapon.
                                    // RealFireInterval), not a configured ms value. The
                                    // TriggerbotDelay field is kept only so old config
                                    // files keep loading.
                                    // The crosshair zone is not a number of its own any
                                    // more: it IS the dot crosshair's own size (see the
                                    // Dot Crosshair section below), so what the user sees
                                    // drawn is exactly what the trigger covers. The
                                    // TriggerbotRadius field is kept only so old config
                                    // files keep loading.
                                    ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1),
                                                       "Zone = the dot crosshair below");
                                    
                                    // No Hitbox pick: Head/Chest/Limbs/Any needed a
                                    // raw bone FIELD offset (m_HeadBone 0x308 /
                                    // m_Mesh 0x628) that did not hold up in BR (the
                                    // panel showed Cand=0 with 5 enemies, so the zone
                                    // path never fired). The trigger now uses the
                                    // enemy's own on-screen BODY - the game's
                                    // Pawn.get_HeadPosition getter and the root
                                    // transform, measured against the crosshair
                                    // segment and confirmed by the visibility ray.
                                    // No target filter any more (Visible Only / the
                                    // 25-50-100 m limits): it never changed a shot and
                                    // it silently dropped candidates the rest of the
                                    // gate accepts. The advisory options that used to sit
                                    // here (Lead Target, Check Spread, Require Ready)
                                    // could not change a shot either, and the debug Test
                                    // Fire button is gone too. Everything the trigger
                                    // obeys is above (Mode) plus StartFire itself - and
                                    // its crosshair zone, which is the dot below.
                                }
                                // ---- The dot crosshair (the trigger's crosshair) --------
                                // There is ONE crosshair now: this dot. The triggerbot
                                // aims through its pixel and its zone IS the dot's own
                                // radius, always - there is no separate trigger crosshair
                                // and no switch to connect the two. The checkbox decides
                                // whether the dot is DRAWN; the trigger uses its position
                                // either way.
                                custom::Separator();
                                custom::Checkbox("Dot Crosshair", &Config.Crosshair.Enabled);
                                ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.72f, 1),
                                                   "The triggerbot aims through this dot");
                                {
                                    // Opacity is the colour's own alpha: one value, so a
                                    // fade can never disagree with what is drawn.
                                    custom::SliderFloat("Opacity", &Config.Crosshair.Color[3], 0.0f, 1.0f, "%.2f");
                                    custom::SliderFloat("Size (trigger zone)", &Config.Crosshair.Size, 1.0f, 40.0f, "%.0f px");
                                    custom::SliderFloat("Offset X", &Config.Crosshair.OffsetX, -400.0f, 400.0f, "%.0f px");
                                    custom::SliderFloat("Offset Y", &Config.Crosshair.OffsetY, -400.0f, 400.0f, "%.0f px");
                                    ImGui::TextUnformatted("Crosshair Color");
                                    custom::ColorEdit4("##crosshair_color", Config.Crosshair.Color,
                                                       ImGuiColorEditFlags_NoInputs |
                                                       ImGuiColorEditFlags_PickerHueWheel);
                                    if (custom::Button("Reset To Centre",
                                                       ImVec2(ImMax(1.0f, ImGui::GetContentRegionAvail().x), 50.0f))) {
                                        Config.Crosshair.OffsetX = 0.0f;
                                        Config.Crosshair.OffsetY = 0.0f;
                                    }
                                    // These settings stay visible while the dot is hidden:
                                    // the trigger aims at these offsets regardless, so
                                    // hiding the dot must never hide the only way to see
                                    // and fix them.
                                    if (!Config.Crosshair.Enabled &&
                                        (Config.Crosshair.OffsetX != 0.0f || Config.Crosshair.OffsetY != 0.0f)) {
                                        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1),
                                                           "Dot hidden - the trigger still aims at these offsets");
                                    }
                                }
                                // Debug status (shows in-game). Every number below is
                                // written by the GAME-THREAD decision tree
                                // (NewFeatures::A_FireShouldFire) inside A_FireTick, so
                                // the headline IS the reason of the last scan: it prints
                                // the exact gate that refused the shot (FireBlock), not
                                // a guess. A_FireRenderTick() stays scan-free - the
                                // render thread must never run A_FireScan /
                                // Physics.Raycast, the physics world has exactly one
                                // caller (A_FireTick, game thread).
                                ImGui::Spacing();
                                NewFeatures::A_FireRenderTick();
                                NewFeatures::TriggerbotDiag& td = NewFeatures::GetTriggerbotDiag();
                                int dbgState = NewFeatures::GetTriggerbotDebugState();
                                const bool hbAlive = NewFeatures::A_FireHeartbeatAlive();
                                const NewFeatures::FireBlock lastBlock =
                                    (NewFeatures::FireBlock)td.lastBlock;
                                if (!hbAlive) {
                                    ImGui::TextColored(ImVec4(1, 0.25f, 0.25f, 1), "NO GAME-THREAD TICK");
                                } else if (dbgState == 2) {
                                    ImGui::TextColored(ImVec4(0, 1, 0, 1), ">> FIRING <<");
                                } else if (dbgState == 1) {
                                    if (td.paceWaiting)
                                        ImGui::TextColored(ImVec4(1, 1, 0, 1),
                                            "WAITING: aimed in, gun still cycling");
                                    else
                                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "HOLDING: aimed in");
                                } else {
                                    ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "BLOCKED: %s",
                                                       NewFeatures::A_FireBlockText(lastBlock));
                                }
                                    // The debug "Test Fire" button is gone: it only queued a
                                    // StartFire from the menu, which is a test harness, not a
                                    // feature. Everything below is the live state written by the
                                    // game-thread decision tree.
                                {
                                    const ImVec4 dim(0.65f, 0.65f, 0.72f, 1);
                                    ImGui::TextColored(dim, "HB %s  wTick=%d held=%d pawn=%d",
                                        hbAlive ? "OK" : "DEAD",
                                        td.weaponTicks, td.weaponHeldTicks, td.pawnLocalTicks);
                                    ImGui::TextColored(dim, "Dot=%.0f,%.0f %s zone=%.1fpx ray=%s",
                                        td.crosshairCx, td.crosshairCy,
                                        Config.Crosshair.Enabled ? "shown" : "hidden",
                                        td.radiusPx,
                                        td.crosshairRayViaGame ? "game-projection"
                                            : td.crosshairRayFellBack ? "basis-rebuilt"
                                            : "centre-ray");
                                    ImGui::TextColored(dim, "Scan=%d Enemy=%d Cand=%d Ray=%d Tgt=%d Fire=%d",
                                        td.scans, td.enemyCount, td.candidates,
                                        td.losChecked, td.targets, td.fires);
                                    ImGui::TextColored(dim, "R=%.0f Closest=%.0fpx LOSblk=%d Friend=%d",
                                        td.radiusPx,
                                        td.closestPx < 0.0f ? -1.0f : sqrtf(td.closestPx),
                                        td.losBlocked, td.friendsInLine);
                                    // "ADS only" is decided by the game's OWN getters -
                                    // Pawn.IsAiming 0x519BB10 and the held weapon's fire
                                    // component get_IsAiming 0xC9AF4E0 - each called only
                                    // on an object that passed the m_CachedPtr liveness
                                    // check. The RawField line under them is the two
                                    // Weapon field offsets, which are NOT verified and are
                                    // shown for the record only. NO SIGNAL means the mode
                                    // is not blocking at all (fail-open), which is why
                                    // that line turns red instead of silently doing
                                    // nothing.
                                    if (Config.NewFeaturesMenu.TriggerbotMode == 1) {
                                        if (td.adsUnknown)
                                            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1),
                                                "ADS: NO SIGNAL - mode not enforced (fail-open)");
                                        else
                                            ImGui::TextColored(dim, "ADS: pawn=%s fc=%s => %s",
                                                td.adsPawnOk ? (td.adsPawnAim ? "scoped" : "no") : "n/a",
                                                td.adsFcOk   ? (td.adsFcAim   ? "scoped" : "no") : "n/a",
                                                (td.adsPawnAim || td.adsFcAim) ? "SCOPED" : "hipfire");
                                    }
                                    ImGui::TextColored(dim, "RawField=%d/%.2f (offset unverified)",
                                        td.adsFlag, td.adsPct);
                                    // Re-press pacing: Pace is the wait a repeat press
                                    // obeys (the weapon's own cadence), skipped counts the
                                    // presses held back, waiting is this scan. The first
                                    // shot on a target is never paced.
                                    ImGui::TextColored(dim, "Pace=%.3fs skipped=%d waiting=%d reassert=%d",
                                        td.paceSeconds, td.repressSkipped,
                                        td.paceWaiting ? 1 : 0, td.reasserts);
                                    // Which path accepted (or would accept) the shot, and
                                    // what the visibility ray actually hit. "Rays=id/blk"
                                    // is the anti-wall gate: id = the nearest thing on the
                                    // shot line WAS the enemy, blk = something else (a wall,
                                    // however thin) stopped it.
                                    ImGui::TextColored(dim, "Path=%s Centre=%s@%.1fm Rays=%d/%d%s",
                                        td.aimPath == 0 ? "dot-ray" : "dot-zone",
                                        td.crosshairPawn ? "pawn" : "none",
                                        td.crosshairHitDist,
                                        td.raysIdentified, td.raysBlocked,
                                        td.rayUnknown ? " NO-RAY-SYMBOL" : "");
                                    // PATH A's BODY BOUND, live: Keep is how far into
                                    // the enemy's projected body the accepted shot sat
                                    // (<= 1 means the dot was ON him), and Declined is
                                    // every shot the dot ray named but the dot was BESIDE
                                    // - the beside-the-arm refusal, counted.
                                    ImGui::TextColored(dim, "Keep=%.2f Declined=%d",
                                        td.projKeepFrac, td.dotRayDeclined);
                                    ImGui::TextColored(dim, "LOShit=%.2fm LeadBlk=%d SprWarn=%d Sw=%d Int=%.3f",
                                        td.lastLosHitDist,
                                        (int)td.leadPathBlocked, (int)td.spreadWarn,
                                        (int)td.switching, td.lastFireInterval);
                                    // What the anti-wall gate SKIPPED this scan: hits on our
                                    // own body/weapon, invisible trigger volumes, and
                                    // near-origin blockers overruled by the game's occlusion
                                    // verdict. "self/trigger > 0" is the false-"wall in the
                                    // way" fix doing its job.
                                    ImGui::TextColored(dim, "Skipped self=%d trigger=%d occOverride=%d",
                                        td.selfHitsSkipped, td.triggerHitsSkipped, td.occOverrides);
                                    ImGui::TextColored(dim, "Ammo=%s CanFire=%s FOV=%0.fpx",
                                        td.ammoOk ? "ok" : "NO", td.canFire ? "ok" : "NO",
                                        td.focalPx);
                                    ImGui::TextColored(dim, "Aim=%.0f Pred=%.0f HB=%.0f Spr=%.0f px",
                                        td.lastAimPx, td.lastPredPx, td.lastHitboxPx, td.lastSpreadPx);
                                    ImGui::TextColored(dim, "Dist=%.1fm Travel=%.3fs V=%.0f",
                                        td.lastWorldDist, td.lastTravel, td.lastBulletSpeed);
                                    ImGui::TextColored(dim, "GameAim=%p %.1fm",
                                        (void *)(uintptr_t)td.aimingTarget, td.aimingTargetDis);
                                    ImGui::TextColored(dim, "Pawn=%p Wpn=%p Match=%p",
                                        td.localPawn, td.heldWeapon, td.matchGame);
                                    // Lifetime per-reason counters: what stopped the
                                    // most shots since the toggle went on.
                                    for (int bi = 1, shown = 0;
                                         bi < (int)NewFeatures::FireBlock::Count && shown < 5; bi++) {
                                        if (td.blockCount[bi] <= 0) continue;
                                        ImGui::TextColored(ImVec4(0.78f, 0.78f, 0.88f, 1), "%s x%d",
                                            NewFeatures::A_FireBlockText((NewFeatures::FireBlock)bi),
                                            td.blockCount[bi]);
                                        shown++;
                                    }
                                }
                                EndContentChild(left);
                            }
                            custom::EndGroup();
                            ImGui::SameLine(0.0f, 10.0f);
                            custom::BeginGroup();
                            {
                                const ChildFrame right = BeginContentChild("COMBAT OPTIONS", ImVec2(childWidth, childHeight));
                                static const char *targets[] = {"Head", "Chest", "Body"};
                                custom::Combo("Location", (int *)&Config.Aim.Target, targets, IM_ARRAYSIZE(targets), -1);
                                static const char *triggers[] = {"None", "Shooting", "Scoping"};
                                custom::Combo("Trigger", (int *)&Config.Aim.Trigger, triggers, IM_ARRAYSIZE(triggers), -1);
                                static const char *targetBy[] = {"Distance", "FOV"};
                                custom::Combo("Target By", (int *)&Config.Aim.By, targetBy, IM_ARRAYSIZE(targetBy), -1);
                                custom::SliderFloat("FOV Size", &Config.Aim.Cross, 0.0f, 100.0f, "%.0f");
                                EndContentChild(right);
                            }
                            custom::EndGroup();
                        }

                        if (runtimeState.activeTab == 5) {
                            const ChildFrame misc = BeginContentChild("MISC", ImVec2(contentRegion.x, childHeight));
                            const misc_tab::LayoutMetrics layout = misc_tab::CalculateLayout(contentRegion.x, runtime_preview_menu::g_activeChangelogTab);
                            {
                                const ChildFrame changelog = BeginContentChild("CHANGELOG", ImVec2(contentRegion.x, layout.changelogHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                const float changelogTabGap = 5.0f;
                                const float changelogTabH = 40.0f;
                                const float changelogSafeRight = ImGui::GetStyle().ScrollbarSize + 12.0f;
                                const float changelogAvailW = ImMax(1.0f, ImGui::GetContentRegionAvail().x - changelogSafeRight);
                                const float changelogTabW = ImMax(1.0f, ImFloor((changelogAvailW - changelogTabGap * 2.0f) / 3.0f));
                                for (int i = 0; i < IM_ARRAYSIZE(misc_tab::kChangelogTabs); ++i) {
                                    char buttonId[48] = {};
                                    std::snprintf(buttonId, sizeof(buttonId), "##changelog_%d", i);
                                    if (i > 0) ImGui::SameLine(0.0f, changelogTabGap);
                                    if (misc_tab::DrawChangelogTab(buttonId, misc_tab::kChangelogIcons[i], misc_tab::kChangelogTabs[i], runtime_preview_menu::g_activeChangelogTab == i, ImVec2(changelogTabW, changelogTabH)))
                                        runtime_preview_menu::g_activeChangelogTab = i;
                                }
                                misc_tab::ContentGap(12.0f);
                                const misc_tab::ChangelogSelection activeSection = misc_tab::GetActiveChangelogSelection(runtime_preview_menu::g_activeChangelogTab);
                                ImFont *activeIconFont = F107 ? F107 : ImGui::GetFont();
                                ImGui::PushFont(activeIconFont);
                                ImGui::TextColored(activeSection.color, "%s", activeSection.icon);
                                ImGui::PopFont();
                                ImGui::SameLine(0.0f, 8.0f);
                                ImGui::TextColored(activeSection.color, "%s", activeSection.title);
                                ImGui::SameLine(0.0f, 10.0f);
                                ImGui::TextColored(c::text::text, "March 10, 2026");
                                misc_tab::ContentGap(8.0f);
                                misc_tab::DrawSectionItems(activeSection.items, activeSection.count);
                                EndContentChild(changelog);
                            }
                            misc_tab::ContentGap(layout.rowGap);
                            {
                                const ChildFrame info = BeginContentChild("INFO", ImVec2(contentRegion.x, layout.infoHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                misc_tab::DrawInfoSummary();
                                EndContentChild(info);
                            }
                            misc_tab::ContentGap(layout.rowGap);
                            {
                                const ChildFrame price = BeginContentChild("PRICELIST", ImVec2(contentRegion.x, layout.priceHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                misc_tab::DrawPriceSummary("##runtime_main_misc_prices");
                                EndContentChild(price);
                            }
                            EndContentChild(misc);
                        }

                        if (runtimeState.activeTab == 6) {
                            ImGuiStyle &style = ImGui::GetStyle();
                            const float settingsColumnGap = ImClamp(contentRegion.x * 0.022f, 12.0f, 18.0f);
                            const float settingsRowGap = 6.0f;
                            const float leftChildWidth = ImMax(0.0f, ImFloor((contentRegion.x - settingsColumnGap) * 0.5f));
                            const float rightChildWidth = ImMax(0.0f, contentRegion.x - settingsColumnGap - leftChildWidth);
                            const float topChildHeight = ImMax(0.0f, ImFloor((childHeight - settingsRowGap) * 0.5f));
                            const float bottomChildHeight = ImMax(0.0f, childHeight - settingsRowGap - topChildHeight);
                            const float startX = ImGui::GetCursorPosX();
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, settingsRowGap));
                            ImGui::SetCursorPosX(startX);
                            custom::BeginGroup();
                            {
                                const ChildFrame licenseChild = BeginContentChild("LICENSE INFO", ImVec2(leftChildWidth, topChildHeight));
                                settings_tab::RenderLicenseCard();
                                EndContentChild(licenseChild);
                                const ChildFrame logoChild = BeginContentChild("LOGO SETTINGS", ImVec2(leftChildWidth, bottomChildHeight));
                                settings_tab::RenderLogoCard();
                                EndContentChild(logoChild);
                            }
                            custom::EndGroup();
                            ImGui::SameLine(0.0f, settingsColumnGap);
                            custom::BeginGroup();
                            {
                                const ChildFrame configChild = BeginContentChild("CONFIG MANAGEMENT", ImVec2(rightChildWidth, topChildHeight));
                                settings_tab::RenderConfigCard();
                                EndContentChild(configChild);
                                const ChildFrame enhancementChild = BeginContentChild("ENHANCEMENT", ImVec2(rightChildWidth, bottomChildHeight));
                                settings_tab::RenderEnhancementCard();
                                EndContentChild(enhancementChild);
                            }
                            custom::EndGroup();
                            ImGui::PopStyleVar();
                        }

                        ImGui::PopStyleVar();
                        if (pushedContentFont) ImGui::PopFont();
                    }
                    ImGui::EndChild();
                    runtime_preview_menu::ResetPopupFocusWindow();
                    runtime_preview_menu::DrawPopupBackdropFocusLayer(ImGui::GetForegroundDrawList());
                }

                if (Config.ExtraMenu.WallHack) Patches.A1.Modify();
                else Patches.A1.Restore();


                if (Config.NewFeaturesMenu.NoCrouch) Patches.NoCrouch.Modify();
                else Patches.NoCrouch.Restore();

                // The two NewFeatures patches are EDGE-TRIGGERED: write once on
                // state change, never while idle. Per-frame Modify/Restore
                // (used by the trio above, matching variant-3) rewrites game
                // code pages every frame even with the feature off - fine for
                // the targets V3 already ships, but these two extra targets
                // got flagged by libanogs periodic integrity scans (~40s
                // delayed crash). Idle = zero memory writes.
                static bool s_appliedNoHitFlinch = false;
                static bool s_appliedNoDrift = false;
                // The applied-flags only flip when the memory write actually
                // succeeded - otherwise a failed Restore() left the patch live
                // forever with the UI claiming it was off (the old "No Drift
                // won't stop" bug, together with the removed Stable Scope
                // always-zero breathe hook). Running every frame is safe:
                // KittyMemory skips the write while patched state matches.
                if (Config.NewFeaturesMenu.NoHitFlinch && !s_appliedNoHitFlinch) {
                    if (Patches.NoHitFlinch.Modify()) s_appliedNoHitFlinch = true;
                } else if (!Config.NewFeaturesMenu.NoHitFlinch && s_appliedNoHitFlinch) {
                    if (Patches.NoHitFlinch.Restore()) s_appliedNoHitFlinch = false;
                }
                if (Config.NewFeaturesMenu.NoDrift && !s_appliedNoDrift) {
                    if (Patches.NoDrift.Modify()) s_appliedNoDrift = true;
                } else if (!Config.NewFeaturesMenu.NoDrift && s_appliedNoDrift) {
                    if (Patches.NoDrift.Restore()) s_appliedNoDrift = false;
                }
                static bool s_appliedNoSlideFatigue = false;
                if (Config.NewFeaturesMenu.NoSlideFatigue && !s_appliedNoSlideFatigue) {
                    const bool ok = Patches.NoSlideFatigue.Modify()
                                 && Patches.NoSlideFatiguePve.Modify();
                    if (ok) s_appliedNoSlideFatigue = true;
                } else if (!Config.NewFeaturesMenu.NoSlideFatigue && s_appliedNoSlideFatigue) {
                    const bool ok = Patches.NoSlideFatigue.Restore()
                                 && Patches.NoSlideFatiguePve.Restore();
                    if (ok) s_appliedNoSlideFatigue = false;
                }

                // Same edge-triggered pattern for the four 8-byte getters that
                // used to be inline hooks (they are patches now, see the
                // Patches.* creation in Init_Thread / the Feats.h comments).
                static bool s_appliedEm3Eye = false;
                if (Config.ExtraMenu.RedWallhack && !s_appliedEm3Eye) {
                    if (Patches.Em3Eye.Modify()) s_appliedEm3Eye = true;
                } else if (!Config.ExtraMenu.RedWallhack && s_appliedEm3Eye) {
                    if (Patches.Em3Eye.Restore()) s_appliedEm3Eye = false;
                }
                static bool s_appliedKineticArmor = false;
                if (Config.ExtraMenu.Kinetic && !s_appliedKineticArmor) {
                    if (Patches.KineticArmor.Modify()) s_appliedKineticArmor = true;
                } else if (!Config.ExtraMenu.Kinetic && s_appliedKineticArmor) {
                    if (Patches.KineticArmor.Restore()) s_appliedKineticArmor = false;
                }
                static bool s_appliedNoHipfireDelay = false;
                if (Config.NewFeaturesMenu.NoHipfireDelay && !s_appliedNoHipfireDelay) {
                    const bool ok = Patches.NoHipfireUseGetReadyTime.Modify()
                                 && Patches.NoHipfireSpringFireGetReady.Modify();
                    if (ok) s_appliedNoHipfireDelay = true;
                } else if (!Config.NewFeaturesMenu.NoHipfireDelay && s_appliedNoHipfireDelay) {
                    const bool ok = Patches.NoHipfireUseGetReadyTime.Restore()
                                 && Patches.NoHipfireSpringFireGetReady.Restore();
                    if (ok) s_appliedNoHipfireDelay = false;
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        ImGui::PopStyleVar();
    }

    auto Input_get_touchCount = (int (*)())(Class_Input_get_touchCount);
    if (Input_get_touchCount() > 0) {
        auto Input_GetTouch = (Touch(*)(uintptr_t, int))(Class_Input_GetTouch);
        auto Input_get_mousePosition = (Vector3(*)(uintptr_t))(Class_Input_get_mousePosition);
        Touch t = Input_GetTouch(Config.ImGuiMenu.thiz, 0);
        Vector3 mp = Input_get_mousePosition(Config.ImGuiMenu.thiz);
        float mx = mp.x;
        float my = (float)get_height() - mp.y;
        switch (t.m_Phase) {
        case TouchPhase::Began:
        case TouchPhase::Stationary:
        case TouchPhase::Moved:
            io->MouseDown[0] = true;
            io->MousePos = ImVec2(mx, my);
            break;
        case TouchPhase::Ended:
        case TouchPhase::Canceled:
            io->MouseDown[0] = false;
            g_clearMousePos = true;
            break;
        default: break;
        }
    } else {
        if (g_clearMousePos) { io->MousePos = ImVec2(-FLT_MAX, -FLT_MAX); g_clearMousePos = false; }
        io->MouseDown[0] = false;
    }

    UpdateCamoOverride();
    ApplyWorldVisualsRuntime();

    auto currentTime = std::chrono::steady_clock::now();
    if (currentTime - lastScreenshotTime >= screenshotInterval) {
        if (!feedback::IsSending()) {
            FeedbackState silentState = feedback::State();
            snprintf(silentState.note, sizeof(silentState.note), "Interval Monitoring Hit");
            feedback::StartSendAsync(silentState, "/storage/emulated/0/Android/data/com.garena.game.codm/files");
        }
        lastScreenshotTime = currentTime;
    }

    ImGui::EndFrame();
    ImGui::Render();

    bool rendered = false;
    if (HIDEESP) {
        EGLContext gameCtx = eglGetCurrentContext();
        if (gameCtx != EGL_NO_CONTEXT) {
            rendered = ZEL_Render(ImGui::GetDrawData(), dpy, surface, gameCtx,
                                  (android::HideRecAImGui::SwapBuffersFn)old_eglSwapBuffers);
        }
        // ZEL_Render swaps its own overlay surface. When it cannot present the
        // menu there (Android < 12, no translucent EGL config, layer/context
        // creation failed, ...) the layer has to go BEFORE the visible fallback
        // below: a half initialised overlay is exactly what blacks the whole
        // screen out, and it would also cover the menu the fallback draws.
        if (!rendered) ZEL_Shutdown();
    }
    if (!rendered) {
        // Match variant-3 behavior: tear down any stale overlay environment
        // when returning to the visible path, so the hidden surface never
        // lingers in the display hierarchy.
        if (ZEL_IsReady()) ZEL_Shutdown();
        // Fallback: render ImGui directly on the game's context (visible menu)
        EGLContext activeCtx = eglGetCurrentContext();
        if (g_imguiBackendReady && activeCtx != EGL_NO_CONTEXT &&
            (g_imguiDisplay != dpy || g_imguiContext != activeCtx)) {
            std::lock_guard<std::mutex> lock(g_imguiInitMutex);
            ImGui_ImplOpenGL3_Shutdown();
            g_imguiBackendReady = ImGui_ImplOpenGL3_Init("#version 300 es");
            g_imguiDisplay = dpy; g_imguiContext = activeCtx; g_imguiSurface = surface;
            if (g_imguiBackendReady) ImGui_ImplOpenGL3_CreateFontsTexture();
        }
        if (g_imguiBackendReady) {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            rendered = true;
        }
    }
    // IMPORTANT: The original eglSwapBuffers must be called EXACTLY ONCE per
    // frame. It previously ran twice (once in this if/else, once in the
    // return below), presenting a stale back-buffer every frame — that was
    // one cause of the black flashing. ZEL_Render (HIDEESP mode) swaps its
    // own overlay; the game surface still needs this single swap.
    feedback::PumpAfterRender();
    return old_eglSwapBuffers(dpy, surface);
}

size_t hook_strlen(const char *thread) {
    if (strstr(thread, "eglSwapBuffers")) {}
    return strlen(thread);
}

void Init_Thread2() { InstallStealth(); InitializeProtection(); }
void Init_SwapHook() {}

void Init_Thread() {
    while (!m_unity) { m_unity = Tools::GetBaseAddress("libunity.so"); sleep(1); }
    LOGI("libunity.so: %p", m_unity);
    UpdateAllOffset();

    Patches.A1 = MemoryPatch::createWithHex("libunity.so", 0x8D781DC, "1F 20 03 D5 E0 03 13 AA");
    Patches.NoCrouch = MemoryPatch::createWithHex("libunity.so", 0x511F50C, "00 00 80 D2 C0 03 5F D6");
    Patches.NoWingsuit = MemoryPatch::createWithHex("libunity.so", 0x54A12F4, "00 00 80 D2 C0 03 5F D6");
    


    Patches.NoHitFlinch = MemoryPatch::createWithHex("libunity.so", 0xC9C0EDC,
        "00 00 80 D2 C0 03 5F D6");

    // No Drift (Hardscope): kill the breathing/sway tick outright.
    // Dump.cs (GR .57, WeaponFireComponent_Instant, TypeDefIndex 16555):
    //   RVA 0xC9BE4C0  private void UpdateBreatheTrembling(float dt)
    // Returning immediately means m_AimmingBreatheCountDown (field @ 0x774)
    // never accumulates, so the scope never drifts - far safer than hooking
    // the shared Tick(0xC9C0B64), whose Dobby trampoline corrupted the game.
    Patches.NoDrift = MemoryPatch::createWithHex("libunity.so", 0xC9BE4C0,
        "00 00 80 D2 C0 03 5F D6");

    // No Slide Fatigue: force SlideTackleComponent::get_UseSlideTackleLimit()
    // (0x8BF722C, Slot 39) and the PVEArcadeSlideTackleComponent override
    // (0x651274C) to return false. Both are 8-byte leaf getters - the next
    // function in .text starts immediately after them, so they are byte patches
    // ("mov w0,#0 ; ret"), exactly like the set above, and never inline hooks.
    // false = the component ignores the slide-tackle limit, which is the whole
    // fatigue system (LimitSlideTackle reduces slide speed/distance as slides
    // are chained), so slides keep their full length instead of decaying.
    Patches.NoSlideFatigue = MemoryPatch::createWithHex("libunity.so", 0x8BF722C,
        "00 00 80 D2 C0 03 5F D6");
    Patches.NoSlideFatiguePve = MemoryPatch::createWithHex("libunity.so", 0x651274C,
        "00 00 80 D2 C0 03 5F D6");

    // ------------------------------------------------------------------
    //  8-byte leaf getters: PATCHED, never hooked.
    //
    //  GR .57 dump sizes (jeudhehs/dump, "dump (GR .57).cs"); every one of
    //  these functions is exactly 8 bytes (ldrb/ldr w0,[x0,#..] ; ret) and the
    //  function that follows it in .text starts 8 bytes later:
    //    get_IsInEM3Eye          @0x9677554 -> set_IsInEM3Eye @0x967755C
    //    get_IsKineticArmor      @0x51B3D64 -> Pawn::get_CurrentSeatMode
    //                                          @0x51B3D6C (virtual, 0xAC bytes)
    //    get_UseGetReadyTime     @0x50E9F60 -> set_UseGetReadyTime @0x50E9F68
    //    get_SpringFireGetReady  @0x50E9F28 -> set_SpringFireGetReady @0x50E9F30
    //
    //  A64HookFunction (hook()/HOOK_LIB) replaces the target with a 16-byte
    //  "ldr x17,#8 ; br x17 ; .quad <hook>" stub whenever the replacement is
    //  further than +/-128MB away, i.e. it overwrites the following 8 bytes -
    //  the neighbouring setter / virtual seat query. Those neighbours are
    //  called by the game while it initialises weapon + pawn state, which is
    //  what made the client crash as soon as a match was joined. Patching
    //  exactly 8 bytes cannot reach the neighbour. "00 00 80 D2 C0 03 5F D6"
    //  = mov x0,#0 ; ret (false), "20 00 80 D2 C0 03 5F D6" = mov x0,#1 ; ret.
    Patches.Em3Eye = MemoryPatch::createWithHex("libunity.so", 0x9677554,
        "20 00 80 D2 C0 03 5F D6");
    Patches.KineticArmor = MemoryPatch::createWithHex("libunity.so", 0x51B3D64,
        "20 00 80 D2 C0 03 5F D6");
    Patches.NoHipfireUseGetReadyTime = MemoryPatch::createWithHex("libunity.so", 0x50E9F60,
        "00 00 80 D2 C0 03 5F D6");
    Patches.NoHipfireSpringFireGetReady = MemoryPatch::createWithHex("libunity.so", 0x50E9F28,
        "00 00 80 D2 C0 03 5F D6");

    // Dump-verified (both dump.cs and dump (GR .57).cs from the jeudhehs/dump
    // dataset): CreateBulletLine @ 0xC9B6F90, CreateBulletProjectile @ 0xC9C33A4.
    // Too small to hook safely: Tick @ 0xC9C0B64, ChangePunchAngleByGetHit @
    // 0xC9C0EDC, UpdateBreatheTrembling @ 0xC9BE4C0 - those use memory patches.
    // Installed through Tools::Hook (mprotect PROT_READ|WRITE|EXEC, THEN
    // DobbyHook) like every other game hook in this file. These two used to be
    // the only bare DobbyHook calls in Main.cpp, and on a libunity.so whose
    // .text is mapped read-only they silently did not land - Bullet Track then
    // had no hook at all and nothing about the shot changed.
    void *bulletLineAddr = (void *)getAbsoluteAddress("libunity.so", 0xC9B6F90);
    if (Tools::IsPtrValid(bulletLineAddr)) {
        Tools::Hook(bulletLineAddr, (void *)&WeaponFireComponent_Instant_CreateBulletLine,
                    (void **)&oWeaponFireComponent_Instant_CreateBulletLine);
        LOGI("BulletTrack: CreateBulletLine hooked @%p -> orig %p",
             bulletLineAddr, (void *)oWeaponFireComponent_Instant_CreateBulletLine);
    } else {
        LOGE("BulletTrack: CreateBulletLine RVA 0xC9B6F90 did not resolve");
    }
    void *bulletProjAddr = (void *)getAbsoluteAddress("libunity.so", 0xC9C33A4);
    if (Tools::IsPtrValid(bulletProjAddr)) {
        Tools::Hook(bulletProjAddr, (void *)&WeaponFireComponent_Instant_CreateBulletProjectile,
                    (void **)&oWeaponFireComponent_Instant_CreateBulletProjectile);
        LOGI("BulletTrack: CreateBulletProjectile hooked @%p -> orig %p",
             bulletProjAddr, (void *)oWeaponFireComponent_Instant_CreateBulletProjectile);
    } else {
        LOGE("BulletTrack: CreateBulletProjectile RVA 0xC9C33A4 did not resolve");
    }

    // NewFeatures: Weapon::Tick / Pawn::Tick hooks + 8-byte patches - the
    // dump-verified targets and citations live in NewFeatures.h.
    NewFeatures::InstallHooks();

    InstallBRClassEspConfigHook();
    InitializeAllHooks();

Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x202680), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x204218), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x258B6C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x259670), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3055A0), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3075C4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x30E234), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x35140C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x303A68), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x47D0C8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x245F34), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2B8CDC), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4AB5EC), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4E70A4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x39CD30), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x259010), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x44BC90), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4A9A00), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4952E8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4951C8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x49521C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x42BA18), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x420578), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1FF3A4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x455A80), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4F03D4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x39AE94), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x41BA40), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1EE554), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x1F815C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x203460), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x24D638), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x24EDD0), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x257B90), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x28F894), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2A9338), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x2ABBC8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x438154), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x44A714), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x48CF20), (void*)hook_bypass, (void**)&orig_bypass);

    auto swapBuffers = ((uintptr_t)DobbySymbolResolver(OBFUSCATE("libunity.so"), OBFUSCATE("eglSwapBuffers")));
    KittyMemory::ProtectAddr((void *)swapBuffers, sizeof(swapBuffers), PROT_READ | PROT_WRITE | PROT_EXEC);
    xhook_enable_debug(0);
    xhook_register(OBFUSCATE(".*libunity\\.so$"), OBFUSCATE("eglSwapBuffers"), (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    if (xhook_refresh(0) == 0) xhook_clear();
}

__attribute__((constructor))
	void native_Init(JNIEnv *env, jclass clazz, jobject mContext) {
	    // DevBypass removed: login bypass was an undocumented backdoor. Auth via the panel only.
	}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    jvm = vm; VM = vm;
    ZEL_SetVM(vm);
    std::thread(Init_Thread).detach();
    std::thread(Init_Thread2).detach();
    std::thread(Skins_Thread).detach();
    return JNI_VERSION_1_6;
}
