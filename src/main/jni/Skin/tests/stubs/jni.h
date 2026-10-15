// Host stub for <jni.h> - used ONLY by the desktop syntax checks in
// Skin/tests and Hacks/Feature/tests (the real header comes from the NDK in
// the Android build).
//
// Every accessor takes an arbitrary argument list (the real JNI API is
// variadic) but returns its TRUE type, so a call site that would be a type
// error against the NDK fails here too. It cannot check argument counts - a
// real NDK/device build is still the only proof of a JNI call.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <mutex>

typedef int32_t  jint;
typedef int8_t   jbyte;
typedef int16_t  jshort;
typedef int64_t  jlong;
typedef float    jfloat;
typedef double   jdouble;
typedef uint8_t  jboolean;
typedef uint16_t jchar;
typedef jint     jsize;
typedef unsigned char jbool;

// Separate classes, as in the real C++ jni.h: a jclass must NOT be
// interchangeable with a jobject at overload resolution, while still
// converting to one (that upcast is what real code relies on).
struct _jobject {};
struct _jclass    : _jobject {};
struct _jstring   : _jobject {};
struct _jarray    : _jobject {};
struct _jobjectArray : _jarray {};
struct _jthrowable : _jobject {};
struct _jweak     : _jobject {};
typedef _jobject* jobject;
typedef _jclass*  jclass;
typedef _jstring* jstring;
typedef _jarray*  jarray;
typedef _jobjectArray* jobjectArray;
typedef _jthrowable* jthrowable;
typedef _jweak*   jweak;
struct _jfieldID;
typedef _jfieldID* jfieldID;
struct _jmethodID;
typedef _jmethodID* jmethodID;

typedef jarray jbyteArray;
typedef jarray jcharArray;
typedef jarray jshortArray;
typedef jarray jintArray;
typedef jarray jlongArray;
typedef jarray jfloatArray;
typedef jarray jdoubleArray;
typedef jarray jbooleanArray;

typedef union jvalue {
    jboolean z; jbyte b; jchar c; jshort s;
    jint i; jlong j; jfloat f; jdouble d; jobject l;
} jvalue;

#ifndef JNIEXPORT
#define JNIEXPORT __attribute__((visibility("default")))
#endif
#ifndef JNICALL
#define JNICALL
#endif

#define JNI_VERSION_1_1    0x00010001
#define JNI_VERSION_1_2    0x00010002
#define JNI_VERSION_1_4    0x00010004
#define JNI_VERSION_1_6    0x00010006
#define JNI_VERSION_1_8    0x00010008
#define JNI_OK             0
#define JNI_ERR           (-1)
#define JNI_EDETACHED     (-2)
#define JNI_EVERSION      (-3)
#define JNI_ENOMEM        (-4)
#define JNI_EEXIST        (-5)
#define JNI_EINVAL        (-6)
#define JNI_FALSE          0
#define JNI_TRUE           1
#define JNI_COMMIT         1
#define JNI_ABORT          2

struct JNINativeInterface_;
struct JNIInvokeInterface_;

// Any name in this list may be called with any arguments; the return type is
// the real one. Add a neighbour when a new call appears in the header graph.
#define HOST_STUB_ANY_ARGS template<typename... A>

struct _JNIEnv {
    const JNINativeInterface_* functions = nullptr;

