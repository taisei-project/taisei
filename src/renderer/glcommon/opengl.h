/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL_platform.h>
#include <SDL_opengl.h>

#include "assert.h"

#define TSGL_EXT_VENDORS \
	TSGL_EXT_VENDOR(NATIVE) \
	TSGL_EXT_VENDOR(KHR) \
	TSGL_EXT_VENDOR(ARB) \
	TSGL_EXT_VENDOR(EXT) \
	TSGL_EXT_VENDOR(OES) \
	TSGL_EXT_VENDOR(AMD) \
	TSGL_EXT_VENDOR(NV) \
	TSGL_EXT_VENDOR(ANGLE) \
	TSGL_EXT_VENDOR(MESA) \

enum {
	#define TSGL_EXT_VENDOR(v) _TSGL_EXTVNUM_##v,
	TSGL_EXT_VENDORS
	#undef TSGL_EXT_VENDOR
};

typedef enum {
	#define TSGL_EXT_VENDOR(v) TSGL_EXTFLAG_##v = (1 << _TSGL_EXTVNUM_##v),
	TSGL_EXT_VENDORS
	#undef TSGL_EXT_VENDOR
} ext_flag_t;

static_assert(TSGL_EXTFLAG_KHR == (1 << _TSGL_EXTVNUM_KHR), "");

struct glext_s; // defined at the very bottom
extern struct glext_s glext;

void glcommon_load_library(void);
void glcommon_load_functions(void);
void glcommon_check_extensions(void);
void glcommon_unload_library(void);
ext_flag_t glcommon_check_extension(const char *ext);
ext_flag_t glcommon_require_extension(const char *ext);

//
// XXX: This is a real mess. We probably should have completely separate headers
// for GL and GLES stuff, somehow, while still letting the GL and GLES backends
// share most of the code. Currently, it is assumed that GL and GLES use the same
// calling convention, and the pointers to equivalent API functions are compatible.
//

#ifndef GLAPIENTRY
	#define GLAPIENTRY
#endif

#ifndef GL_APIENTRY
	#define GL_APIENTRY GLAPIENTRY
#endif

#ifndef APIENTRY
	#define APIENTRY GLAPIENTRY
#endif

#if defined(DEBUG) && defined(TAISEI_BUILDCONF_DEBUG_OPENGL)
	#define DEBUG_GL_DEFAULT 1
#else
	#define DEBUG_GL_DEFAULT 0
#endif

//
// XXX: SDL headers are old. Maybe depend on the system GLES headers instead?
//

typedef void (GL_APIENTRY  *GLDEBUGPROCKHR)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);

/*
 *
 *      From here on it's mostly generated code (by the embedded python script)
 *
 */

