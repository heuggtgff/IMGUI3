#pragma once

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
    {
        Rotation0   = 0,
        Rotation90  = 1,
        Rotation180 = 2,
        Rotation270 = 3
    };

    template <typename enum_t, typename = std::enable_if_t<std::is_enum_v<enum_t>>>
    constexpr enum_t operator|(enum_t lhs, enum_t rhs)
    {
        using underlying_t = std::underlying_type_t<enum_t>;
        return static_cast<enum_t>(static_cast<underlying_t>(lhs) | static_cast<underlying_t>(rhs));
    }

    template <typename enum_t, typename = std::enable_if_t<std::is_enum_v<enum_t>>>
    constexpr enum_t operator|=(enum_t &lhs, enum_t rhs)
    {
        return (lhs = lhs | rhs);
    }

} // namespace android::anwcreator::detail::types

namespace android::anwcreator::detail::jni
{

    struct JNIEnvironment
    {
        JNIEnv *env;
        JavaVM *vm;

        JNIEnvironment(JavaVM *javaVM) : env(nullptr), vm(javaVM)
        {
            if (vm)
                vm->AttachCurrentThread(&env, nullptr);
        }

        ~JNIEnvironment() {}

        inline bool IsValid() const
        {
            return env != nullptr;
        }

        inline operator JNIEnv *() const
        {
            return env;
        }

        inline JNIEnv *operator->() const
        {
            return env;
        }

        inline bool CheckException(const char * /*context*/ = nullptr)
        {
            if (!env || !env->ExceptionCheck())
                return false;

            env->ExceptionDescribe();
            env->ExceptionClear();
            return true;
        }
    };

    struct LocalRef
    {
        JNIEnv *env;
        jobject obj;

        LocalRef(JNIEnv *e, jobject o) : env(e), obj(o) {}

        ~LocalRef()
        {
            if (env && obj)
                env->DeleteLocalRef(obj);
        }

        LocalRef(const LocalRef &) = delete;
        LocalRef &operator=(const LocalRef &) = delete;

        LocalRef(LocalRef &&other) noexcept : env(other.env), obj(other.obj)
        {
            other.obj = nullptr;
        }

        inline operator jobject() const { return obj; }
        inline operator jclass() const { return reinterpret_cast<jclass>(obj); }
        inline jobject get() const { return obj; }
        inline jclass getClass() const { return reinterpret_cast<jclass>(obj); }
        inline bool IsValid() const { return obj != nullptr; }
        inline explicit operator bool() const { return obj != nullptr; }
    };

    struct GlobalRef
    {
        JNIEnv *env;
        jobject obj;

        GlobalRef(JNIEnv *e, jobject o) : env(e), obj(nullptr)
        {
            if (e && o)
                obj = e->NewGlobalRef(o);
        }

        ~GlobalRef()
        {
            Release();
        }

        void Release()
        {
            if (env && obj)
            {
                env->DeleteGlobalRef(obj);
                obj = nullptr;
            }
        }

        GlobalRef(const GlobalRef &) = delete;
        GlobalRef &operator=(const GlobalRef &) = delete;

        GlobalRef(GlobalRef &&other) noexcept : env(other.env), obj(other.obj)
        {
            other.obj = nullptr;
        }

        inline operator jobject() const { return obj; }
        inline jobject get() const { return obj; }
        inline bool IsValid() const { return obj != nullptr; }
        inline explicit operator bool() const { return obj != nullptr; }
    };

} // namespace android::anwcreator::detail::jni to

namespace android::anwcreator::detail::framework
{

    struct DisplayMetrics
    {
        int32_t widthPixels;
        int32_t heightPixels;
        float   density;
        int32_t densityDpi;
    };

    struct DisplayInfo
    {
        int32_t                    width;
        int32_t                    height;
        types::DisplayRotation     rotation;
        float                      refreshRate;
    };