    HOST_STUB_ANY_ARGS jclass  FindClass(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jclass  GetObjectClass(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jclass  GetSuperclass(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jboolean IsAssignableFrom(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jboolean IsInstanceOf(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jboolean IsSameObject(A&&...) { return {}; }

    HOST_STUB_ANY_ARGS jmethodID GetMethodID(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jmethodID GetStaticMethodID(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jfieldID  GetFieldID(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jfieldID  GetStaticFieldID(A&&...) { return {}; }

    HOST_STUB_ANY_ARGS jobject NewObject(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jobjectArray NewObjectArray(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jobject AllocObject(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jobject GetObjectArrayElement(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void    SetObjectArrayElement(A&&...) {}

    HOST_STUB_ANY_ARGS jobject  CallObjectMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jboolean CallBooleanMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint     CallIntMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jlong    CallLongMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jfloat   CallFloatMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jdouble  CallDoubleMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jchar    CallCharMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jshort   CallShortMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void     CallVoidMethod(A&&...) {}

    HOST_STUB_ANY_ARGS jobject CallNonvirtualObjectMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint    CallNonvirtualIntMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void    CallNonvirtualVoidMethod(A&&...) {}

    HOST_STUB_ANY_ARGS jobject  CallStaticObjectMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint     CallStaticIntMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jboolean CallStaticBooleanMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jfloat   CallStaticFloatMethod(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void     CallStaticVoidMethod(A&&...) {}

    HOST_STUB_ANY_ARGS jobject  GetObjectField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint     GetIntField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jboolean GetBooleanField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jlong    GetLongField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jfloat   GetFloatField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jdouble  GetDoubleField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jshort   GetShortField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jchar    GetCharField(A&&...) { return {}; }

    HOST_STUB_ANY_ARGS jobject  GetStaticObjectField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint     GetStaticIntField(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jboolean GetStaticBooleanField(A&&...) { return {}; }

    HOST_STUB_ANY_ARGS void SetObjectField(A&&...) {}
    HOST_STUB_ANY_ARGS void SetIntField(A&&...) {}
    HOST_STUB_ANY_ARGS void SetBooleanField(A&&...) {}
    HOST_STUB_ANY_ARGS void SetLongField(A&&...) {}
    HOST_STUB_ANY_ARGS void SetFloatField(A&&...) {}
    HOST_STUB_ANY_ARGS void SetStaticObjectField(A&&...) {}
    HOST_STUB_ANY_ARGS void SetStaticIntField(A&&...) {}
    HOST_STUB_ANY_ARGS void SetStaticBooleanField(A&&...) {}

    HOST_STUB_ANY_ARGS const char* GetStringUTFChars(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void        ReleaseStringUTFChars(A&&...) {}
    HOST_STUB_ANY_ARGS jsize       GetStringUTFLength(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jstring     NewStringUTF(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS const jchar* GetStringChars(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void        ReleaseStringChars(A&&...) {}
    HOST_STUB_ANY_ARGS jsize       GetStringLength(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jstring     NewString(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void        GetStringRegion(A&&...) {}
    HOST_STUB_ANY_ARGS void        GetStringUTFRegion(A&&...) {}

    HOST_STUB_ANY_ARGS jbyteArray    NewByteArray(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jbooleanArray NewBooleanArray(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jcharArray    NewCharArray(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jintArray     NewIntArray(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jlongArray    NewLongArray(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jfloatArray   NewFloatArray(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jdoubleArray  NewDoubleArray(A&&...) { return {}; }

    HOST_STUB_ANY_ARGS jsize        GetArrayLength(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void         GetByteArrayRegion(A&&...) {}
    HOST_STUB_ANY_ARGS void         SetByteArrayRegion(A&&...) {}
    HOST_STUB_ANY_ARGS jbyte*       GetByteArrayElements(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void         ReleaseByteArrayElements(A&&...) {}
    HOST_STUB_ANY_ARGS jint*        GetIntArrayElements(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void         ReleaseIntArrayElements(A&&...) {}
    HOST_STUB_ANY_ARGS jboolean*    GetBooleanArrayElements(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void         ReleaseBooleanArrayElements(A&&...) {}
    HOST_STUB_ANY_ARGS jfloat*      GetFloatArrayElements(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void         ReleaseFloatArrayElements(A&&...) {}
    HOST_STUB_ANY_ARGS void*        GetPrimitiveArrayCritical(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void         ReleasePrimitiveArrayCritical(A&&...) {}

    HOST_STUB_ANY_ARGS jobject NewGlobalRef(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jobject NewLocalRef(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jweak   NewWeakGlobalRef(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void    DeleteGlobalRef(A&&...) {}
    HOST_STUB_ANY_ARGS void    DeleteLocalRef(A&&...) {}
    HOST_STUB_ANY_ARGS void    DeleteWeakGlobalRef(A&&...) {}
    HOST_STUB_ANY_ARGS jint    PushLocalFrame(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jobject PopLocalFrame(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint    EnsureLocalCapacity(A&&...) { return {}; }

    HOST_STUB_ANY_ARGS jthrowable ExceptionOccurred(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void       ExceptionDescribe(A&&...) {}
    HOST_STUB_ANY_ARGS void       ExceptionClear(A&&...) {}
    HOST_STUB_ANY_ARGS jboolean   ExceptionCheck(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint       Throw(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint       ThrowNew(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void       FatalError(A&&...) {}
    HOST_STUB_ANY_ARGS jint       MonitorEnter(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint       MonitorExit(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint       RegisterNatives(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint       UnregisterNatives(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint       GetJavaVM(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint       GetVersion(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS void*      GetDirectBufferAddress(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jlong      GetDirectBufferCapacity(A&&...) { return {}; }
};
typedef _JNIEnv JNIEnv;

struct _JavaVM {
    const JNIInvokeInterface_* functions = nullptr;
    HOST_STUB_ANY_ARGS jint AttachCurrentThread(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint AttachCurrentThreadAsDaemon(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint DetachCurrentThread(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint GetEnv(A&&...) { return {}; }
    HOST_STUB_ANY_ARGS jint DestroyJavaVM(A&&...) { return {}; }
};
typedef _JavaVM JavaVM;

typedef struct JavaVMAttachArgs {
    jint version;
    const char* name;
    jobject group;
} JavaVMAttachArgs;

typedef struct JavaVMOption {
    char* optionString;
    void* extraInfo;
} JavaVMOption;

typedef struct JavaVMInitArgs {
    jint version;
    jint nOptions;
    JavaVMOption* options;
    jboolean ignoreUnrecognized;
} JavaVMInitArgs;

struct JNIInvokeInterface_ {};
struct JNINativeInterface_ {};

typedef jint (*JNI_OnLoad_t)(JavaVM*, void*);
