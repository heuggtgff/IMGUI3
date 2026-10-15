// Host stub for <GLES3/gl3.h> - desktop syntax checks only (no NDK GL headers
// on this machine). The scalar typedefs are identical to the real ones, and
// IMGUI's bundled loader (imgui_impl_opengl3_loader.h) installs the same names
// as macros when a TU includes it, so including both is not a conflict.
//
// Entry points are declared variadic on purpose: this stub must not reject a
// call whose real GLES3 signature it does not model. Return types are the real
// ones, so a call site that uses the result still type-checks.
#pragma once

#include <cstddef>

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef int GLfixed;
typedef int GLclampx;
typedef void GLvoid;
typedef long GLintptr;
typedef long GLsizeiptr;
typedef char GLchar;
typedef long long GLint64;
typedef unsigned long long GLuint64;
typedef struct __GLsync *GLsync;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_NONE 0
#define GL_ZERO 0
#define GL_ONE 1
#define GL_NO_ERROR 0

#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_QUAD_STRIP 0x0008
#define GL_POLYGON 0x0009
#define GL_LINE_STRIP_ADJACENCY 0x000B
#define GL_TRIANGLES_ADJACENCY 0x000C
#define GL_PATCHES 0x000E
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_STENCIL_TEST 0x0B90
#define GL_CULL_FACE 0x0B44
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_PRIMITIVE_RESTART 0x8F9D
#define GL_DITHER 0x0BD0
#define GL_MULTISAMPLE 0x809D
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define GL_VERTEX_ARRAY_BINDING_OES 0x85B5
#define GL_SAMPLER_BINDING 0x8919
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_SCISSOR_BOX 0x0C10
#define GL_VIEWPORT 0x0BA2
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FLOAT 0x1406
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_UNSIGNED_INT 0x1405
#define GL_RGBA 0x1908
#define GL_RGB 0x1907
#define GL_ALPHA 0x1906
#define GL_BGRA 0x80E1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_REPEAT 0x2901
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_BLEND_EQUATION_ALPHA 0x883D
#define GL_FUNC_ADD 0x8006
#define GL_FILL 0x1B02
#define GL_LINE 0x1B01
#define GL_FRONT_AND_BACK 0x0408
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_POLYGON_MODE 0x0B40
#define GL_CLIP_ORIGIN 0x935C
#define GL_LOWER_LEFT 0x8BB1
#define GL_UPPER_LEFT 0x8BB2
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_NUM_EXTENSIONS 0x821D
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_RGBA8 0x8058
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE 0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE 0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_VERTEX_ATTRIB_ARRAY_POINTER 0x8645
#define GL_VERSION_1_0 1
#define GL_VERSION_1_1 1
#define GL_VERSION_1_3 1
#define GL_VERSION_1_4 1
#define GL_VERSION_1_5 1
#define GL_VERSION_2_0 1
#define GL_VERSION_3_0 1
#define GL_VERSION_3_1 1
#define GL_VERSION_3_2 1
#define GL_VERSION_3_3 1
#define GL_GLEXT_PROTOTYPES 1

#define HOST_STUB_GL_NAME template<typename... A>