    class Display
    {
    public:
        static DisplayInfo GetDisplayInfo(JNIEnv *env, ANativeActivity *activity)
        {
            DisplayInfo info{};

            if (!env || !activity || !activity->clazz)
                return info;

            jni::LocalRef activityCls(env, env->GetObjectClass(activity->clazz));
            if (!activityCls)
                return info;

            jmethodID getWindowManager =
                env->GetMethodID(activityCls, "getWindowManager", "()Landroid/view/WindowManager;");
            if (!getWindowManager)
                return info;

            jni::LocalRef windowManager(env,
                                        env->CallObjectMethod(activity->clazz, getWindowManager));
            if (!windowManager || jni::JNIEnvironment(activity->vm).CheckException("Activity.getWindowManager()"))
            {
                return info;
            }

            jni::LocalRef windowManagerCls(env, env->GetObjectClass(windowManager));
            jmethodID getDefaultDisplay =
                env->GetMethodID(windowManagerCls, "getDefaultDisplay", "()Landroid/view/Display;");
            if (!getDefaultDisplay)
                return info;

            jni::LocalRef display(env,
                                  env->CallObjectMethod(windowManager, getDefaultDisplay));
            if (!display || jni::JNIEnvironment(activity->vm).CheckException("WindowManager.getDefaultDisplay()"))
            {
                return info;
            }

            jni::LocalRef displayMetricsCls(env, env->FindClass("android/util/DisplayMetrics"));
            jmethodID displayMetricsCtor =
                env->GetMethodID(displayMetricsCls, "<init>", "()V");
            jni::LocalRef displayMetrics(env,
                                         env->NewObject(displayMetricsCls, displayMetricsCtor));

            jni::LocalRef displayCls(env, env->GetObjectClass(display));
            jmethodID getRealMetrics =
                env->GetMethodID(displayCls, "getRealMetrics", "(Landroid/util/DisplayMetrics;)V");
            env->CallVoidMethod(display, getRealMetrics, displayMetrics.get());

            jfieldID widthPixelsField  = env->GetFieldID(displayMetricsCls, "widthPixels", "I");
            jfieldID heightPixelsField = env->GetFieldID(displayMetricsCls, "heightPixels", "I");

            info.width  = env->GetIntField(displayMetrics, widthPixelsField);
            info.height = env->GetIntField(displayMetrics, heightPixelsField);

            jmethodID getRotation = env->GetMethodID(displayCls, "getRotation", "()I");
            int32_t   rotation    = env->CallIntMethod(display, getRotation);
            info.rotation         = static_cast<types::DisplayRotation>(rotation);

            jmethodID getRefreshRate =
                env->GetMethodID(displayCls, "getRefreshRate", "()F");
            if (getRefreshRate)
            {
                info.refreshRate = env->CallFloatMethod(display, getRefreshRate);
            }

            return info;
        }
    };

    class SurfaceControl
    {
    public:
        struct Builder
        {
            JNIEnv *env;
            jobject builder;
            jclass  builderClass;

            Builder(JNIEnv *e) : env(e), builder(nullptr), builderClass(nullptr)
            {
                builderClass = env->FindClass("android/view/SurfaceControl$Builder");
                if (!builderClass)
                    return;

                jmethodID ctor = env->GetMethodID(builderClass, "<init>", "()V");
                builder        = env->NewObject(builderClass, ctor);
            }

            ~Builder()
            {
                if (env && builder)
                    env->DeleteLocalRef(builder);
                if (env && builderClass)
                    env->DeleteLocalRef(builderClass);
            }

            Builder &SetName(const char *name)
            {
                if (!builder)
                    return *this;

                jmethodID setName = env->GetMethodID(
                    builderClass,
                    "setName",
                    "(Ljava/lang/String;)Landroid/view/SurfaceControl$Builder;");
                if (setName)
                {
                    jni::LocalRef jName(env, env->NewStringUTF(name));
                    builder = env->CallObjectMethod(builder, setName, jName.get());
                }
                return *this;
            }

            Builder &SetParent(jobject parent)
            {
                if (!builder || !parent)
                    return *this;

                jmethodID setParent = env->GetMethodID(
                    builderClass,
                    "setParent",
                    "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Builder;");
                if (setParent)
                {
                    builder = env->CallObjectMethod(builder, setParent, parent);
                }
                return *this;
            }

            Builder &SetBufferSize(int32_t width, int32_t height)
            {
                if (!builder)
                    return *this;

                jmethodID setBufferSize = env->GetMethodID(
                    builderClass,
                    "setBufferSize",
                    "(II)Landroid/view/SurfaceControl$Builder;");
                if (setBufferSize)
                {
                    builder = env->CallObjectMethod(builder, setBufferSize, width, height);
                }
                return *this;
            }

            Builder &SetFormat(int32_t format)
            {
                if (!builder) return *this;
                jmethodID setFormat = env->GetMethodID(
                    builderClass,
                    "setFormat",
                    "(I)Landroid/view/SurfaceControl$Builder;");
                if (setFormat) builder = env->CallObjectMethod(builder, setFormat, (jint)format);
                return *this;
            }

            Builder &SetOpaque(bool opaque)
            {
                if (!builder) return *this;
                jmethodID setOpaque = env->GetMethodID(
                    builderClass,
                    "setOpaque",
                    "(Z)Landroid/view/SurfaceControl$Builder;");
                if (setOpaque) builder = env->CallObjectMethod(builder, setOpaque, (jboolean)opaque);
                return *this;
            }

            Builder &SetFlags(uint32_t flags, uint32_t mask)
            {
                if (!builder)
                    return *this;

                jmethodID setFlags = env->GetMethodID(
                    builderClass,
                    "setFlags",
                    "(II)Landroid/view/SurfaceControl$Builder;");
                if (setFlags)
                {
                    builder = env->CallObjectMethod(builder,
                                                    setFlags,
                                                    static_cast<jint>(flags),
                                                    static_cast<jint>(mask));
                }
                return *this;
            }

            Builder &SetSkipScreenshot(bool skip)
            {
                if (skip)
                {
                    constexpr uint32_t FLAG_SKIP_SCREENSHOT = 0x40;
                    SetFlags(FLAG_SKIP_SCREENSHOT, FLAG_SKIP_SCREENSHOT);
                }
                return *this;
            }