// @BEGIN:typedefs@
typedef void (APIENTRY *tsglActiveTexture_ptr)(GLenum texture);
typedef void (APIENTRY *tsglAttachShader_ptr)(GLuint program, GLuint shader);
typedef void (APIENTRY *tsglBindBuffer_ptr)(GLenum target, GLuint buffer);
typedef void (APIENTRY *tsglBindFramebuffer_ptr)(GLenum target, GLuint framebuffer);
typedef void (GL_APIENTRY *tsglBindTexture_ptr)(GLenum target, GLuint texture);
typedef void (APIENTRY *tsglBindVertexArray_ptr)(GLuint array);
typedef void (APIENTRY *tsglBlendEquationSeparate_ptr)(GLenum modeRGB, GLenum modeAlpha);
typedef void (APIENTRY *tsglBlendFuncSeparate_ptr)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void (APIENTRY *tsglBufferData_ptr)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRY *tsglBufferSubData_ptr)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (GL_APIENTRY *tsglClear_ptr)(GLbitfield mask);
typedef void (GL_APIENTRY *tsglClearColor_ptr)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRY *tsglCompileShader_ptr)(GLuint shader);
typedef GLuint (APIENTRY *tsglCreateProgram_ptr)(void);
typedef GLuint (APIENTRY *tsglCreateShader_ptr)(GLenum type);
typedef void (GL_APIENTRY *tsglCullFace_ptr)(GLenum mode);
typedef void (APIENTRY *tsglDebugMessageCallback_ptr)(GLDEBUGPROC callback, const void *userParam);
typedef void (APIENTRY *tsglDebugMessageCallbackARB_ptr)(GLDEBUGPROCARB callback, const void *userParam);
typedef void (GL_APIENTRY *tsglDebugMessageCallbackKHR_ptr)(GLDEBUGPROCKHR callback, const void *userParam);
typedef void (APIENTRY *tsglDebugMessageControl_ptr)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (APIENTRY *tsglDebugMessageControlARB_ptr)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (GL_APIENTRY *tsglDebugMessageControlKHR_ptr)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (APIENTRY *tsglDeleteBuffers_ptr)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *tsglDeleteFramebuffers_ptr)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *tsglDeleteProgram_ptr)(GLuint program);
typedef void (APIENTRY *tsglDeleteShader_ptr)(GLuint shader);
typedef void (GL_APIENTRY *tsglDeleteTextures_ptr)(GLsizei n, const GLuint *textures);
typedef void (APIENTRY *tsglDeleteVertexArrays_ptr)(GLsizei n, const GLuint *arrays);
typedef void (GL_APIENTRY *tsglDepthFunc_ptr)(GLenum func);
typedef void (GL_APIENTRY *tsglDepthMask_ptr)(GLboolean flag);
typedef void (GL_APIENTRY *tsglDisable_ptr)(GLenum cap);
typedef void (APIENTRY *tsglDisableVertexAttribArray_ptr)(GLuint index);
typedef void (GL_APIENTRY *tsglDrawArrays_ptr)(GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY *tsglDrawArraysInstanced_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void (GL_APIENTRY *tsglDrawArraysInstancedANGLE_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
typedef void (APIENTRY *tsglDrawArraysInstancedARB_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
typedef void (APIENTRY *tsglDrawArraysInstancedBaseInstance_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
typedef void (GL_APIENTRY *tsglDrawArraysInstancedBaseInstanceEXT_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
typedef void (GL_APIENTRY *tsglDrawArraysInstancedEXT_ptr)(GLenum mode, GLint start, GLsizei count, GLsizei primcount);
typedef void (GL_APIENTRY *tsglDrawArraysInstancedNV_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
typedef void (APIENTRY *tsglDrawBuffers_ptr)(GLsizei n, const GLenum *bufs);
typedef void (APIENTRY *tsglDrawBuffersARB_ptr)(GLsizei n, const GLenum *bufs);
typedef void (GL_APIENTRY *tsglDrawBuffersEXT_ptr)(GLsizei n, const GLenum *bufs);
typedef void (GL_APIENTRY *tsglDrawElements_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void (APIENTRY *tsglDrawElementsInstanced_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
typedef void (GL_APIENTRY *tsglDrawElementsInstancedANGLE_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
typedef void (APIENTRY *tsglDrawElementsInstancedARB_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
typedef void (APIENTRY *tsglDrawElementsInstancedBaseInstance_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
typedef void (GL_APIENTRY *tsglDrawElementsInstancedBaseInstanceEXT_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
typedef void (GL_APIENTRY *tsglDrawElementsInstancedEXT_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
typedef void (GL_APIENTRY *tsglDrawElementsInstancedNV_ptr)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
typedef void (GL_APIENTRY *tsglEnable_ptr)(GLenum cap);
typedef void (APIENTRY *tsglEnableVertexAttribArray_ptr)(GLuint index);
typedef void (APIENTRY *tsglFramebufferTexture2D_ptr)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY *tsglGenBuffers_ptr)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *tsglGenFramebuffers_ptr)(GLsizei n, GLuint *framebuffers);
typedef void (GL_APIENTRY *tsglGenTextures_ptr)(GLsizei n, GLuint *textures);
typedef void (APIENTRY *tsglGenVertexArrays_ptr)(GLsizei n, GLuint *arrays);
typedef void (APIENTRY *tsglGetActiveUniform_ptr)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void (GL_APIENTRY *tsglGetIntegerv_ptr)(GLenum pname, GLint *data);
typedef void (APIENTRY *tsglGetProgramInfoLog_ptr)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *tsglGetProgramiv_ptr)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *tsglGetShaderInfoLog_ptr)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *tsglGetShaderiv_ptr)(GLuint shader, GLenum pname, GLint *params);
typedef const GLubyte * (GL_APIENTRY *tsglGetString_ptr)(GLenum name);
typedef const GLubyte * (APIENTRY *tsglGetStringi_ptr)(GLenum name, GLuint index);
typedef GLint (APIENTRY *tsglGetUniformLocation_ptr)(GLuint program, const GLchar *name);
typedef void (APIENTRY *tsglLinkProgram_ptr)(GLuint program);
typedef void (APIENTRY *tsglObjectLabel_ptr)(GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
typedef void (GL_APIENTRY *tsglObjectLabelKHR_ptr)(GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
typedef void (GL_APIENTRY *tsglPixelStorei_ptr)(GLenum pname, GLint param);
typedef void (GL_APIENTRY *tsglReadBuffer_ptr)(GLenum src);
typedef void (GL_APIENTRY *tsglReadPixels_ptr)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
typedef void (APIENTRY *tsglShaderSource_ptr)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (GL_APIENTRY *tsglTexImage2D_ptr)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (GL_APIENTRY *tsglTexParameterf_ptr)(GLenum target, GLenum pname, GLfloat param);
typedef void (GL_APIENTRY *tsglTexParameteri_ptr)(GLenum target, GLenum pname, GLint param);
typedef void (GL_APIENTRY *tsglTexSubImage2D_ptr)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRY *tsglUniform1fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform1iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform2fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform2iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform3fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform3iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform4fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform4iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniformMatrix3fv_ptr)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *tsglUniformMatrix4fv_ptr)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *tsglUseProgram_ptr)(GLuint program);
typedef void (APIENTRY *tsglVertexAttribDivisor_ptr)(GLuint index, GLuint divisor);
typedef void (GL_APIENTRY *tsglVertexAttribDivisorANGLE_ptr)(GLuint index, GLuint divisor);
typedef void (APIENTRY *tsglVertexAttribDivisorARB_ptr)(GLuint index, GLuint divisor);
typedef void (GL_APIENTRY *tsglVertexAttribDivisorEXT_ptr)(GLuint index, GLuint divisor);
typedef void (GL_APIENTRY *tsglVertexAttribDivisorNV_ptr)(GLuint index, GLuint divisor);
typedef void (APIENTRY *tsglVertexAttribIPointer_ptr)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void (APIENTRY *tsglVertexAttribPointer_ptr)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (GL_APIENTRY *tsglViewport_ptr)(GLint x, GLint y, GLsizei width, GLsizei height);
// @END:typedefs@

// @BEGIN:undefs@
#undef glActiveTexture
#undef glAttachShader
#undef glBindBuffer
#undef glBindFramebuffer
#undef glBindTexture
#undef glBindVertexArray
#undef glBlendEquationSeparate
#undef glBlendFuncSeparate
#undef glBufferData
#undef glBufferSubData
#undef glClear
#undef glClearColor
#undef glCompileShader
#undef glCreateProgram
#undef glCreateShader
#undef glCullFace
#undef glDebugMessageCallback
#undef glDebugMessageCallbackARB
#undef glDebugMessageCallbackKHR
#undef glDebugMessageControl
#undef glDebugMessageControlARB
#undef glDebugMessageControlKHR
#undef glDeleteBuffers
#undef glDeleteFramebuffers
#undef glDeleteProgram
#undef glDeleteShader
#undef glDeleteTextures
#undef glDeleteVertexArrays
#undef glDepthFunc
#undef glDepthMask
#undef glDisable
#undef glDisableVertexAttribArray
#undef glDrawArrays
#undef glDrawArraysInstanced
#undef glDrawArraysInstancedANGLE
#undef glDrawArraysInstancedARB
#undef glDrawArraysInstancedBaseInstance
#undef glDrawArraysInstancedBaseInstanceEXT
#undef glDrawArraysInstancedEXT
#undef glDrawArraysInstancedNV
#undef glDrawBuffers
#undef glDrawBuffersARB
#undef glDrawBuffersEXT
#undef glDrawElements
#undef glDrawElementsInstanced
#undef glDrawElementsInstancedANGLE
#undef glDrawElementsInstancedARB
#undef glDrawElementsInstancedBaseInstance
#undef glDrawElementsInstancedBaseInstanceEXT
#undef glDrawElementsInstancedEXT
#undef glDrawElementsInstancedNV
#undef glEnable
#undef glEnableVertexAttribArray
#undef glFramebufferTexture2D
#undef glGenBuffers
#undef glGenFramebuffers
#undef glGenTextures
#undef glGenVertexArrays
#undef glGetActiveUniform
#undef glGetIntegerv
#undef glGetProgramInfoLog
#undef glGetProgramiv
#undef glGetShaderInfoLog
#undef glGetShaderiv
#undef glGetString
#undef glGetStringi
#undef glGetUniformLocation
#undef glLinkProgram
#undef glObjectLabel
#undef glObjectLabelKHR
#undef glPixelStorei
#undef glReadBuffer
#undef glReadPixels
#undef glShaderSource
#undef glTexImage2D
#undef glTexParameterf
#undef glTexParameteri
#undef glTexSubImage2D
#undef glUniform1fv
#undef glUniform1iv
#undef glUniform2fv
#undef glUniform2iv
#undef glUniform3fv
#undef glUniform3iv
#undef glUniform4fv
#undef glUniform4iv
#undef glUniformMatrix3fv
#undef glUniformMatrix4fv
#undef glUseProgram
#undef glVertexAttribDivisor
#undef glVertexAttribDivisorANGLE
#undef glVertexAttribDivisorARB
#undef glVertexAttribDivisorEXT
#undef glVertexAttribDivisorNV
#undef glVertexAttribIPointer
#undef glVertexAttribPointer
#undef glViewport
// @END:undefs@

#ifndef LINK_TO_LIBGL
// @BEGIN:redefs@
#define glActiveTexture tsglActiveTexture
#define glAttachShader tsglAttachShader
#define glBindBuffer tsglBindBuffer
#define glBindFramebuffer tsglBindFramebuffer
#define glBindTexture tsglBindTexture
#define glBindVertexArray tsglBindVertexArray
#define glBlendEquationSeparate tsglBlendEquationSeparate
#define glBlendFuncSeparate tsglBlendFuncSeparate
#define glBufferData tsglBufferData
#define glBufferSubData tsglBufferSubData
#define glClear tsglClear
#define glClearColor tsglClearColor
#define glCompileShader tsglCompileShader
#define glCreateProgram tsglCreateProgram
#define glCreateShader tsglCreateShader
#define glCullFace tsglCullFace
#define glDebugMessageCallback tsglDebugMessageCallback
#define glDebugMessageCallbackARB tsglDebugMessageCallbackARB
#define glDebugMessageCallbackKHR tsglDebugMessageCallbackKHR
#define glDebugMessageControl tsglDebugMessageControl
#define glDebugMessageControlARB tsglDebugMessageControlARB
#define glDebugMessageControlKHR tsglDebugMessageControlKHR
#define glDeleteBuffers tsglDeleteBuffers
#define glDeleteFramebuffers tsglDeleteFramebuffers
#define glDeleteProgram tsglDeleteProgram
#define glDeleteShader tsglDeleteShader
#define glDeleteTextures tsglDeleteTextures
#define glDeleteVertexArrays tsglDeleteVertexArrays
#define glDepthFunc tsglDepthFunc
#define glDepthMask tsglDepthMask
#define glDisable tsglDisable
#define glDisableVertexAttribArray tsglDisableVertexAttribArray
#define glDrawArrays tsglDrawArrays
#define glDrawArraysInstanced tsglDrawArraysInstanced
#define glDrawArraysInstancedANGLE tsglDrawArraysInstancedANGLE
#define glDrawArraysInstancedARB tsglDrawArraysInstancedARB
#define glDrawArraysInstancedBaseInstance tsglDrawArraysInstancedBaseInstance
#define glDrawArraysInstancedBaseInstanceEXT tsglDrawArraysInstancedBaseInstanceEXT
#define glDrawArraysInstancedEXT tsglDrawArraysInstancedEXT
#define glDrawArraysInstancedNV tsglDrawArraysInstancedNV
#define glDrawBuffers tsglDrawBuffers
#define glDrawBuffersARB tsglDrawBuffersARB
#define glDrawBuffersEXT tsglDrawBuffersEXT
#define glDrawElements tsglDrawElements
#define glDrawElementsInstanced tsglDrawElementsInstanced
#define glDrawElementsInstancedANGLE tsglDrawElementsInstancedANGLE
#define glDrawElementsInstancedARB tsglDrawElementsInstancedARB
#define glDrawElementsInstancedBaseInstance tsglDrawElementsInstancedBaseInstance
#define glDrawElementsInstancedBaseInstanceEXT tsglDrawElementsInstancedBaseInstanceEXT
#define glDrawElementsInstancedEXT tsglDrawElementsInstancedEXT
#define glDrawElementsInstancedNV tsglDrawElementsInstancedNV
#define glEnable tsglEnable
#define glEnableVertexAttribArray tsglEnableVertexAttribArray
#define glFramebufferTexture2D tsglFramebufferTexture2D
#define glGenBuffers tsglGenBuffers
#define glGenFramebuffers tsglGenFramebuffers
#define glGenTextures tsglGenTextures
#define glGenVertexArrays tsglGenVertexArrays
#define glGetActiveUniform tsglGetActiveUniform
#define glGetIntegerv tsglGetIntegerv
#define glGetProgramInfoLog tsglGetProgramInfoLog
#define glGetProgramiv tsglGetProgramiv
#define glGetShaderInfoLog tsglGetShaderInfoLog
#define glGetShaderiv tsglGetShaderiv
#define glGetString tsglGetString
#define glGetStringi tsglGetStringi
#define glGetUniformLocation tsglGetUniformLocation
#define glLinkProgram tsglLinkProgram
#define glObjectLabel tsglObjectLabel
#define glObjectLabelKHR tsglObjectLabelKHR
#define glPixelStorei tsglPixelStorei
#define glReadBuffer tsglReadBuffer
#define glReadPixels tsglReadPixels
#define glShaderSource tsglShaderSource
#define glTexImage2D tsglTexImage2D
#define glTexParameterf tsglTexParameterf
#define glTexParameteri tsglTexParameteri
#define glTexSubImage2D tsglTexSubImage2D
#define glUniform1fv tsglUniform1fv
#define glUniform1iv tsglUniform1iv
#define glUniform2fv tsglUniform2fv
#define glUniform2iv tsglUniform2iv
#define glUniform3fv tsglUniform3fv
#define glUniform3iv tsglUniform3iv
#define glUniform4fv tsglUniform4fv
#define glUniform4iv tsglUniform4iv
#define glUniformMatrix3fv tsglUniformMatrix3fv
#define glUniformMatrix4fv tsglUniformMatrix4fv
#define glUseProgram tsglUseProgram
#define glVertexAttribDivisor tsglVertexAttribDivisor
#define glVertexAttribDivisorANGLE tsglVertexAttribDivisorANGLE
#define glVertexAttribDivisorARB tsglVertexAttribDivisorARB
#define glVertexAttribDivisorEXT tsglVertexAttribDivisorEXT
#define glVertexAttribDivisorNV tsglVertexAttribDivisorNV
#define glVertexAttribIPointer tsglVertexAttribIPointer
#define glVertexAttribPointer tsglVertexAttribPointer
#define glViewport tsglViewport
// @END:redefs@
#endif // !LINK_TO_LIBGL

#ifndef LINK_TO_LIBGL
// @BEGIN:gldefs@
#define GLDEFS \
GLDEF(glActiveTexture, tsglActiveTexture, tsglActiveTexture_ptr) \
GLDEF(glAttachShader, tsglAttachShader, tsglAttachShader_ptr) \
GLDEF(glBindBuffer, tsglBindBuffer, tsglBindBuffer_ptr) \
GLDEF(glBindFramebuffer, tsglBindFramebuffer, tsglBindFramebuffer_ptr) \
GLDEF(glBindTexture, tsglBindTexture, tsglBindTexture_ptr) \
GLDEF(glBindVertexArray, tsglBindVertexArray, tsglBindVertexArray_ptr) \
GLDEF(glBlendEquationSeparate, tsglBlendEquationSeparate, tsglBlendEquationSeparate_ptr) \
GLDEF(glBlendFuncSeparate, tsglBlendFuncSeparate, tsglBlendFuncSeparate_ptr) \
GLDEF(glBufferData, tsglBufferData, tsglBufferData_ptr) \
GLDEF(glBufferSubData, tsglBufferSubData, tsglBufferSubData_ptr) \
GLDEF(glClear, tsglClear, tsglClear_ptr) \
GLDEF(glClearColor, tsglClearColor, tsglClearColor_ptr) \
GLDEF(glCompileShader, tsglCompileShader, tsglCompileShader_ptr) \
GLDEF(glCreateProgram, tsglCreateProgram, tsglCreateProgram_ptr) \
GLDEF(glCreateShader, tsglCreateShader, tsglCreateShader_ptr) \
GLDEF(glCullFace, tsglCullFace, tsglCullFace_ptr) \
GLDEF(glDebugMessageCallback, tsglDebugMessageCallback, tsglDebugMessageCallback_ptr) \
GLDEF(glDebugMessageCallbackARB, tsglDebugMessageCallbackARB, tsglDebugMessageCallbackARB_ptr) \
GLDEF(glDebugMessageCallbackKHR, tsglDebugMessageCallbackKHR, tsglDebugMessageCallbackKHR_ptr) \
GLDEF(glDebugMessageControl, tsglDebugMessageControl, tsglDebugMessageControl_ptr) \
GLDEF(glDebugMessageControlARB, tsglDebugMessageControlARB, tsglDebugMessageControlARB_ptr) \
GLDEF(glDebugMessageControlKHR, tsglDebugMessageControlKHR, tsglDebugMessageControlKHR_ptr) \
GLDEF(glDeleteBuffers, tsglDeleteBuffers, tsglDeleteBuffers_ptr) \
GLDEF(glDeleteFramebuffers, tsglDeleteFramebuffers, tsglDeleteFramebuffers_ptr) \
GLDEF(glDeleteProgram, tsglDeleteProgram, tsglDeleteProgram_ptr) \
GLDEF(glDeleteShader, tsglDeleteShader, tsglDeleteShader_ptr) \
GLDEF(glDeleteTextures, tsglDeleteTextures, tsglDeleteTextures_ptr) \
GLDEF(glDeleteVertexArrays, tsglDeleteVertexArrays, tsglDeleteVertexArrays_ptr) \
GLDEF(glDepthFunc, tsglDepthFunc, tsglDepthFunc_ptr) \
GLDEF(glDepthMask, tsglDepthMask, tsglDepthMask_ptr) \
GLDEF(glDisable, tsglDisable, tsglDisable_ptr) \
GLDEF(glDisableVertexAttribArray, tsglDisableVertexAttribArray, tsglDisableVertexAttribArray_ptr) \
GLDEF(glDrawArrays, tsglDrawArrays, tsglDrawArrays_ptr) \
GLDEF(glDrawArraysInstanced, tsglDrawArraysInstanced, tsglDrawArraysInstanced_ptr) \
GLDEF(glDrawArraysInstancedANGLE, tsglDrawArraysInstancedANGLE, tsglDrawArraysInstancedANGLE_ptr) \
GLDEF(glDrawArraysInstancedARB, tsglDrawArraysInstancedARB, tsglDrawArraysInstancedARB_ptr) \
GLDEF(glDrawArraysInstancedBaseInstance, tsglDrawArraysInstancedBaseInstance, tsglDrawArraysInstancedBaseInstance_ptr) \
GLDEF(glDrawArraysInstancedBaseInstanceEXT, tsglDrawArraysInstancedBaseInstanceEXT, tsglDrawArraysInstancedBaseInstanceEXT_ptr) \
GLDEF(glDrawArraysInstancedEXT, tsglDrawArraysInstancedEXT, tsglDrawArraysInstancedEXT_ptr) \
GLDEF(glDrawArraysInstancedNV, tsglDrawArraysInstancedNV, tsglDrawArraysInstancedNV_ptr) \
GLDEF(glDrawBuffers, tsglDrawBuffers, tsglDrawBuffers_ptr) \
GLDEF(glDrawBuffersARB, tsglDrawBuffersARB, tsglDrawBuffersARB_ptr) \
GLDEF(glDrawBuffersEXT, tsglDrawBuffersEXT, tsglDrawBuffersEXT_ptr) \
GLDEF(glDrawElements, tsglDrawElements, tsglDrawElements_ptr) \
GLDEF(glDrawElementsInstanced, tsglDrawElementsInstanced, tsglDrawElementsInstanced_ptr) \
GLDEF(glDrawElementsInstancedANGLE, tsglDrawElementsInstancedANGLE, tsglDrawElementsInstancedANGLE_ptr) \
GLDEF(glDrawElementsInstancedARB, tsglDrawElementsInstancedARB, tsglDrawElementsInstancedARB_ptr) \
GLDEF(glDrawElementsInstancedBaseInstance, tsglDrawElementsInstancedBaseInstance, tsglDrawElementsInstancedBaseInstance_ptr) \
GLDEF(glDrawElementsInstancedBaseInstanceEXT, tsglDrawElementsInstancedBaseInstanceEXT, tsglDrawElementsInstancedBaseInstanceEXT_ptr) \
GLDEF(glDrawElementsInstancedEXT, tsglDrawElementsInstancedEXT, tsglDrawElementsInstancedEXT_ptr) \
GLDEF(glDrawElementsInstancedNV, tsglDrawElementsInstancedNV, tsglDrawElementsInstancedNV_ptr) \
GLDEF(glEnable, tsglEnable, tsglEnable_ptr) \
GLDEF(glEnableVertexAttribArray, tsglEnableVertexAttribArray, tsglEnableVertexAttribArray_ptr) \
GLDEF(glFramebufferTexture2D, tsglFramebufferTexture2D, tsglFramebufferTexture2D_ptr) \
GLDEF(glGenBuffers, tsglGenBuffers, tsglGenBuffers_ptr) \
GLDEF(glGenFramebuffers, tsglGenFramebuffers, tsglGenFramebuffers_ptr) \
GLDEF(glGenTextures, tsglGenTextures, tsglGenTextures_ptr) \
GLDEF(glGenVertexArrays, tsglGenVertexArrays, tsglGenVertexArrays_ptr) \
GLDEF(glGetActiveUniform, tsglGetActiveUniform, tsglGetActiveUniform_ptr) \
GLDEF(glGetIntegerv, tsglGetIntegerv, tsglGetIntegerv_ptr) \
GLDEF(glGetProgramInfoLog, tsglGetProgramInfoLog, tsglGetProgramInfoLog_ptr) \
GLDEF(glGetProgramiv, tsglGetProgramiv, tsglGetProgramiv_ptr) \
GLDEF(glGetShaderInfoLog, tsglGetShaderInfoLog, tsglGetShaderInfoLog_ptr) \
GLDEF(glGetShaderiv, tsglGetShaderiv, tsglGetShaderiv_ptr) \
GLDEF(glGetString, tsglGetString, tsglGetString_ptr) \
GLDEF(glGetStringi, tsglGetStringi, tsglGetStringi_ptr) \
GLDEF(glGetUniformLocation, tsglGetUniformLocation, tsglGetUniformLocation_ptr) \
GLDEF(glLinkProgram, tsglLinkProgram, tsglLinkProgram_ptr) \
GLDEF(glObjectLabel, tsglObjectLabel, tsglObjectLabel_ptr) \
GLDEF(glObjectLabelKHR, tsglObjectLabelKHR, tsglObjectLabelKHR_ptr) \
GLDEF(glPixelStorei, tsglPixelStorei, tsglPixelStorei_ptr) \
GLDEF(glReadBuffer, tsglReadBuffer, tsglReadBuffer_ptr) \
GLDEF(glReadPixels, tsglReadPixels, tsglReadPixels_ptr) \
GLDEF(glShaderSource, tsglShaderSource, tsglShaderSource_ptr) \
GLDEF(glTexImage2D, tsglTexImage2D, tsglTexImage2D_ptr) \
GLDEF(glTexParameterf, tsglTexParameterf, tsglTexParameterf_ptr) \
GLDEF(glTexParameteri, tsglTexParameteri, tsglTexParameteri_ptr) \
GLDEF(glTexSubImage2D, tsglTexSubImage2D, tsglTexSubImage2D_ptr) \
GLDEF(glUniform1fv, tsglUniform1fv, tsglUniform1fv_ptr) \
GLDEF(glUniform1iv, tsglUniform1iv, tsglUniform1iv_ptr) \
GLDEF(glUniform2fv, tsglUniform2fv, tsglUniform2fv_ptr) \
GLDEF(glUniform2iv, tsglUniform2iv, tsglUniform2iv_ptr) \
GLDEF(glUniform3fv, tsglUniform3fv, tsglUniform3fv_ptr) \
GLDEF(glUniform3iv, tsglUniform3iv, tsglUniform3iv_ptr) \
GLDEF(glUniform4fv, tsglUniform4fv, tsglUniform4fv_ptr) \
GLDEF(glUniform4iv, tsglUniform4iv, tsglUniform4iv_ptr) \
GLDEF(glUniformMatrix3fv, tsglUniformMatrix3fv, tsglUniformMatrix3fv_ptr) \
GLDEF(glUniformMatrix4fv, tsglUniformMatrix4fv, tsglUniformMatrix4fv_ptr) \
GLDEF(glUseProgram, tsglUseProgram, tsglUseProgram_ptr) \
GLDEF(glVertexAttribDivisor, tsglVertexAttribDivisor, tsglVertexAttribDivisor_ptr) \
GLDEF(glVertexAttribDivisorANGLE, tsglVertexAttribDivisorANGLE, tsglVertexAttribDivisorANGLE_ptr) \
GLDEF(glVertexAttribDivisorARB, tsglVertexAttribDivisorARB, tsglVertexAttribDivisorARB_ptr) \
GLDEF(glVertexAttribDivisorEXT, tsglVertexAttribDivisorEXT, tsglVertexAttribDivisorEXT_ptr) \
GLDEF(glVertexAttribDivisorNV, tsglVertexAttribDivisorNV, tsglVertexAttribDivisorNV_ptr) \
GLDEF(glVertexAttribIPointer, tsglVertexAttribIPointer, tsglVertexAttribIPointer_ptr) \
GLDEF(glVertexAttribPointer, tsglVertexAttribPointer, tsglVertexAttribPointer_ptr) \
GLDEF(glViewport, tsglViewport, tsglViewport_ptr)
// @END:gldefs@

#define GLDEF(glname,tsname,typename) extern typename tsname;
GLDEFS
#undef GLDEF

#endif // !LINK_TO_LIBGL

#ifdef LINK_TO_LIBGL
// @BEGIN:protos@
GLAPI void APIENTRY glActiveTexture (GLenum texture);
GLAPI void APIENTRY glAttachShader (GLuint program, GLuint shader);
GLAPI void APIENTRY glBindBuffer (GLenum target, GLuint buffer);
GLAPI void APIENTRY glBindFramebuffer (GLenum target, GLuint framebuffer);
GL_APICALL void GL_APIENTRY glBindTexture (GLenum target, GLuint texture);
GLAPI void APIENTRY glBindVertexArray (GLuint array);
GLAPI void APIENTRY glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha);
GLAPI void APIENTRY glBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GLAPI void APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
GLAPI void APIENTRY glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
GL_APICALL void GL_APIENTRY glClear (GLbitfield mask);
GL_APICALL void GL_APIENTRY glClearColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLAPI void APIENTRY glCompileShader (GLuint shader);
GLAPI GLuint APIENTRY glCreateProgram (void);
GLAPI GLuint APIENTRY glCreateShader (GLenum type);
GL_APICALL void GL_APIENTRY glCullFace (GLenum mode);
GLAPI void APIENTRY glDebugMessageCallback (GLDEBUGPROC callback, const void *userParam);
GLAPI void APIENTRY glDebugMessageCallbackARB (GLDEBUGPROCARB callback, const void *userParam);
GL_APICALL void GL_APIENTRY glDebugMessageCallbackKHR (GLDEBUGPROCKHR callback, const void *userParam);
GLAPI void APIENTRY glDebugMessageControl (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
GLAPI void APIENTRY glDebugMessageControlARB (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
GL_APICALL void GL_APIENTRY glDebugMessageControlKHR (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
GLAPI void APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers);
GLAPI void APIENTRY glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers);
GLAPI void APIENTRY glDeleteProgram (GLuint program);
GLAPI void APIENTRY glDeleteShader (GLuint shader);
GL_APICALL void GL_APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures);
GLAPI void APIENTRY glDeleteVertexArrays (GLsizei n, const GLuint *arrays);
GL_APICALL void GL_APIENTRY glDepthFunc (GLenum func);
GL_APICALL void GL_APIENTRY glDepthMask (GLboolean flag);
GL_APICALL void GL_APIENTRY glDisable (GLenum cap);
GLAPI void APIENTRY glDisableVertexAttribArray (GLuint index);
GL_APICALL void GL_APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count);
GLAPI void APIENTRY glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
GL_APICALL void GL_APIENTRY glDrawArraysInstancedANGLE (GLenum mode, GLint first, GLsizei count, GLsizei primcount);
GLAPI void APIENTRY glDrawArraysInstancedARB (GLenum mode, GLint first, GLsizei count, GLsizei primcount);
GLAPI void APIENTRY glDrawArraysInstancedBaseInstance (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
GL_APICALL void GL_APIENTRY glDrawArraysInstancedBaseInstanceEXT (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
GL_APICALL void GL_APIENTRY glDrawArraysInstancedEXT (GLenum mode, GLint start, GLsizei count, GLsizei primcount);
GL_APICALL void GL_APIENTRY glDrawArraysInstancedNV (GLenum mode, GLint first, GLsizei count, GLsizei primcount);
GLAPI void APIENTRY glDrawBuffers (GLsizei n, const GLenum *bufs);
GLAPI void APIENTRY glDrawBuffersARB (GLsizei n, const GLenum *bufs);
GL_APICALL void GL_APIENTRY glDrawBuffersEXT (GLsizei n, const GLenum *bufs);
GL_APICALL void GL_APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices);
GLAPI void APIENTRY glDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
GL_APICALL void GL_APIENTRY glDrawElementsInstancedANGLE (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
GLAPI void APIENTRY glDrawElementsInstancedARB (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
GLAPI void APIENTRY glDrawElementsInstancedBaseInstance (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
GL_APICALL void GL_APIENTRY glDrawElementsInstancedBaseInstanceEXT (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
GL_APICALL void GL_APIENTRY glDrawElementsInstancedEXT (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
GL_APICALL void GL_APIENTRY glDrawElementsInstancedNV (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
GL_APICALL void GL_APIENTRY glEnable (GLenum cap);
GLAPI void APIENTRY glEnableVertexAttribArray (GLuint index);
GLAPI void APIENTRY glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI void APIENTRY glGenBuffers (GLsizei n, GLuint *buffers);
GLAPI void APIENTRY glGenFramebuffers (GLsizei n, GLuint *framebuffers);
GL_APICALL void GL_APIENTRY glGenTextures (GLsizei n, GLuint *textures);
GLAPI void APIENTRY glGenVertexArrays (GLsizei n, GLuint *arrays);
GLAPI void APIENTRY glGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GL_APICALL void GL_APIENTRY glGetIntegerv (GLenum pname, GLint *data);
GLAPI void APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
GL_APICALL const GLubyte *GL_APIENTRY glGetString (GLenum name);
GLAPI const GLubyte *APIENTRY glGetStringi (GLenum name, GLuint index);
GLAPI GLint APIENTRY glGetUniformLocation (GLuint program, const GLchar *name);
GLAPI void APIENTRY glLinkProgram (GLuint program);
GLAPI void APIENTRY glObjectLabel (GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
GL_APICALL void GL_APIENTRY glObjectLabelKHR (GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
GL_APICALL void GL_APIENTRY glPixelStorei (GLenum pname, GLint param);
GL_APICALL void GL_APIENTRY glReadBuffer (GLenum src);
GL_APICALL void GL_APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
GLAPI void APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
GL_APICALL void GL_APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
GL_APICALL void GL_APIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param);
GL_APICALL void GL_APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param);
GL_APICALL void GL_APIENTRY glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
GLAPI void APIENTRY glUniform1fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform1iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform2fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform2iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform3iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform4fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform4iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLAPI void APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLAPI void APIENTRY glUseProgram (GLuint program);
GLAPI void APIENTRY glVertexAttribDivisor (GLuint index, GLuint divisor);
GL_APICALL void GL_APIENTRY glVertexAttribDivisorANGLE (GLuint index, GLuint divisor);
GLAPI void APIENTRY glVertexAttribDivisorARB (GLuint index, GLuint divisor);
GL_APICALL void GL_APIENTRY glVertexAttribDivisorEXT (GLuint index, GLuint divisor);
GL_APICALL void GL_APIENTRY glVertexAttribDivisorNV (GLuint index, GLuint divisor);
GLAPI void APIENTRY glVertexAttribIPointer (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
GLAPI void APIENTRY glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
GL_APICALL void GL_APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height);
// @END:protos@

// @BEGIN:reversedefs@
#define tsglActiveTexture glActiveTexture
#define tsglAttachShader glAttachShader
#define tsglBindBuffer glBindBuffer
#define tsglBindFramebuffer glBindFramebuffer
#define tsglBindTexture glBindTexture
#define tsglBindVertexArray glBindVertexArray
#define tsglBlendEquationSeparate glBlendEquationSeparate
#define tsglBlendFuncSeparate glBlendFuncSeparate
#define tsglBufferData glBufferData
#define tsglBufferSubData glBufferSubData
#define tsglClear glClear
#define tsglClearColor glClearColor
#define tsglCompileShader glCompileShader
#define tsglCreateProgram glCreateProgram
#define tsglCreateShader glCreateShader
#define tsglCullFace glCullFace
#define tsglDebugMessageCallback glDebugMessageCallback
#define tsglDebugMessageCallbackARB glDebugMessageCallbackARB
#define tsglDebugMessageCallbackKHR glDebugMessageCallbackKHR
#define tsglDebugMessageControl glDebugMessageControl
#define tsglDebugMessageControlARB glDebugMessageControlARB
#define tsglDebugMessageControlKHR glDebugMessageControlKHR
#define tsglDeleteBuffers glDeleteBuffers
#define tsglDeleteFramebuffers glDeleteFramebuffers
#define tsglDeleteProgram glDeleteProgram
#define tsglDeleteShader glDeleteShader
#define tsglDeleteTextures glDeleteTextures
#define tsglDeleteVertexArrays glDeleteVertexArrays
#define tsglDepthFunc glDepthFunc
#define tsglDepthMask glDepthMask
#define tsglDisable glDisable
#define tsglDisableVertexAttribArray glDisableVertexAttribArray
#define tsglDrawArrays glDrawArrays
#define tsglDrawArraysInstanced glDrawArraysInstanced
#define tsglDrawArraysInstancedANGLE glDrawArraysInstancedANGLE
#define tsglDrawArraysInstancedARB glDrawArraysInstancedARB
#define tsglDrawArraysInstancedBaseInstance glDrawArraysInstancedBaseInstance
#define tsglDrawArraysInstancedBaseInstanceEXT glDrawArraysInstancedBaseInstanceEXT
#define tsglDrawArraysInstancedEXT glDrawArraysInstancedEXT
#define tsglDrawArraysInstancedNV glDrawArraysInstancedNV
#define tsglDrawBuffers glDrawBuffers
#define tsglDrawBuffersARB glDrawBuffersARB
#define tsglDrawBuffersEXT glDrawBuffersEXT
#define tsglDrawElements glDrawElements
#define tsglDrawElementsInstanced glDrawElementsInstanced
#define tsglDrawElementsInstancedANGLE glDrawElementsInstancedANGLE
#define tsglDrawElementsInstancedARB glDrawElementsInstancedARB
#define tsglDrawElementsInstancedBaseInstance glDrawElementsInstancedBaseInstance
#define tsglDrawElementsInstancedBaseInstanceEXT glDrawElementsInstancedBaseInstanceEXT
#define tsglDrawElementsInstancedEXT glDrawElementsInstancedEXT
#define tsglDrawElementsInstancedNV glDrawElementsInstancedNV
#define tsglEnable glEnable
#define tsglEnableVertexAttribArray glEnableVertexAttribArray
#define tsglFramebufferTexture2D glFramebufferTexture2D
#define tsglGenBuffers glGenBuffers
#define tsglGenFramebuffers glGenFramebuffers
#define tsglGenTextures glGenTextures
#define tsglGenVertexArrays glGenVertexArrays
#define tsglGetActiveUniform glGetActiveUniform
#define tsglGetIntegerv glGetIntegerv
#define tsglGetProgramInfoLog glGetProgramInfoLog
#define tsglGetProgramiv glGetProgramiv
#define tsglGetShaderInfoLog glGetShaderInfoLog
#define tsglGetShaderiv glGetShaderiv
#define tsglGetString glGetString
#define tsglGetStringi glGetStringi
#define tsglGetUniformLocation glGetUniformLocation
#define tsglLinkProgram glLinkProgram
#define tsglObjectLabel glObjectLabel
#define tsglObjectLabelKHR glObjectLabelKHR
#define tsglPixelStorei glPixelStorei
#define tsglReadBuffer glReadBuffer
#define tsglReadPixels glReadPixels
#define tsglShaderSource glShaderSource
#define tsglTexImage2D glTexImage2D
#define tsglTexParameterf glTexParameterf
#define tsglTexParameteri glTexParameteri
#define tsglTexSubImage2D glTexSubImage2D
#define tsglUniform1fv glUniform1fv
#define tsglUniform1iv glUniform1iv
#define tsglUniform2fv glUniform2fv
#define tsglUniform2iv glUniform2iv
#define tsglUniform3fv glUniform3fv
#define tsglUniform3iv glUniform3iv
#define tsglUniform4fv glUniform4fv
#define tsglUniform4iv glUniform4iv
#define tsglUniformMatrix3fv glUniformMatrix3fv
#define tsglUniformMatrix4fv glUniformMatrix4fv
#define tsglUseProgram glUseProgram
#define tsglVertexAttribDivisor glVertexAttribDivisor
#define tsglVertexAttribDivisorANGLE glVertexAttribDivisorANGLE
#define tsglVertexAttribDivisorARB glVertexAttribDivisorARB
#define tsglVertexAttribDivisorEXT glVertexAttribDivisorEXT
#define tsglVertexAttribDivisorNV glVertexAttribDivisorNV
#define tsglVertexAttribIPointer glVertexAttribIPointer
#define tsglVertexAttribPointer glVertexAttribPointer
#define tsglViewport glViewport
// @END:reversedefs@
#endif // LINK_TO_LIBGL

struct glext_s {
	struct {
		char major;
		char minor;
		bool is_es;
	} version;

	ext_flag_t debug_output;
	ext_flag_t instanced_arrays;
	ext_flag_t base_instance;
	ext_flag_t pixel_buffer_object;
	ext_flag_t depth_texture;
	ext_flag_t draw_buffers;

	//
	// debug_output
	//

	tsglDebugMessageControl_ptr DebugMessageControl;
	#undef glDebugMessageControl
	#define glDebugMessageControl (glext.DebugMessageControl)

	tsglDebugMessageCallback_ptr DebugMessageCallback;
	#undef glDebugMessageCallback
	#define glDebugMessageCallback (glext.DebugMessageCallback)

	tsglObjectLabel_ptr ObjectLabel;
	#undef glObjectLabel
	#define glObjectLabel (glext.ObjectLabel)

	// instanced_arrays

	tsglDrawArraysInstanced_ptr DrawArraysInstanced;
	#undef glDrawArraysInstanced
	#define glDrawArraysInstanced (glext.DrawArraysInstanced)

	tsglDrawElementsInstanced_ptr DrawElementsInstanced;
	#undef glDrawElementsInstanced
	#define glDrawElementsInstanced (glext.DrawElementsInstanced)

	tsglVertexAttribDivisor_ptr VertexAttribDivisor;
	#undef glVertexAttribDivisor
	#define glVertexAttribDivisor (glext.VertexAttribDivisor)

	//
	// base_instance
	//

	tsglDrawArraysInstancedBaseInstance_ptr DrawArraysInstancedBaseInstance;
	#undef glDrawArraysInstancedBaseInstance
	#define glDrawArraysInstancedBaseInstance (glext.DrawArraysInstancedBaseInstance)

	tsglDrawElementsInstancedBaseInstance_ptr DrawElementsInstancedBaseInstance;
	#undef glDrawElementsInstancedBaseInstance
	#define glDrawElementsInstancedBaseInstance (glext.DrawElementsInstancedBaseInstance)

	//
	// draw_buffers
	//

	tsglDrawBuffers_ptr DrawBuffers;
	#undef glDrawBuffers
	#define glDrawBuffers (glext.DrawBuffers)
};

#undef GLANY_ATLEAST
#define GLANY_ATLEAST(mjr, mnr) (glext.version.major >= (mjr) && glext.version.minor >= (mnr))

#undef GL_ATLEAST
#define GL_ATLEAST(mjr, mnr) (!glext.version.is_es && GLANY_ATLEAST(mjr, mnr))

#undef GLES_ATLEAST
#define GLES_ATLEAST(mjr, mnr) (glext.version.is_es && GLANY_ATLEAST(mjr, mnr))
