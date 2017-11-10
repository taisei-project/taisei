#if 0
#
#   Run this file with python3 to update the defs.
#   It automatically picks up all the gl* functions used in Taisei code and updates itself.
#

# You can force the script to pick up functions that are not directly called in the code here

force_funcs = {
    'glDrawArraysInstancedEXT',
    'glDrawArraysInstancedARB',
    'glGetShaderiv',
    'glGetShaderInfoLog',
    'glGetProgramInfoLog',
    'glDebugMessageControlARB',
    'glDebugMessageCallbackARB',
}

force_funcs |= {"glUniform%i%sv" % (i, s) for i in range(1, 5) for s in ('f', 'i', 'ui')}

import sys, re
from pathlib import Path as P

thisfile = P(sys.argv[0])
srcdir = thisfile.parent
glfuncs = set()

try:
    idir = sys.argv[1]
except IndexError:
    idir = '/usr/include/SDL2'

glheaders = (
    P('%s/SDL_opengl.h' % idir).read_text() +
    P('%s/SDL_opengl_glext.h' % idir).read_text()
)

regex_glcall = re.compile(r'(gl[A-Z][a-zA-Z0-9]+)\(')
regex_glproto_template = r'(^GLAPI\s+([a-zA-Z_0-9\s]+?\**)\s*((?:GL)?APIENTRY)\s+%s\s*\(\s*(.*?)\s*\);)'

for src in srcdir.glob('**/*.c'):
    for func in regex_glcall.findall(src.read_text()):
        glfuncs.add(func)

glfuncs = sorted(glfuncs | force_funcs)

typedefs = []
prototypes = []

for func in glfuncs:
    try:
        proto, rtype, callconv, params = re.findall(regex_glproto_template % func, glheaders, re.DOTALL|re.MULTILINE)[0]
    except IndexError:
        print("Function '%s' not found in the GL headers. Either it really does not exist, or this script is bugged. Please check the opengl headers in %s\nUpdate aborted." % (func, idir))
        exit(1)

    proto = re.sub(r'\s+', ' ', proto, re.DOTALL|re.MULTILINE)
    params = ', '.join(re.split(r',\s*', params))

    typename = 'ts%s_ptr' % func
    typedef = 'typedef %s (%s *%s)(%s);' % (rtype, callconv, typename, params)

    typedefs.append(typedef)
    prototypes.append(proto)

text = thisfile.read_text()

subs = {
    'gldefs': '#define GLDEFS \\\n' + ' \\\n'.join('GLDEF(%s, ts%s, ts%s_ptr)' % (f, f, f) for f in glfuncs),
    'undefs': '\n'.join('#undef %s' % f for f in glfuncs),
    'redefs': '\n'.join('#define %s ts%s' % (f, f) for f in glfuncs),
    'reversedefs': '\n'.join('#define ts%s %s' % (f, f) for f in glfuncs),
    'typedefs': '\n'.join(typedefs),
    'protos': '\n'.join(prototypes),
}

for key, val in subs.items():
    text = re.sub(
        r'(// @BEGIN:%s@)(.*?)(// @END:%s@)' % (key, key),
        r'\1\n%s\n\3' % val,
        text,
        flags=re.DOTALL|re.MULTILINE
    )

thisfile.write_text(text)