            jobject Build()
            {
                if (!builder)
                    return nullptr;

                jmethodID buildMethod = env->GetMethodID(
                    builderClass,
                    "build",
                    "()Landroid/view/SurfaceControl;");
                if (!buildMethod)
                    return nullptr;

                return env->CallObjectMethod(builder, buildMethod);
            }

            inline bool IsValid() const { return builder != nullptr; }
        };

        class Transaction
        {
        public:
            JNIEnv *env;
            jobject transaction;
            jclass  transactionClass;

            Transaction(JNIEnv *e) : env(e), transaction(nullptr), transactionClass(nullptr)
            {
                transactionClass = env->FindClass("android/view/SurfaceControl$Transaction");
                if (!transactionClass)
                    return;

                jmethodID ctor = env->GetMethodID(transactionClass, "<init>", "()V");
                transaction     = env->NewObject(transactionClass, ctor);
            }

            ~Transaction()
            {
                if (env && transaction)
                    env->DeleteLocalRef(transaction);
                if (env && transactionClass)
                    env->DeleteLocalRef(transactionClass);
            }

            Transaction &SetAlpha(jobject surfaceControl, float alpha)
            {
                if (!transaction || !surfaceControl)
                    return *this;

                jmethodID setAlpha = env->GetMethodID(
                    transactionClass,
                    "setAlpha",
                    "(Landroid/view/SurfaceControl;F)Landroid/view/SurfaceControl$Transaction;");
                if (setAlpha)
                {
                    transaction = env->CallObjectMethod(transaction,
                                                        setAlpha,
                                                        surfaceControl,
                                                        alpha);
                }
                return *this;
            }

            Transaction &SetLayer(jobject surfaceControl, int32_t z)
            {
                if (!transaction || !surfaceControl)
                    return *this;

                jmethodID setLayer = env->GetMethodID(
                    transactionClass,
                    "setLayer",
                    "(Landroid/view/SurfaceControl;I)Landroid/view/SurfaceControl$Transaction;");
                if (setLayer)
                {
                    transaction = env->CallObjectMethod(transaction,
                                                        setLayer,
                                                        surfaceControl,
                                                        z);
                }
                return *this;
            }

            Transaction &SetPosition(jobject surfaceControl, float x, float y)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID m = env->GetMethodID(transactionClass,"setPosition","(Landroid/view/SurfaceControl;FF)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction,m,surfaceControl,x,y);
                return *this;
            }

            Transaction &SetMatrix(jobject surfaceControl, float dsdx, float dtdx, float dtdy, float dsdy)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID m = env->GetMethodID(transactionClass,"setMatrix","(Landroid/view/SurfaceControl;FFFF)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction,m,surfaceControl,dsdx,dtdx,dtdy,dsdy);
                return *this;
            }

