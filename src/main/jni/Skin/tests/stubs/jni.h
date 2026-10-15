#pragma once
#include <cstdint>
#include <cstddef>
typedef int32_t jint; typedef int8_t jbyte; typedef int16_t jshort; typedef int64_t jlong;
typedef float jfloat; typedef double jdouble; typedef uint8_t jboolean; typedef uint16_t jchar;
typedef jint jsize; typedef unsigned char jbool;
struct _jobject; typedef _jobject* jobject; typedef _jobject* jclass; typedef _jobject* jstring;
typedef _jobject* jarray; typedef _jobject* jobjectArray; typedef _jobject* jthrowable;
typedef _jobject* jweak; typedef _jobject* jmethodID_placeholder;
struct _jfieldID; typedef _jfieldID* jfieldID;
struct _jmethodID; typedef _jmethodID* jmethodID;
struct _JNIEnv; typedef _JNIEnv JNIEnv;
struct _JavaVM; typedef _JavaVM JavaVM;
typedef union jvalue { jboolean z; jbyte b; jchar c; jshort s; jint i; jlong j; jfloat f; jdouble d; jobject l; } jvalue;
struct JNINativeInterface_;
struct JNIInvokeInterface_;