// NOT extern "C": a variadic template cannot have C language linkage. Real GL
// has C linkage, but nothing in a syntax check can observe the difference.
HOST_STUB_GL_NAME void glActiveTexture(A&&...) {}
HOST_STUB_GL_NAME void glAttachShader(A&&...) {}
HOST_STUB_GL_NAME void glBindBuffer(A&&...) {}
HOST_STUB_GL_NAME void glBindSampler(A&&...) {}
HOST_STUB_GL_NAME void glBindTexture(A&&...) {}
HOST_STUB_GL_NAME void glBindVertexArray(A&&...) {}
HOST_STUB_GL_NAME void glBindVertexArrayOES(A&&...) {}
HOST_STUB_GL_NAME void glBlendEquation(A&&...) {}
HOST_STUB_GL_NAME void glBlendEquationSeparate(A&&...) {}
HOST_STUB_GL_NAME void glBlendFunc(A&&...) {}
HOST_STUB_GL_NAME void glBlendFuncSeparate(A&&...) {}
HOST_STUB_GL_NAME void glBufferData(A&&...) {}
HOST_STUB_GL_NAME void glBufferSubData(A&&...) {}
HOST_STUB_GL_NAME void glClear(A&&...) {}
HOST_STUB_GL_NAME void glClearColor(A&&...) {}
HOST_STUB_GL_NAME void glClipControl(A&&...) {}
HOST_STUB_GL_NAME void glCompileShader(A&&...) {}
HOST_STUB_GL_NAME GLuint glCreateProgram(A&&...) { return {}; }
HOST_STUB_GL_NAME GLuint glCreateShader(A&&...) { return {}; }
HOST_STUB_GL_NAME void glDeleteBuffers(A&&...) {}
HOST_STUB_GL_NAME void glDeleteProgram(A&&...) {}
HOST_STUB_GL_NAME void glDeleteShader(A&&...) {}
HOST_STUB_GL_NAME void glDeleteTextures(A&&...) {}
HOST_STUB_GL_NAME void glDeleteVertexArrays(A&&...) {}
HOST_STUB_GL_NAME void glDeleteVertexArraysOES(A&&...) {}
HOST_STUB_GL_NAME void glDetachShader(A&&...) {}
HOST_STUB_GL_NAME void glDisable(A&&...) {}
HOST_STUB_GL_NAME void glDisableVertexAttribArray(A&&...) {}
HOST_STUB_GL_NAME void glDrawElements(A&&...) {}
HOST_STUB_GL_NAME void glDrawElementsBaseVertex(A&&...) {}
HOST_STUB_GL_NAME void glEnable(A&&...) {}
HOST_STUB_GL_NAME void glEnableVertexAttribArray(A&&...) {}
HOST_STUB_GL_NAME void glFlush(A&&...) {}
HOST_STUB_GL_NAME void glGenBuffers(A&&...) {}
HOST_STUB_GL_NAME void glGenTextures(A&&...) {}
HOST_STUB_GL_NAME void glGenVertexArrays(A&&...) {}
HOST_STUB_GL_NAME void glGenVertexArraysOES(A&&...) {}
HOST_STUB_GL_NAME GLint glGetAttribLocation(A&&...) { return {}; }
HOST_STUB_GL_NAME GLenum glGetError(A&&...) { return {}; }
HOST_STUB_GL_NAME void glGetIntegerv(A&&...) {}
HOST_STUB_GL_NAME void glGetProgramInfoLog(A&&...) {}
HOST_STUB_GL_NAME void glGetProgramiv(A&&...) {}
HOST_STUB_GL_NAME void glGetShaderInfoLog(A&&...) {}
HOST_STUB_GL_NAME void glGetShaderiv(A&&...) {}
HOST_STUB_GL_NAME const GLubyte* glGetString(A&&...) { return {}; }
HOST_STUB_GL_NAME const GLubyte* glGetStringi(A&&...) { return {}; }
HOST_STUB_GL_NAME GLint glGetUniformLocation(A&&...) { return {}; }
HOST_STUB_GL_NAME void glGetVertexAttribPointerv(A&&...) {}
HOST_STUB_GL_NAME void glGetVertexAttribiv(A&&...) {}
HOST_STUB_GL_NAME GLboolean glIsEnabled(A&&...) { return {}; }
HOST_STUB_GL_NAME void glLinkProgram(A&&...) {}
HOST_STUB_GL_NAME void glPixelStorei(A&&...) {}
HOST_STUB_GL_NAME void glPolygonMode(A&&...) {}
HOST_STUB_GL_NAME void glReadPixels(A&&...) {}
HOST_STUB_GL_NAME void glScissor(A&&...) {}
HOST_STUB_GL_NAME void glShaderSource(A&&...) {}
HOST_STUB_GL_NAME void glTexImage2D(A&&...) {}
HOST_STUB_GL_NAME void glTexParameteri(A&&...) {}
HOST_STUB_GL_NAME void glUniform1i(A&&...) {}
HOST_STUB_GL_NAME void glUniformMatrix4fv(A&&...) {}
HOST_STUB_GL_NAME void glUseProgram(A&&...) {}
HOST_STUB_GL_NAME void glVertexAttribPointer(A&&...) {}
HOST_STUB_GL_NAME void glViewport(A&&...) {}
// The rest of the core profile, so a new call in the UI/render headers does
// not force an edit here. Only the value-returning entry points need a
// concrete type; the rest are variadic (arguments are NOT checked by a stub).
HOST_STUB_GL_NAME void glColorMask(A&&...) {}
HOST_STUB_GL_NAME void glDepthMask(A&&...) {}
HOST_STUB_GL_NAME void glStencilMask(A&&...) {}
HOST_STUB_GL_NAME void glColorMaski(A&&...) {}
HOST_STUB_GL_NAME void glDepthFunc(A&&...) {}
HOST_STUB_GL_NAME void glDepthRangef(A&&...) {}
HOST_STUB_GL_NAME void glClearDepthf(A&&...) {}
HOST_STUB_GL_NAME void glCullFace(A&&...) {}
HOST_STUB_GL_NAME void glFrontFace(A&&...) {}
HOST_STUB_GL_NAME void glLineWidth(A&&...) {}
HOST_STUB_GL_NAME void glPointSize(A&&...) {}
HOST_STUB_GL_NAME void glBlendColor(A&&...) {}
HOST_STUB_GL_NAME void glPolygonOffset(A&&...) {}
HOST_STUB_GL_NAME void glSampleCoverage(A&&...) {}
HOST_STUB_GL_NAME void glFinish(A&&...) {}
HOST_STUB_GL_NAME void glTexSubImage2D(A&&...) {}
HOST_STUB_GL_NAME void glTexParameterf(A&&...) {}
HOST_STUB_GL_NAME void glGenerateMipmap(A&&...) {}
HOST_STUB_GL_NAME void glBindAttribLocation(A&&...) {}
HOST_STUB_GL_NAME void glValidateProgram(A&&...) {}
HOST_STUB_GL_NAME void glGetActiveUniform(A&&...) {}
HOST_STUB_GL_NAME void glGetActiveAttrib(A&&...) {}
HOST_STUB_GL_NAME void glGetTexLevelParameteriv(A&&...) {}
HOST_STUB_GL_NAME void glVertexAttrib1f(A&&...) {}
HOST_STUB_GL_NAME void glVertexAttrib2f(A&&...) {}
HOST_STUB_GL_NAME void glVertexAttrib3f(A&&...) {}
HOST_STUB_GL_NAME void glVertexAttrib4f(A&&...) {}
HOST_STUB_GL_NAME void glVertexAttribDivisor(A&&...) {}
HOST_STUB_GL_NAME void glDrawArrays(A&&...) {}
HOST_STUB_GL_NAME void glDrawArraysInstanced(A&&...) {}
HOST_STUB_GL_NAME void glDrawElementsInstanced(A&&...) {}
HOST_STUB_GL_NAME void glMultiDrawElements(A&&...) {}
HOST_STUB_GL_NAME void glDrawRangeElements(A&&...) {}
HOST_STUB_GL_NAME void glBlitFramebuffer(A&&...) {}
HOST_STUB_GL_NAME void glFramebufferTexture2D(A&&...) {}
HOST_STUB_GL_NAME void glFramebufferRenderbuffer(A&&...) {}
HOST_STUB_GL_NAME GLenum glCheckFramebufferStatus(A&&...) { return {}; }
HOST_STUB_GL_NAME void glBindFramebuffer(A&&...) {}
HOST_STUB_GL_NAME void glDeleteFramebuffers(A&&...) {}
HOST_STUB_GL_NAME void glGenFramebuffers(A&&...) {}
HOST_STUB_GL_NAME void glBindRenderbuffer(A&&...) {}
HOST_STUB_GL_NAME void glDeleteRenderbuffers(A&&...) {}
HOST_STUB_GL_NAME void glGenRenderbuffers(A&&...) {}
HOST_STUB_GL_NAME void glRenderbufferStorage(A&&...) {}
HOST_STUB_GL_NAME void glUniform1f(A&&...) {}
HOST_STUB_GL_NAME void glUniform1fv(A&&...) {}
HOST_STUB_GL_NAME void glUniform2f(A&&...) {}
HOST_STUB_GL_NAME void glUniform3f(A&&...) {}
HOST_STUB_GL_NAME void glUniform4f(A&&...) {}
HOST_STUB_GL_NAME void glUniform1iv(A&&...) {}
HOST_STUB_GL_NAME void glUniform2iv(A&&...) {}
HOST_STUB_GL_NAME void glUniform2fv(A&&...) {}
HOST_STUB_GL_NAME void glUniform3fv(A&&...) {}
HOST_STUB_GL_NAME void glUniform4fv(A&&...) {}
HOST_STUB_GL_NAME void glUniformMatrix3fv(A&&...) {}
HOST_STUB_GL_NAME void glUniformMatrix4x3fv(A&&...) {}
HOST_STUB_GL_NAME void glGetUniformfv(A&&...) {}
HOST_STUB_GL_NAME void glGetUniformiv(A&&...) {}
HOST_STUB_GL_NAME void glGetBufferParameteriv(A&&...) {}
HOST_STUB_GL_NAME void glGetFloatv(A&&...) {}
HOST_STUB_GL_NAME void glGetBooleanv(A&&...) {}
HOST_STUB_GL_NAME void glGetShaderSource(A&&...) {}
HOST_STUB_GL_NAME void glBindBufferRange(A&&...) {}
HOST_STUB_GL_NAME void glBindBufferBase(A&&...) {}
HOST_STUB_GL_NAME void glMapBufferRange(A&&...) {}
HOST_STUB_GL_NAME void glUnmapBuffer(A&&...) {}
HOST_STUB_GL_NAME void glDispatchCompute(A&&...) {}
HOST_STUB_GL_NAME void glMemoryBarrier(A&&...) {}
HOST_STUB_GL_NAME void glFlushMappedBufferRange(A&&...) {}
HOST_STUB_GL_NAME void glDiscardFramebufferEXT(A&&...) {}
HOST_STUB_GL_NAME void glInvalidateFramebuffer(A&&...) {}

// The desktop/immediate-mode names imgui_impl_opengl2 and some legs of this
// repo still reference; harmless on the GL3 path.
HOST_STUB_GL_NAME void glBegin(A&&...) {}
HOST_STUB_GL_NAME void glEnd(A&&...) {}
HOST_STUB_GL_NAME void glVertex2f(A&&...) {}
HOST_STUB_GL_NAME void glTexCoord2f(A&&...) {}
HOST_STUB_GL_NAME void* glXGetProcAddressARB(A&&...) { return {}; }