#/*
"""
*/
#endif

/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef TAISEIGL_H
#define TAISEIGL_H

#include <SDL_platform.h>
#include <SDL_opengl.h>
// #include <SDL_opengl_glext.h>

struct glext_s; // defined at the very bottom
extern struct glext_s glext;

void load_gl_library(void);
void load_gl_functions(void);
void check_gl_extensions(void);
void unload_gl_library(void);

#define gluPerspective tsgluPerspective
void gluPerspective(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar);

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifdef DEBUG
#define DEBUG_GL
#endif

/*
 *
 *      From here on it's mostly generated code (by the embedded python script)
 *
 */

// @BEGIN:typedefs@
typedef void (GLAPIENTRY *tsglActiveTexture_ptr)(GLenum texture);
typedef void (APIENTRY *tsglAttachShader_ptr)(GLuint program, GLuint shader);
typedef void (APIENTRY *tsglBindBuffer_ptr)(GLenum target, GLuint buffer);
typedef void (APIENTRY *tsglBindFramebuffer_ptr)(GLenum target, GLuint framebuffer);
typedef void (GLAPIENTRY *tsglBindTexture_ptr)(GLenum target, GLuint texture);
typedef void (GLAPIENTRY *tsglBlendEquation_ptr)(GLenum mode);
typedef void (GLAPIENTRY *tsglBlendFunc_ptr)(GLenum sfactor, GLenum dfactor);
typedef void (APIENTRY *tsglBlendFuncSeparate_ptr)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void (APIENTRY *tsglBufferData_ptr)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRY *tsglBufferSubData_ptr)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (GLAPIENTRY *tsglClear_ptr)(GLbitfield mask);
typedef void (GLAPIENTRY *tsglClearColor_ptr)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (GLAPIENTRY *tsglColor3f_ptr)(GLfloat red, GLfloat green, GLfloat blue);
typedef void (GLAPIENTRY *tsglColor4f_ptr)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRY *tsglCompileShader_ptr)(GLuint shader);
typedef GLuint (APIENTRY *tsglCreateProgram_ptr)(void);
typedef GLuint (APIENTRY *tsglCreateShader_ptr)(GLenum type);
typedef void (GLAPIENTRY *tsglCullFace_ptr)(GLenum mode);
typedef void (APIENTRY *tsglDebugMessageCallback_ptr)(GLDEBUGPROC callback, const void *userParam);
typedef void (APIENTRY *tsglDebugMessageCallbackARB_ptr)(GLDEBUGPROCARB callback, const void *userParam);
typedef void (APIENTRY *tsglDebugMessageControl_ptr)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (APIENTRY *tsglDebugMessageControlARB_ptr)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (APIENTRY *tsglDeleteBuffers_ptr)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *tsglDeleteFramebuffers_ptr)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *tsglDeleteProgram_ptr)(GLuint program);
typedef void (APIENTRY *tsglDeleteShader_ptr)(GLuint shader);
typedef void (GLAPIENTRY *tsglDeleteTextures_ptr)(GLsizei n, const GLuint *textures);
typedef void (GLAPIENTRY *tsglDepthFunc_ptr)(GLenum func);
typedef void (GLAPIENTRY *tsglDepthMask_ptr)(GLboolean flag);
typedef void (GLAPIENTRY *tsglDisable_ptr)(GLenum cap);
typedef void (GLAPIENTRY *tsglDrawArrays_ptr)(GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY *tsglDrawArraysInstanced_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void (APIENTRY *tsglDrawArraysInstancedARB_ptr)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
typedef void (APIENTRY *tsglDrawArraysInstancedEXT_ptr)(GLenum mode, GLint start, GLsizei count, GLsizei primcount);
typedef void (GLAPIENTRY *tsglDrawElements_ptr)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (GLAPIENTRY *tsglEnable_ptr)(GLenum cap);
typedef void (GLAPIENTRY *tsglEnableClientState_ptr)(GLenum cap);
typedef void (APIENTRY *tsglFramebufferTexture2D_ptr)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (GLAPIENTRY *tsglFrustum_ptr)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (APIENTRY *tsglGenBuffers_ptr)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *tsglGenFramebuffers_ptr)(GLsizei n, GLuint *framebuffers);
typedef void (GLAPIENTRY *tsglGenTextures_ptr)(GLsizei n, GLuint *textures);
typedef void (APIENTRY *tsglGetActiveUniform_ptr)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void (GLAPIENTRY *tsglGetIntegerv_ptr)(GLenum pname, GLint *params);
typedef void (APIENTRY *tsglGetProgramInfoLog_ptr)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *tsglGetProgramiv_ptr)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *tsglGetShaderInfoLog_ptr)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRY *tsglGetShaderiv_ptr)(GLuint shader, GLenum pname, GLint *params);
typedef const GLubyte * (GLAPIENTRY *tsglGetString_ptr)(GLenum name);
typedef GLint (APIENTRY *tsglGetUniformLocation_ptr)(GLuint program, const GLchar *name);
typedef void (APIENTRY *tsglLinkProgram_ptr)(GLuint program);
typedef void (GLAPIENTRY *tsglLoadIdentity_ptr)(void);
typedef void * (APIENTRY *tsglMapBuffer_ptr)(GLenum target, GLenum access);
typedef void (GLAPIENTRY *tsglMatrixMode_ptr)(GLenum mode);
typedef void (GLAPIENTRY *tsglNormalPointer_ptr)(GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (GLAPIENTRY *tsglOrtho_ptr)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (GLAPIENTRY *tsglPopMatrix_ptr)(void);
typedef void (GLAPIENTRY *tsglPushMatrix_ptr)(void);
typedef void (GLAPIENTRY *tsglReadBuffer_ptr)(GLenum mode);
typedef void (GLAPIENTRY *tsglReadPixels_ptr)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
typedef void (GLAPIENTRY *tsglRotatef_ptr)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
typedef void (GLAPIENTRY *tsglScalef_ptr)(GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *tsglShaderSource_ptr)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (GLAPIENTRY *tsglTexCoordPointer_ptr)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (GLAPIENTRY *tsglTexImage2D_ptr)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (GLAPIENTRY *tsglTexParameterf_ptr)(GLenum target, GLenum pname, GLfloat param);
typedef void (GLAPIENTRY *tsglTexParameteri_ptr)(GLenum target, GLenum pname, GLint param);
typedef void (GLAPIENTRY *tsglTexSubImage2D_ptr)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (GLAPIENTRY *tsglTranslatef_ptr)(GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *tsglUniform1f_ptr)(GLint location, GLfloat v0);
typedef void (APIENTRY *tsglUniform1fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform1i_ptr)(GLint location, GLint v0);
typedef void (APIENTRY *tsglUniform1iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform1uiv_ptr)(GLint location, GLsizei count, const GLuint *value);
typedef void (APIENTRY *tsglUniform2f_ptr)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *tsglUniform2fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform2iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform2uiv_ptr)(GLint location, GLsizei count, const GLuint *value);
typedef void (APIENTRY *tsglUniform3f_ptr)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *tsglUniform3fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform3iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform3uiv_ptr)(GLint location, GLsizei count, const GLuint *value);
typedef void (APIENTRY *tsglUniform4f_ptr)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY *tsglUniform4fv_ptr)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY *tsglUniform4iv_ptr)(GLint location, GLsizei count, const GLint *value);
typedef void (APIENTRY *tsglUniform4uiv_ptr)(GLint location, GLsizei count, const GLuint *value);
typedef GLboolean (APIENTRY *tsglUnmapBuffer_ptr)(GLenum target);
typedef void (APIENTRY *tsglUseProgram_ptr)(GLuint program);
typedef void (GLAPIENTRY *tsglVertexPointer_ptr)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
typedef void (GLAPIENTRY *tsglViewport_ptr)(GLint x, GLint y, GLsizei width, GLsizei height);
// @END:typedefs@

// @BEGIN:undefs@
#undef glActiveTexture
#undef glAttachShader
#undef glBindBuffer
#undef glBindFramebuffer
#undef glBindTexture
#undef glBlendEquation
#undef glBlendFunc
#undef glBlendFuncSeparate
#undef glBufferData
#undef glBufferSubData
#undef glClear
#undef glClearColor
#undef glColor3f
#undef glColor4f
#undef glCompileShader
#undef glCreateProgram
#undef glCreateShader
#undef glCullFace
#undef glDebugMessageCallback
#undef glDebugMessageCallbackARB
#undef glDebugMessageControl
#undef glDebugMessageControlARB
#undef glDeleteBuffers
#undef glDeleteFramebuffers
#undef glDeleteProgram
#undef glDeleteShader
#undef glDeleteTextures
#undef glDepthFunc
#undef glDepthMask
#undef glDisable
#undef glDrawArrays
#undef glDrawArraysInstanced
#undef glDrawArraysInstancedARB
#undef glDrawArraysInstancedEXT
#undef glDrawElements
#undef glEnable
#undef glEnableClientState
#undef glFramebufferTexture2D
#undef glFrustum
#undef glGenBuffers
#undef glGenFramebuffers
#undef glGenTextures
#undef glGetActiveUniform
#undef glGetIntegerv
#undef glGetProgramInfoLog
#undef glGetProgramiv
#undef glGetShaderInfoLog
#undef glGetShaderiv
#undef glGetString
#undef glGetUniformLocation
#undef glLinkProgram
#undef glLoadIdentity
#undef glMapBuffer
#undef glMatrixMode
#undef glNormalPointer
#undef glOrtho
#undef glPopMatrix
#undef glPushMatrix
#undef glReadBuffer
#undef glReadPixels
#undef glRotatef
#undef glScalef
#undef glShaderSource
#undef glTexCoordPointer
#undef glTexImage2D
#undef glTexParameterf
#undef glTexParameteri
#undef glTexSubImage2D
#undef glTranslatef
#undef glUniform1f
#undef glUniform1fv
#undef glUniform1i
#undef glUniform1iv
#undef glUniform1uiv
#undef glUniform2f
#undef glUniform2fv
#undef glUniform2iv
#undef glUniform2uiv
#undef glUniform3f
#undef glUniform3fv
#undef glUniform3iv
#undef glUniform3uiv
#undef glUniform4f
#undef glUniform4fv
#undef glUniform4iv
#undef glUniform4uiv
#undef glUnmapBuffer
#undef glUseProgram
#undef glVertexPointer
#undef glViewport
// @END:undefs@

#ifndef LINK_TO_LIBGL
// @BEGIN:redefs@
#define glActiveTexture tsglActiveTexture
#define glAttachShader tsglAttachShader
#define glBindBuffer tsglBindBuffer
#define glBindFramebuffer tsglBindFramebuffer
#define glBindTexture tsglBindTexture
#define glBlendEquation tsglBlendEquation
#define glBlendFunc tsglBlendFunc
#define glBlendFuncSeparate tsglBlendFuncSeparate
#define glBufferData tsglBufferData
#define glBufferSubData tsglBufferSubData
#define glClear tsglClear
#define glClearColor tsglClearColor
#define glColor3f tsglColor3f
#define glColor4f tsglColor4f
#define glCompileShader tsglCompileShader
#define glCreateProgram tsglCreateProgram
#define glCreateShader tsglCreateShader
#define glCullFace tsglCullFace
#define glDebugMessageCallback tsglDebugMessageCallback
#define glDebugMessageCallbackARB tsglDebugMessageCallbackARB
#define glDebugMessageControl tsglDebugMessageControl
#define glDebugMessageControlARB tsglDebugMessageControlARB
#define glDeleteBuffers tsglDeleteBuffers
#define glDeleteFramebuffers tsglDeleteFramebuffers
#define glDeleteProgram tsglDeleteProgram
#define glDeleteShader tsglDeleteShader
#define glDeleteTextures tsglDeleteTextures
#define glDepthFunc tsglDepthFunc
#define glDepthMask tsglDepthMask
#define glDisable tsglDisable
#define glDrawArrays tsglDrawArrays
#define glDrawArraysInstanced tsglDrawArraysInstanced
#define glDrawArraysInstancedARB tsglDrawArraysInstancedARB
#define glDrawArraysInstancedEXT tsglDrawArraysInstancedEXT
#define glDrawElements tsglDrawElements
#define glEnable tsglEnable
#define glEnableClientState tsglEnableClientState
#define glFramebufferTexture2D tsglFramebufferTexture2D
#define glFrustum tsglFrustum
#define glGenBuffers tsglGenBuffers
#define glGenFramebuffers tsglGenFramebuffers
#define glGenTextures tsglGenTextures
#define glGetActiveUniform tsglGetActiveUniform
#define glGetIntegerv tsglGetIntegerv
#define glGetProgramInfoLog tsglGetProgramInfoLog
#define glGetProgramiv tsglGetProgramiv
#define glGetShaderInfoLog tsglGetShaderInfoLog
#define glGetShaderiv tsglGetShaderiv
#define glGetString tsglGetString
#define glGetUniformLocation tsglGetUniformLocation
#define glLinkProgram tsglLinkProgram
#define glLoadIdentity tsglLoadIdentity
#define glMapBuffer tsglMapBuffer
#define glMatrixMode tsglMatrixMode
#define glNormalPointer tsglNormalPointer
#define glOrtho tsglOrtho
#define glPopMatrix tsglPopMatrix
#define glPushMatrix tsglPushMatrix
#define glReadBuffer tsglReadBuffer
#define glReadPixels tsglReadPixels
#define glRotatef tsglRotatef
#define glScalef tsglScalef
#define glShaderSource tsglShaderSource
#define glTexCoordPointer tsglTexCoordPointer
#define glTexImage2D tsglTexImage2D
#define glTexParameterf tsglTexParameterf
#define glTexParameteri tsglTexParameteri
#define glTexSubImage2D tsglTexSubImage2D
#define glTranslatef tsglTranslatef
#define glUniform1f tsglUniform1f
#define glUniform1fv tsglUniform1fv
#define glUniform1i tsglUniform1i
#define glUniform1iv tsglUniform1iv
#define glUniform1uiv tsglUniform1uiv
#define glUniform2f tsglUniform2f
#define glUniform2fv tsglUniform2fv
#define glUniform2iv tsglUniform2iv
#define glUniform2uiv tsglUniform2uiv
#define glUniform3f tsglUniform3f
#define glUniform3fv tsglUniform3fv
#define glUniform3iv tsglUniform3iv
#define glUniform3uiv tsglUniform3uiv
#define glUniform4f tsglUniform4f
#define glUniform4fv tsglUniform4fv
#define glUniform4iv tsglUniform4iv
#define glUniform4uiv tsglUniform4uiv
#define glUnmapBuffer tsglUnmapBuffer
#define glUseProgram tsglUseProgram
#define glVertexPointer tsglVertexPointer
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
GLDEF(glBlendEquation, tsglBlendEquation, tsglBlendEquation_ptr) \
GLDEF(glBlendFunc, tsglBlendFunc, tsglBlendFunc_ptr) \
GLDEF(glBlendFuncSeparate, tsglBlendFuncSeparate, tsglBlendFuncSeparate_ptr) \
GLDEF(glBufferData, tsglBufferData, tsglBufferData_ptr) \
GLDEF(glBufferSubData, tsglBufferSubData, tsglBufferSubData_ptr) \
GLDEF(glClear, tsglClear, tsglClear_ptr) \
GLDEF(glClearColor, tsglClearColor, tsglClearColor_ptr) \
GLDEF(glColor3f, tsglColor3f, tsglColor3f_ptr) \
GLDEF(glColor4f, tsglColor4f, tsglColor4f_ptr) \
GLDEF(glCompileShader, tsglCompileShader, tsglCompileShader_ptr) \
GLDEF(glCreateProgram, tsglCreateProgram, tsglCreateProgram_ptr) \
GLDEF(glCreateShader, tsglCreateShader, tsglCreateShader_ptr) \
GLDEF(glCullFace, tsglCullFace, tsglCullFace_ptr) \
GLDEF(glDebugMessageCallback, tsglDebugMessageCallback, tsglDebugMessageCallback_ptr) \
GLDEF(glDebugMessageCallbackARB, tsglDebugMessageCallbackARB, tsglDebugMessageCallbackARB_ptr) \
GLDEF(glDebugMessageControl, tsglDebugMessageControl, tsglDebugMessageControl_ptr) \
GLDEF(glDebugMessageControlARB, tsglDebugMessageControlARB, tsglDebugMessageControlARB_ptr) \
GLDEF(glDeleteBuffers, tsglDeleteBuffers, tsglDeleteBuffers_ptr) \
GLDEF(glDeleteFramebuffers, tsglDeleteFramebuffers, tsglDeleteFramebuffers_ptr) \
GLDEF(glDeleteProgram, tsglDeleteProgram, tsglDeleteProgram_ptr) \
GLDEF(glDeleteShader, tsglDeleteShader, tsglDeleteShader_ptr) \
GLDEF(glDeleteTextures, tsglDeleteTextures, tsglDeleteTextures_ptr) \
GLDEF(glDepthFunc, tsglDepthFunc, tsglDepthFunc_ptr) \
GLDEF(glDepthMask, tsglDepthMask, tsglDepthMask_ptr) \
GLDEF(glDisable, tsglDisable, tsglDisable_ptr) \
GLDEF(glDrawArrays, tsglDrawArrays, tsglDrawArrays_ptr) \
GLDEF(glDrawArraysInstanced, tsglDrawArraysInstanced, tsglDrawArraysInstanced_ptr) \
GLDEF(glDrawArraysInstancedARB, tsglDrawArraysInstancedARB, tsglDrawArraysInstancedARB_ptr) \
GLDEF(glDrawArraysInstancedEXT, tsglDrawArraysInstancedEXT, tsglDrawArraysInstancedEXT_ptr) \
GLDEF(glDrawElements, tsglDrawElements, tsglDrawElements_ptr) \
GLDEF(glEnable, tsglEnable, tsglEnable_ptr) \
GLDEF(glEnableClientState, tsglEnableClientState, tsglEnableClientState_ptr) \
GLDEF(glFramebufferTexture2D, tsglFramebufferTexture2D, tsglFramebufferTexture2D_ptr) \
GLDEF(glFrustum, tsglFrustum, tsglFrustum_ptr) \
GLDEF(glGenBuffers, tsglGenBuffers, tsglGenBuffers_ptr) \
GLDEF(glGenFramebuffers, tsglGenFramebuffers, tsglGenFramebuffers_ptr) \
GLDEF(glGenTextures, tsglGenTextures, tsglGenTextures_ptr) \
GLDEF(glGetActiveUniform, tsglGetActiveUniform, tsglGetActiveUniform_ptr) \
GLDEF(glGetIntegerv, tsglGetIntegerv, tsglGetIntegerv_ptr) \
GLDEF(glGetProgramInfoLog, tsglGetProgramInfoLog, tsglGetProgramInfoLog_ptr) \
GLDEF(glGetProgramiv, tsglGetProgramiv, tsglGetProgramiv_ptr) \
GLDEF(glGetShaderInfoLog, tsglGetShaderInfoLog, tsglGetShaderInfoLog_ptr) \
GLDEF(glGetShaderiv, tsglGetShaderiv, tsglGetShaderiv_ptr) \
GLDEF(glGetString, tsglGetString, tsglGetString_ptr) \
GLDEF(glGetUniformLocation, tsglGetUniformLocation, tsglGetUniformLocation_ptr) \
GLDEF(glLinkProgram, tsglLinkProgram, tsglLinkProgram_ptr) \
GLDEF(glLoadIdentity, tsglLoadIdentity, tsglLoadIdentity_ptr) \
GLDEF(glMapBuffer, tsglMapBuffer, tsglMapBuffer_ptr) \
GLDEF(glMatrixMode, tsglMatrixMode, tsglMatrixMode_ptr) \
GLDEF(glNormalPointer, tsglNormalPointer, tsglNormalPointer_ptr) \
GLDEF(glOrtho, tsglOrtho, tsglOrtho_ptr) \
GLDEF(glPopMatrix, tsglPopMatrix, tsglPopMatrix_ptr) \
GLDEF(glPushMatrix, tsglPushMatrix, tsglPushMatrix_ptr) \
GLDEF(glReadBuffer, tsglReadBuffer, tsglReadBuffer_ptr) \
GLDEF(glReadPixels, tsglReadPixels, tsglReadPixels_ptr) \
GLDEF(glRotatef, tsglRotatef, tsglRotatef_ptr) \
GLDEF(glScalef, tsglScalef, tsglScalef_ptr) \
GLDEF(glShaderSource, tsglShaderSource, tsglShaderSource_ptr) \
GLDEF(glTexCoordPointer, tsglTexCoordPointer, tsglTexCoordPointer_ptr) \
GLDEF(glTexImage2D, tsglTexImage2D, tsglTexImage2D_ptr) \
GLDEF(glTexParameterf, tsglTexParameterf, tsglTexParameterf_ptr) \
GLDEF(glTexParameteri, tsglTexParameteri, tsglTexParameteri_ptr) \
GLDEF(glTexSubImage2D, tsglTexSubImage2D, tsglTexSubImage2D_ptr) \
GLDEF(glTranslatef, tsglTranslatef, tsglTranslatef_ptr) \
GLDEF(glUniform1f, tsglUniform1f, tsglUniform1f_ptr) \
GLDEF(glUniform1fv, tsglUniform1fv, tsglUniform1fv_ptr) \
GLDEF(glUniform1i, tsglUniform1i, tsglUniform1i_ptr) \
GLDEF(glUniform1iv, tsglUniform1iv, tsglUniform1iv_ptr) \
GLDEF(glUniform1uiv, tsglUniform1uiv, tsglUniform1uiv_ptr) \
GLDEF(glUniform2f, tsglUniform2f, tsglUniform2f_ptr) \
GLDEF(glUniform2fv, tsglUniform2fv, tsglUniform2fv_ptr) \
GLDEF(glUniform2iv, tsglUniform2iv, tsglUniform2iv_ptr) \
GLDEF(glUniform2uiv, tsglUniform2uiv, tsglUniform2uiv_ptr) \
GLDEF(glUniform3f, tsglUniform3f, tsglUniform3f_ptr) \
GLDEF(glUniform3fv, tsglUniform3fv, tsglUniform3fv_ptr) \
GLDEF(glUniform3iv, tsglUniform3iv, tsglUniform3iv_ptr) \
GLDEF(glUniform3uiv, tsglUniform3uiv, tsglUniform3uiv_ptr) \
GLDEF(glUniform4f, tsglUniform4f, tsglUniform4f_ptr) \
GLDEF(glUniform4fv, tsglUniform4fv, tsglUniform4fv_ptr) \
GLDEF(glUniform4iv, tsglUniform4iv, tsglUniform4iv_ptr) \
GLDEF(glUniform4uiv, tsglUniform4uiv, tsglUniform4uiv_ptr) \
GLDEF(glUnmapBuffer, tsglUnmapBuffer, tsglUnmapBuffer_ptr) \
GLDEF(glUseProgram, tsglUseProgram, tsglUseProgram_ptr) \
GLDEF(glVertexPointer, tsglVertexPointer, tsglVertexPointer_ptr) \
GLDEF(glViewport, tsglViewport, tsglViewport_ptr)
// @END:gldefs@

#define GLDEF(glname,tsname,typename) extern typename tsname;
GLDEFS
#undef GLDEF

#endif // !LINK_TO_LIBGL

#ifdef LINK_TO_LIBGL
// @BEGIN:protos@
GLAPI void GLAPIENTRY glActiveTexture( GLenum texture );
GLAPI void APIENTRY glAttachShader (GLuint program, GLuint shader);
GLAPI void APIENTRY glBindBuffer (GLenum target, GLuint buffer);
GLAPI void APIENTRY glBindFramebuffer (GLenum target, GLuint framebuffer);
GLAPI void GLAPIENTRY glBindTexture( GLenum target, GLuint texture );
GLAPI void GLAPIENTRY glBlendEquation( GLenum mode );
GLAPI void GLAPIENTRY glBlendFunc( GLenum sfactor, GLenum dfactor );
GLAPI void APIENTRY glBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GLAPI void APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
GLAPI void APIENTRY glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
GLAPI void GLAPIENTRY glClear( GLbitfield mask );
GLAPI void GLAPIENTRY glClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
GLAPI void GLAPIENTRY glColor3f( GLfloat red, GLfloat green, GLfloat blue );
GLAPI void GLAPIENTRY glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
GLAPI void APIENTRY glCompileShader (GLuint shader);
GLAPI GLuint APIENTRY glCreateProgram (void);
GLAPI GLuint APIENTRY glCreateShader (GLenum type);
GLAPI void GLAPIENTRY glCullFace( GLenum mode );
GLAPI void APIENTRY glDebugMessageCallback (GLDEBUGPROC callback, const void *userParam);
GLAPI void APIENTRY glDebugMessageCallbackARB (GLDEBUGPROCARB callback, const void *userParam);
GLAPI void APIENTRY glDebugMessageControl (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
GLAPI void APIENTRY glDebugMessageControlARB (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
GLAPI void APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers);
GLAPI void APIENTRY glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers);
GLAPI void APIENTRY glDeleteProgram (GLuint program);
GLAPI void APIENTRY glDeleteShader (GLuint shader);
GLAPI void GLAPIENTRY glDeleteTextures( GLsizei n, const GLuint *textures);
GLAPI void GLAPIENTRY glDepthFunc( GLenum func );
GLAPI void GLAPIENTRY glDepthMask( GLboolean flag );
GLAPI void GLAPIENTRY glDisable( GLenum cap );
GLAPI void GLAPIENTRY glDrawArrays( GLenum mode, GLint first, GLsizei count );
GLAPI void APIENTRY glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
GLAPI void APIENTRY glDrawArraysInstancedARB (GLenum mode, GLint first, GLsizei count, GLsizei primcount);
GLAPI void APIENTRY glDrawArraysInstancedEXT (GLenum mode, GLint start, GLsizei count, GLsizei primcount);
GLAPI void GLAPIENTRY glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
GLAPI void GLAPIENTRY glEnable( GLenum cap );
GLAPI void GLAPIENTRY glEnableClientState( GLenum cap );
GLAPI void APIENTRY glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI void GLAPIENTRY glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
GLAPI void APIENTRY glGenBuffers (GLsizei n, GLuint *buffers);
GLAPI void APIENTRY glGenFramebuffers (GLsizei n, GLuint *framebuffers);
GLAPI void GLAPIENTRY glGenTextures( GLsizei n, GLuint *textures );
GLAPI void APIENTRY glGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GLAPI void GLAPIENTRY glGetIntegerv( GLenum pname, GLint *params );
GLAPI void APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLAPI void APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
GLAPI const GLubyte * GLAPIENTRY glGetString( GLenum name );
GLAPI GLint APIENTRY glGetUniformLocation (GLuint program, const GLchar *name);
GLAPI void APIENTRY glLinkProgram (GLuint program);
GLAPI void GLAPIENTRY glLoadIdentity( void );
GLAPI void *APIENTRY glMapBuffer (GLenum target, GLenum access);
GLAPI void GLAPIENTRY glMatrixMode( GLenum mode );
GLAPI void GLAPIENTRY glNormalPointer( GLenum type, GLsizei stride, const GLvoid *ptr );
GLAPI void GLAPIENTRY glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val );
GLAPI void GLAPIENTRY glPopMatrix( void );
GLAPI void GLAPIENTRY glPushMatrix( void );
GLAPI void GLAPIENTRY glReadBuffer( GLenum mode );
GLAPI void GLAPIENTRY glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
GLAPI void GLAPIENTRY glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
GLAPI void GLAPIENTRY glScalef( GLfloat x, GLfloat y, GLfloat z );
GLAPI void APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
GLAPI void GLAPIENTRY glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr );
GLAPI void GLAPIENTRY glTexImage2D( GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
GLAPI void GLAPIENTRY glTexParameterf( GLenum target, GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glTexParameteri( GLenum target, GLenum pname, GLint param );
GLAPI void GLAPIENTRY glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
GLAPI void GLAPIENTRY glTranslatef( GLfloat x, GLfloat y, GLfloat z );
GLAPI void APIENTRY glUniform1f (GLint location, GLfloat v0);
GLAPI void APIENTRY glUniform1fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform1i (GLint location, GLint v0);
GLAPI void APIENTRY glUniform1iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform1uiv (GLint location, GLsizei count, const GLuint *value);
GLAPI void APIENTRY glUniform2f (GLint location, GLfloat v0, GLfloat v1);
GLAPI void APIENTRY glUniform2fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform2iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform2uiv (GLint location, GLsizei count, const GLuint *value);
GLAPI void APIENTRY glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GLAPI void APIENTRY glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform3iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform3uiv (GLint location, GLsizei count, const GLuint *value);
GLAPI void APIENTRY glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLAPI void APIENTRY glUniform4fv (GLint location, GLsizei count, const GLfloat *value);
GLAPI void APIENTRY glUniform4iv (GLint location, GLsizei count, const GLint *value);
GLAPI void APIENTRY glUniform4uiv (GLint location, GLsizei count, const GLuint *value);
GLAPI GLboolean APIENTRY glUnmapBuffer (GLenum target);
GLAPI void APIENTRY glUseProgram (GLuint program);
GLAPI void GLAPIENTRY glVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr );
GLAPI void GLAPIENTRY glViewport( GLint x, GLint y, GLsizei width, GLsizei height );
// @END:protos@

// @BEGIN:reversedefs@
#define tsglActiveTexture glActiveTexture
#define tsglAttachShader glAttachShader
#define tsglBindBuffer glBindBuffer
#define tsglBindFramebuffer glBindFramebuffer
#define tsglBindTexture glBindTexture
#define tsglBlendEquation glBlendEquation
#define tsglBlendFunc glBlendFunc
#define tsglBlendFuncSeparate glBlendFuncSeparate
#define tsglBufferData glBufferData
#define tsglBufferSubData glBufferSubData
#define tsglClear glClear
#define tsglClearColor glClearColor
#define tsglColor3f glColor3f
#define tsglColor4f glColor4f
#define tsglCompileShader glCompileShader
#define tsglCreateProgram glCreateProgram
#define tsglCreateShader glCreateShader
#define tsglCullFace glCullFace
#define tsglDebugMessageCallback glDebugMessageCallback
#define tsglDebugMessageCallbackARB glDebugMessageCallbackARB
#define tsglDebugMessageControl glDebugMessageControl
#define tsglDebugMessageControlARB glDebugMessageControlARB
#define tsglDeleteBuffers glDeleteBuffers
#define tsglDeleteFramebuffers glDeleteFramebuffers
#define tsglDeleteProgram glDeleteProgram
#define tsglDeleteShader glDeleteShader
#define tsglDeleteTextures glDeleteTextures
#define tsglDepthFunc glDepthFunc
#define tsglDepthMask glDepthMask
#define tsglDisable glDisable
#define tsglDrawArrays glDrawArrays
#define tsglDrawArraysInstanced glDrawArraysInstanced
#define tsglDrawArraysInstancedARB glDrawArraysInstancedARB
#define tsglDrawArraysInstancedEXT glDrawArraysInstancedEXT
#define tsglDrawElements glDrawElements
#define tsglEnable glEnable
#define tsglEnableClientState glEnableClientState
#define tsglFramebufferTexture2D glFramebufferTexture2D
#define tsglFrustum glFrustum
#define tsglGenBuffers glGenBuffers
#define tsglGenFramebuffers glGenFramebuffers
#define tsglGenTextures glGenTextures
#define tsglGetActiveUniform glGetActiveUniform
#define tsglGetIntegerv glGetIntegerv
#define tsglGetProgramInfoLog glGetProgramInfoLog
#define tsglGetProgramiv glGetProgramiv
#define tsglGetShaderInfoLog glGetShaderInfoLog
#define tsglGetShaderiv glGetShaderiv
#define tsglGetString glGetString
#define tsglGetUniformLocation glGetUniformLocation
#define tsglLinkProgram glLinkProgram
#define tsglLoadIdentity glLoadIdentity
#define tsglMapBuffer glMapBuffer
#define tsglMatrixMode glMatrixMode
#define tsglNormalPointer glNormalPointer
#define tsglOrtho glOrtho
#define tsglPopMatrix glPopMatrix
#define tsglPushMatrix glPushMatrix
#define tsglReadBuffer glReadBuffer
#define tsglReadPixels glReadPixels
#define tsglRotatef glRotatef
#define tsglScalef glScalef
#define tsglShaderSource glShaderSource
#define tsglTexCoordPointer glTexCoordPointer
#define tsglTexImage2D glTexImage2D
#define tsglTexParameterf glTexParameterf
#define tsglTexParameteri glTexParameteri
#define tsglTexSubImage2D glTexSubImage2D
#define tsglTranslatef glTranslatef
#define tsglUniform1f glUniform1f
#define tsglUniform1fv glUniform1fv
#define tsglUniform1i glUniform1i
#define tsglUniform1iv glUniform1iv
#define tsglUniform1uiv glUniform1uiv
#define tsglUniform2f glUniform2f
#define tsglUniform2fv glUniform2fv
#define tsglUniform2iv glUniform2iv
#define tsglUniform2uiv glUniform2uiv
#define tsglUniform3f glUniform3f
#define tsglUniform3fv glUniform3fv
#define tsglUniform3iv glUniform3iv
#define tsglUniform3uiv glUniform3uiv
#define tsglUniform4f glUniform4f
#define tsglUniform4fv glUniform4fv
#define tsglUniform4iv glUniform4iv
#define tsglUniform4uiv glUniform4uiv
#define tsglUnmapBuffer glUnmapBuffer
#define tsglUseProgram glUseProgram
#define tsglVertexPointer glVertexPointer
#define tsglViewport glViewport
// @END:reversedefs@
#endif // LINK_TO_LIBGL

struct glext_s {
    struct {
        char major;
        char minor;
    } version;

    unsigned int draw_instanced: 1;
    unsigned int debug_output: 1;
    unsigned int EXT_draw_instanced: 1;
    unsigned int ARB_draw_instanced: 1;

    tsglDrawArraysInstanced_ptr DrawArraysInstanced;
    tsglDebugMessageControl_ptr DebugMessageControl;
    tsglDebugMessageCallback_ptr DebugMessageCallback;
};

#endif // !TAISEIGL_H

// Intentionally not guarded

#ifndef TAISEIGL_NO_EXT_ABSTRACTION
    #undef glDrawArraysInstanced
    #undef glDebugMessageControl
    #undef glDebugMessageCallback
    #define glDrawArraysInstanced (glext.DrawArraysInstanced)
    #define glDebugMessageControl (glext.DebugMessageControl)
    #define glDebugMessageCallback (glext.DebugMessageCallback)
#elif defined(LINK_TO_LIBGL)
    #undef glDrawArraysInstanced
    #undef glDebugMessageControl
    #undef glDebugMessageCallback
#endif // !TAISEIGL_NO_EXT_ABSTRACTION

// Don't even think about touching the construct below
/*
"""
#*/