            Transaction &Show(jobject surfaceControl)
            {
                if (!transaction || !surfaceControl)
                    return *this;

                jmethodID show = env->GetMethodID(
                    transactionClass,
                    "show",
                    "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (show)
                {
                    transaction = env->CallObjectMethod(transaction, show, surfaceControl);
                }
                return *this;
            }

            Transaction &Hide(jobject surfaceControl)
            {
                if (!transaction || !surfaceControl)
                    return *this;

                jmethodID hide = env->GetMethodID(
                    transactionClass,
                    "hide",
                    "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (hide)
                {
                    transaction = env->CallObjectMethod(transaction, hide, surfaceControl);
                }
                return *this;
            }

            Transaction &SetParent(jobject child, jobject newParent)
            {
                if (!transaction || !child || !newParent)
                    return *this;

                jmethodID setParent = env->GetMethodID(
                    transactionClass,
                    "setParent",
                    "(Landroid/view/SurfaceControl;Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (setParent)
                {
                    transaction = env->CallObjectMethod(transaction,
                                                        setParent,
                                                        child,
                                                        newParent);
                }
                return *this;
            }

            Transaction &Remove(jobject surfaceControl)
            {
                if (!transaction || !surfaceControl)
                    return *this;

                jmethodID remove = env->GetMethodID(
                    transactionClass,
                    "remove",
                    "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (remove)
                {
                    transaction = env->CallObjectMethod(transaction, remove, surfaceControl);
                }
                return *this;
            }

            void Apply()
            {
                if (!transaction)
                    return;

                jmethodID apply = env->GetMethodID(transactionClass, "apply", "()V");
                if (apply)
                {
                    env->CallVoidMethod(transaction, apply);
                }
            }

            inline bool IsValid() const { return transaction != nullptr; }
        };

        static jobject GetParentSurfaceControl(JNIEnv *env, ANativeActivity *activity)
        {
            if (!env || !activity || !activity->clazz)
                return nullptr;

            jni::LocalRef activityCls(env, env->GetObjectClass(activity->clazz));
            jmethodID getWindow =
                env->GetMethodID(activityCls, "getWindow", "()Landroid/view/Window;");
            if (!getWindow)
                return nullptr;

            jni::LocalRef window(env,
                                 env->CallObjectMethod(activity->clazz, getWindow));
            if (!window)
                return nullptr;

            jni::LocalRef windowCls(env, env->GetObjectClass(window));
            jmethodID getDecorView =
                env->GetMethodID(windowCls, "getDecorView", "()Landroid/view/View;");
            if (!getDecorView)
                return nullptr;

            jni::LocalRef decorView(env,
                                    env->CallObjectMethod(window, getDecorView));
            if (!decorView)
                return nullptr;

            jni::LocalRef viewCls(env, env->GetObjectClass(decorView));
            jmethodID getViewRootImpl =
                env->GetMethodID(viewCls,
                                 "getViewRootImpl",
                                 "()Landroid/view/ViewRootImpl;");
            if (!getViewRootImpl)
                return nullptr;

            jni::LocalRef viewRootImpl(env,
                                       env->CallObjectMethod(decorView, getViewRootImpl));
            if (!viewRootImpl)
                return nullptr;

            jni::LocalRef viewRootImplCls(env, env->GetObjectClass(viewRootImpl));
            jmethodID getSurfaceControl =
                env->GetMethodID(viewRootImplCls,
                                 "getSurfaceControl",
                                 "()Landroid/view/SurfaceControl;");
            if (!getSurfaceControl)
                return nullptr;

            return env->CallObjectMethod(viewRootImpl, getSurfaceControl);
        }
    };

    class Surface
    {
    public:
        static jobject CreateFromSurfaceControl(JNIEnv *env, jobject surfaceControl)
        {
            if (!env || !surfaceControl)
                return nullptr;

            jni::LocalRef surfaceCls(env, env->FindClass("android/view/Surface"));
            if (!surfaceCls)
                return nullptr;

            jmethodID ctor =
                env->GetMethodID(surfaceCls, "<init>", "(Landroid/view/SurfaceControl;)V");
            if (!ctor)
                return nullptr;

            return env->NewObject(surfaceCls, ctor, surfaceControl);
        }
    };

} // namespace android::anwcreator::detail::framework to

namespace android::anwcreator::detail
{
    struct WindowContext
    {
        std::unique_ptr<jni::GlobalRef> surfaceControl;
        std::unique_ptr<jni::GlobalRef> surface;
        std::unique_ptr<jni::GlobalRef> parentSurfaceControl; // parent root
        ANativeWindow                  *nativeWindow;
        int32_t                         width;
        int32_t                         height;
        bool                            skipScreenshot;

        WindowContext()
            : surfaceControl(nullptr),
              surface(nullptr),
              parentSurfaceControl(nullptr),
              nativeWindow(nullptr),
              width(0),
              height(0),
              skipScreenshot(false)
        {
        }

        ~WindowContext()
        {
            Release();
        }

        void Release()
        {
            if (nativeWindow)
            {
                ANativeWindow_release(nativeWindow);
                nativeWindow = nullptr;
            }
            surface.reset();
            surfaceControl.reset();
            parentSurfaceControl.reset();
        }
    };

} // namespace android::anwcreator::detail end to

namespace android
{
    class ANwCreator
    {
    public:
        struct DisplayInfo
        {
            int32_t theta;
            int32_t width;
            int32_t height;
            float   refreshRate;

            DisplayInfo()
                : theta(0), width(0), height(0), refreshRate(60.0f)
            {
            }
        };

        struct CreateOptions
        {
            const char *name;
            int32_t     width;
            int32_t     height;
            bool        skipScreenshot;

            CreateOptions()
                : name(""),
                  width(-1),
                  height(-1),
                  skipScreenshot(false)
            {
            }
        };

    public:
        static DisplayInfo GetDisplayInfo(ANativeActivity *activity)
        {
            DisplayInfo result{};

            if (!activity || !activity->vm || !activity->clazz)
                return result;

            anwcreator::detail::jni::JNIEnvironment jniEnv(activity->vm);
            if (!jniEnv.IsValid())
                return result;

            auto displayInfo = anwcreator::detail::framework::Display::GetDisplayInfo(jniEnv,
                                                                                      activity);

            result.width       = displayInfo.width;
            result.height      = displayInfo.height;
            result.theta       = 90 * static_cast<int32_t>(displayInfo.rotation);
            result.refreshRate = displayInfo.refreshRate;

            return result;
        }

        static ANativeWindow *Create(ANativeActivity *activity,
                                     const CreateOptions &options = CreateOptions())
        {
            if (!activity || !activity->vm || !activity->clazz)
                return nullptr;

            anwcreator::detail::jni::JNIEnvironment jniEnv(activity->vm);
            if (!jniEnv.IsValid())
                return nullptr;

            int32_t width  = options.width;
            int32_t height = options.height;

            if (width <= 0 || height <= 0)
            {
                auto displayInfo = anwcreator::detail::framework::Display::GetDisplayInfo(jniEnv,
                                                                                          activity);
                width  = displayInfo.width;
                height = displayInfo.height;
            }

            jobject parentSC =
                anwcreator::detail::framework::SurfaceControl::GetParentSurfaceControl(jniEnv,
                                                                                       activity);
            if (!parentSC && jniEnv.CheckException("GetParentSurfaceControl"))
                return nullptr;

            anwcreator::detail::framework::SurfaceControl::Builder builder(jniEnv);
            if (!builder.IsValid())
                return nullptr;

            builder.SetName(options.name)
                .SetBufferSize(width, height)
                .SetFormat(1)       // pixil RGBA_8888
                .SetOpaque(false)   // alpha buffer
                .SetSkipScreenshot(options.skipScreenshot);

            if (parentSC)
                builder.SetParent(parentSC);

            jobject localSurfaceControl = builder.Build();
            if (!localSurfaceControl || jniEnv.CheckException("Builder.build()"))
            {
                if (parentSC)
                    jniEnv->DeleteLocalRef(parentSC);
                return nullptr;
            }

            auto context                = std::make_unique<anwcreator::detail::WindowContext>();
            context->width              = width;
            context->height             = height;
            context->skipScreenshot     = options.skipScreenshot;
            context->surfaceControl     =
                std::make_unique<anwcreator::detail::jni::GlobalRef>(jniEnv, localSurfaceControl);

            // Parent root surface store
            if (parentSC)
            {
                context->parentSurfaceControl =
                    std::make_unique<anwcreator::detail::jni::GlobalRef>(jniEnv, parentSC);
            }

            {
                anwcreator::detail::framework::SurfaceControl::Transaction transaction(jniEnv);
                if (transaction.IsValid())
                {
                    transaction.SetAlpha(context->surfaceControl->get(), 1.0f)
                        .SetLayer(context->surfaceControl->get(), 0x7FFFFFFE)
                        .SetPosition(context->surfaceControl->get(), 0.0f, 0.0f)
                        .SetMatrix(context->surfaceControl->get(), 1.0f, 0.0f, 0.0f, 1.0f)
                        .Show(context->surfaceControl->get())
                        .Apply();
                }
            }

            jobject localSurface =
                anwcreator::detail::framework::Surface::CreateFromSurfaceControl(
                    jniEnv,
                    context->surfaceControl->get());

            if (!localSurface || jniEnv.CheckException("Create Surface"))
            {
                jniEnv->DeleteLocalRef(localSurfaceControl);
                if (parentSC)
                    jniEnv->DeleteLocalRef(parentSC);
                return nullptr;
            }

            context->surface =
                std::make_unique<anwcreator::detail::jni::GlobalRef>(jniEnv, localSurface);

            context->nativeWindow = ANativeWindow_fromSurface(jniEnv,
                                                              context->surface->get());
            if (!context->nativeWindow)
            {
                jniEnv->DeleteLocalRef(localSurface);
                jniEnv->DeleteLocalRef(localSurfaceControl);
                if (parentSC)
                    jniEnv->DeleteLocalRef(parentSC);
                return nullptr;
            }

            ANativeWindow *result = context->nativeWindow;

            m_windowContexts.emplace(result, std::move(context));

            jniEnv->DeleteLocalRef(localSurface);
            jniEnv->DeleteLocalRef(localSurfaceControl);
            if (parentSC)
                jniEnv->DeleteLocalRef(parentSC);

            return result;
        }

        static void Destroy(ANativeActivity *activity, ANativeWindow *nativeWindow)
        {
            if (!nativeWindow)
                return;

            auto it = m_windowContexts.find(nativeWindow);
            if (it == m_windowContexts.end())
            {
                ANativeWindow_release(nativeWindow);
                return;
            }

            auto &context = it->second;

            if (activity && activity->vm && context->surfaceControl && context->surfaceControl->IsValid())
            {
                anwcreator::detail::jni::JNIEnvironment jniEnv(activity->vm);
                if (jniEnv.IsValid())
                {
                    anwcreator::detail::framework::SurfaceControl::Transaction transaction(jniEnv);
                    if (transaction.IsValid())
                        transaction.Remove(context->surfaceControl->get()).Apply();
                }
            }

            context->Release();
            m_windowContexts.erase(it);
        }

        static bool IsValid(ANativeWindow *nativeWindow)
        {
            return nativeWindow && m_windowContexts.count(nativeWindow) > 0;
        }

        static bool GetWindowSize(ANativeWindow *nativeWindow,
                                  int32_t *outWidth,
                                  int32_t *outHeight)
        {
            auto it = m_windowContexts.find(nativeWindow);
            if (it == m_windowContexts.end())
                return false;

            if (outWidth)
                *outWidth = it->second->width;
            if (outHeight)
                *outHeight = it->second->height;

            return true;
        }

        // Ensure the overlay surface is attached to the current root and visible
        static void EnsureVisible(ANativeActivity *activity, ANativeWindow *nativeWindow)
        {
            if (!activity || !activity->vm || !nativeWindow)
                return;

            auto it = m_windowContexts.find(nativeWindow);
            if (it == m_windowContexts.end())
                return;

            auto &context = it->second;
            if (!context || !context->surfaceControl || !context->surfaceControl->IsValid())
                return;

            anwcreator::detail::jni::JNIEnvironment jniEnv(activity->vm);
            if (!jniEnv.IsValid())
                return;

            anwcreator::detail::framework::SurfaceControl::Transaction transaction(jniEnv);
            if (!transaction.IsValid())
                return;

            // Get current View SurfaceControl
            jobject currentParent =
                anwcreator::detail::framework::SurfaceControl::GetParentSurfaceControl(
                    jniEnv,
                    activity);

            bool needReparent = false;

            if (currentParent && context->parentSurfaceControl && context->parentSurfaceControl->IsValid())
            {
                if (!jniEnv->IsSameObject(currentParent, context->parentSurfaceControl->get()))
                {
                    needReparent = true;
                }
            }
            else if (currentParent && !context->parentSurfaceControl)
            {
                needReparent = true;
            }

            if (needReparent && currentParent)
            {
                transaction.SetParent(context->surfaceControl->get(), currentParent);

                context->parentSurfaceControl =
                    std::make_unique<anwcreator::detail::jni::GlobalRef>(jniEnv, currentParent);
            }


            float scaleX = 1.0f, scaleY = 1.0f;
            auto displayInfo = anwcreator::detail::framework::Display::GetDisplayInfo(jniEnv, activity);
            if (context->width > 0 && context->height > 0 && displayInfo.width > 0 && displayInfo.height > 0)
            {
                int32_t targetW = displayInfo.width;
                int32_t targetH = displayInfo.height;
               
                const bool bufferLandscape = context->width >= context->height;
                const bool targetLandscape = targetW >= targetH;
                if (bufferLandscape != targetLandscape)
                    std::swap(targetW, targetH);
                scaleX = (float)targetW / (float)context->width;
                scaleY = (float)targetH / (float)context->height;
                if (!(scaleX > 0.0f) || !(scaleY > 0.0f) || scaleX > 4.0f || scaleY > 4.0f)
                    scaleX = scaleY = 1.0f;
            }

            transaction
                .SetAlpha(context->surfaceControl->get(), 1.0f)
                .SetLayer(context->surfaceControl->get(), 0x7FFFFFFE)
                .SetPosition(context->surfaceControl->get(), 0.0f, 0.0f)
                .SetMatrix(context->surfaceControl->get(), scaleX, 0.0f, 0.0f, scaleY)
                .Show(context->surfaceControl->get())
                .Apply();

            if (currentParent)
                jniEnv->DeleteLocalRef(currentParent);

        }

    private:
        inline static std::unordered_map<ANativeWindow *,
            std::unique_ptr<anwcreator::detail::WindowContext>> m_windowContexts;
    };

} // namespace android

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
    bool RenderDrawData(ImDrawData* drawData,EGLDisplay gameDisplay,EGLSurface gameSurface,
                        EGLContext gameContext,SwapBuffersFn swapBuffersFn);
    void Shutdown();
    constexpr operator bool() const{return m_state;}
private:
    bool EnsureEnvironment(EGLDisplay gameDisplay,EGLContext gameContext,int32_t width,int32_t height);
    bool InitEnvironment(EGLDisplay gameDisplay,EGLContext gameContext,int32_t width,int32_t height);
    void UnInitEnvironment();
    EGLConfig FindConfig(EGLDisplay display,EGLContext gameContext);
private:
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
HideRecAImGui::HideRecAImGui(const Options& options):m_options(options){}
HideRecAImGui::~HideRecAImGui(){UnInitEnvironment();}

EGLConfig HideRecAImGui::FindConfig(EGLDisplay display,EGLContext gameContext){
    (void)gameContext;
    const EGLint attrs[]={
        EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,0x00000040, // EGL_OPENGL_ES3_BIT_KHR
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,
        EGL_NONE
    };
    EGLConfig cfg=nullptr;
    EGLint count=0;
    if(eglChooseConfig(display,attrs,&cfg,1,&count)==EGL_TRUE&&count>0&&cfg)return cfg;

    EGLint total=0;
    if(eglGetConfigs(display,nullptr,0,&total)!=EGL_TRUE||total<=0)return nullptr;
    std::vector<EGLConfig> configs((size_t)total);
    if(eglGetConfigs(display,configs.data(),total,&total)!=EGL_TRUE)return nullptr;
    for(int i=0;i<total;i++){
        EGLint st=0,rt=0,r=0,g=0,b=0,a=0;
        eglGetConfigAttrib(display,configs[i],EGL_SURFACE_TYPE,&st);
        eglGetConfigAttrib(display,configs[i],EGL_RENDERABLE_TYPE,&rt);
        eglGetConfigAttrib(display,configs[i],EGL_RED_SIZE,&r);
        eglGetConfigAttrib(display,configs[i],EGL_GREEN_SIZE,&g);
        eglGetConfigAttrib(display,configs[i],EGL_BLUE_SIZE,&b);
        eglGetConfigAttrib(display,configs[i],EGL_ALPHA_SIZE,&a);
        if((st&EGL_WINDOW_BIT)&&(rt&0x00000040)&&r>=8&&g>=8&&b>=8&&a>=8)return configs[i];
    }
    return nullptr;
}

bool HideRecAImGui::InitEnvironment(EGLDisplay gameDisplay,EGLContext gameContext,int32_t width,int32_t height){
    if(!m_options.activity||gameDisplay==EGL_NO_DISPLAY||gameContext==EGL_NO_CONTEXT||width<=0||height<=0)return false;
    ANwCreator::CreateOptions opt;
    opt.name="HideRecSecure";
    opt.width=width;
    opt.height=height;
    opt.skipScreenshot=m_options.skipScreenshot;
    m_nativeWindow=ANwCreator::Create(m_options.activity,opt);
    if(!m_nativeWindow)return false;
    m_display=gameDisplay;
    m_gameContext=gameContext;

    EGLConfig config=FindConfig(m_display,gameContext);
    if(!config){UnInitEnvironment();return false;}
    EGLint format=0;
    if(eglGetConfigAttrib(m_display,config,EGL_NATIVE_VISUAL_ID,&format)!=EGL_TRUE){UnInitEnvironment();return false;}
    ANativeWindow_setBuffersGeometry(m_nativeWindow,width,height,format);

    m_surface=eglCreateWindowSurface(m_display,config,m_nativeWindow,nullptr);
    if(m_surface==EGL_NO_SURFACE){UnInitEnvironment();return false;}

    const EGLint ctxAttrs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    m_overlayContext=eglCreateContext(m_display,config,gameContext,ctxAttrs);
    if(m_overlayContext==EGL_NO_CONTEXT){UnInitEnvironment();return false;}

    EGLint realW=0,realH=0;
    if(eglQuerySurface(m_display,m_surface,EGL_WIDTH,&realW)==EGL_TRUE&&realW>0)m_screenWidth=realW;else m_screenWidth=width;
    if(eglQuerySurface(m_display,m_surface,EGL_HEIGHT,&realH)==EGL_TRUE&&realH>0)m_screenHeight=realH;else m_screenHeight=height;
    m_state=true;
    ANwCreator::EnsureVisible(m_options.activity,m_nativeWindow);
    return true;
}

bool HideRecAImGui::EnsureEnvironment(EGLDisplay gameDisplay,EGLContext gameContext,int32_t width,int32_t height){
    if(gameDisplay==EGL_NO_DISPLAY||gameContext==EGL_NO_CONTEXT||width<=0||height<=0)return false;
    if(m_state&&(m_display!=gameDisplay||m_gameContext!=gameContext||m_screenWidth!=width||m_screenHeight!=height))UnInitEnvironment();
    if(!m_state&&!InitEnvironment(gameDisplay,gameContext,width,height))return false;
    return true;
}

bool HideRecAImGui::RenderDrawData(ImDrawData* drawData,EGLDisplay gameDisplay,EGLSurface gameSurface,
                                   EGLContext gameContext,SwapBuffersFn swapBuffersFn){
    if(!drawData||!swapBuffersFn||gameSurface==EGL_NO_SURFACE)return false;

    int32_t wantedW=(int32_t)std::lround(drawData->DisplaySize.x);
    int32_t wantedH=(int32_t)std::lround(drawData->DisplaySize.y);
    if(wantedW<=0||wantedH<=0)return false;
    if(!EnsureEnvironment(gameDisplay,gameContext,wantedW,wantedH))return false;

    EGLDisplay oldDisplay=eglGetCurrentDisplay();
    EGLSurface oldDraw=eglGetCurrentSurface(EGL_DRAW);
    EGLSurface oldRead=eglGetCurrentSurface(EGL_READ);
    EGLContext oldContext=eglGetCurrentContext();


    if(m_overlayContext==EGL_NO_CONTEXT||eglMakeCurrent(m_display,m_surface,m_surface,m_overlayContext)!=EGL_TRUE)return false;
    EGLint vw=0,vh=0;
    if(eglQuerySurface(m_display,m_surface,EGL_WIDTH,&vw)!=EGL_TRUE||vw<=0)vw=m_screenWidth;
    if(eglQuerySurface(m_display,m_surface,EGL_HEIGHT,&vh)!=EGL_TRUE||vh<=0)vh=m_screenHeight;
    glViewport(0,0,vw,vh);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.f,0.f,0.f,0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
// buffer alpha 0
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
    EGLBoolean ok=swapBuffersFn(m_display,m_surface);

    if(oldDisplay!=EGL_NO_DISPLAY&&oldContext!=EGL_NO_CONTEXT)
        eglMakeCurrent(oldDisplay,oldDraw,oldRead,oldContext);
    else
        eglMakeCurrent(m_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    return ok==EGL_TRUE;
}

void HideRecAImGui::Shutdown(){UnInitEnvironment();}
void HideRecAImGui::UnInitEnvironment(){
    m_state=false;
    if(m_display!=EGL_NO_DISPLAY&&m_surface!=EGL_NO_SURFACE){
        if(eglGetCurrentSurface(EGL_DRAW)==m_surface)
            eglMakeCurrent(m_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        eglDestroySurface(m_display,m_surface);
    }
    if(m_display!=EGL_NO_DISPLAY&&m_overlayContext!=EGL_NO_CONTEXT)
        eglDestroyContext(m_display,m_overlayContext);
    m_surface=EGL_NO_SURFACE;
    m_overlayContext=EGL_NO_CONTEXT;
    // m_gameContext milik game: JANGAN eglDestroyContext / eglTerminate.
    if(m_nativeWindow){ANwCreator::Destroy(m_options.activity,m_nativeWindow);m_nativeWindow=nullptr;}
    m_display=EGL_NO_DISPLAY;
    m_gameContext=EGL_NO_CONTEXT;
    m_screenWidth=m_screenHeight=-1;
}
}

#include <jni.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <cstring>



static ANativeActivity g_nativeActivity{};
static bool g_initialized = false;

// JNI_OnLoad removed
static JavaVM* g_vm = nullptr;
extern "C" void SetANativeActivityVM(JavaVM* vm)
{
    g_vm = vm;
}

static jobject GetCurrentActivity(JNIEnv* env)
{
    jclass activityThreadCls =
        env->FindClass("android/app/ActivityThread");
    if (!activityThreadCls)
        return nullptr;

    jmethodID currentActivityThread =
        env->GetStaticMethodID(
            activityThreadCls,
            "currentActivityThread",
            "()Landroid/app/ActivityThread;");
    if (!currentActivityThread)
        return nullptr;

    jobject activityThread =
        env->CallStaticObjectMethod(
            activityThreadCls,
            currentActivityThread);
    if (!activityThread)
        return nullptr;

    jfieldID mActivitiesField =
        env->GetFieldID(
            activityThreadCls,
            "mActivities",
            "Landroid/util/ArrayMap;");
    if (!mActivitiesField)
        return nullptr;

    jobject mActivities =
        env->GetObjectField(activityThread, mActivitiesField);
    if (!mActivities)
        return nullptr;

    jclass arrayMapCls = env->GetObjectClass(mActivities);

    jmethodID valuesMethod =
        env->GetMethodID(
            arrayMapCls,
            "values",
            "()Ljava/util/Collection;");
    if (!valuesMethod)
        return nullptr;

    jobject values =
        env->CallObjectMethod(mActivities, valuesMethod);
    if (!values)
        return nullptr;

    jclass collectionCls = env->GetObjectClass(values);

    jmethodID iteratorMethod =
        env->GetMethodID(
            collectionCls,
            "iterator",
            "()Ljava/util/Iterator;");
    if (!iteratorMethod)
        return nullptr;

    jobject iterator =
        env->CallObjectMethod(values, iteratorMethod);
    if (!iterator)
        return nullptr;

    jclass iteratorCls = env->GetObjectClass(iterator);

    jmethodID hasNext =
        env->GetMethodID(iteratorCls, "hasNext", "()Z");
    jmethodID next =
        env->GetMethodID(iteratorCls, "next", "()Ljava/lang/Object;");

    while (env->CallBooleanMethod(iterator, hasNext))
    {
        jobject record =
            env->CallObjectMethod(iterator, next);
        if (!record)
            continue;

        jclass recordCls = env->GetObjectClass(record);

        jfieldID activityField =
            env->GetFieldID(
                recordCls,
                "activity",
                "Landroid/app/Activity;");
        if (!activityField)
            continue;

        jobject activity =
            env->GetObjectField(record, activityField);
        if (activity)
            return activity;
    }

    return nullptr;
}

extern "C"
ANativeActivity* GetANativeActivity()
{
    if (g_initialized)
        return &g_nativeActivity;

    if (!g_vm)
        return nullptr;

    JNIEnv* env = nullptr;
    if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
        return nullptr;

    jobject activity = GetCurrentActivity(env);
    if (!activity)
        return nullptr;

    memset(&g_nativeActivity, 0, sizeof(g_nativeActivity));
    g_nativeActivity.vm    = g_vm;
    g_nativeActivity.env   = env;
    g_nativeActivity.clazz = env->NewGlobalRef(activity);

    g_initialized = true;

    
    return &g_nativeActivity;
}

#include <jni.h>
#include <android/native_activity.h>
#include <EGL/egl.h>

extern "C" ANativeActivity* GetANativeActivity();
extern "C" void SetANativeActivityVM(JavaVM* vm);

static android::HideRecAImGui* g_zel_imgui=nullptr;
static JavaVM* g_zel_vm=nullptr;

inline void ZEL_SetVM(JavaVM* vm){g_zel_vm=vm;SetANativeActivityVM(vm);}

inline bool ZEL_Render(ImDrawData* drawData,EGLDisplay dpy,EGLSurface surface,EGLContext context,
                       android::HideRecAImGui::SwapBuffersFn swapFn){
    if(!g_zel_imgui){
        ANativeActivity* activity=GetANativeActivity();
        if(!activity)return false;
        android::HideRecAImGui::Options opts;
        opts.activity=activity;
        opts.skipScreenshot=true;
        g_zel_imgui=new android::HideRecAImGui(opts);
    }
    return g_zel_imgui->RenderDrawData(drawData,dpy,surface,context,swapFn);
}

inline void ZEL_Shutdown(){
    if(g_zel_imgui){delete g_zel_imgui;g_zel_imgui=nullptr;}
}
inline bool ZEL_IsReady(){return g_zel_imgui&&(bool)*g_zel_imgui;}

