/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "taiseigl.h"

#ifndef __APPLE__
#ifdef __WINDOWS__
// #include <GL/wgl.h>
#else
#include <GL/glx.h>
#endif
#endif

#include <string.h>
#include <stdio.h>

int tgl_ext[_TGLEXT_COUNT];

#ifndef __APPLE__
typedef void (*GLFuncPtr)(void);
GLFuncPtr get_proc_address(char *name) {
#ifdef __WINDOWS__
	return (GLFuncPtr)wglGetProcAddress(name);
#else
	return glXGetProcAddress((GLubyte *)name);
#endif
}
#endif


void check_gl_extensions(void) {
	int l;
	char *ext = (char*)glGetString(GL_EXTENSIONS);
	char *last, *pos;

	last = ext;
	pos = ext;
	while((pos = strchr(pos, ' '))) {
		pos++;

		l = pos - last - 1;

		if(strncmp(last, "GL_EXT_draw_instanced", l) == 0)
			tgl_ext[TGLEXT_draw_instanced] = 1;

		last = pos;
	}
}

void load_gl_functions(void) {
#ifdef __WINDOWS__
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)get_proc_address("glActiveTexture");
	glBlendEquation = (PFNGLBLENDEQUATIONPROC)get_proc_address("glBlendEquation");
#endif

#ifndef __APPLE__
	glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)get_proc_address("glBlendFuncSeparate");
	glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)get_proc_address("glDrawArraysInstanced");

	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)get_proc_address("glBindFramebuffer");
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)get_proc_address("glGenFramebuffers");
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)get_proc_address("glFramebufferTexture2D");
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)get_proc_address("glDeleteFramebuffers");

	glGenBuffers = (PFNGLGENBUFFERSPROC)get_proc_address("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)get_proc_address("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)get_proc_address("glBufferData");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)get_proc_address("glBufferSubData");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)get_proc_address("glDeleteBuffers");

	glCreateProgram = (PFNGLCREATEPROGRAMPROC)get_proc_address("glCreateProgram");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)get_proc_address("glLinkProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)get_proc_address("glUseProgram");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)get_proc_address("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)get_proc_address("glGetProgramInfoLog");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)get_proc_address("glDeleteProgram");

	glCreateShader = (PFNGLCREATESHADERPROC)get_proc_address("glCreateShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)get_proc_address("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)get_proc_address("glGetShaderInfoLog");
	glShaderSource = (PFNGLSHADERSOURCEPROC)get_proc_address("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)get_proc_address("glCompileShader");
	glAttachShader = (PFNGLATTACHSHADERPROC)get_proc_address("glAttachShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)get_proc_address("glDeleteShader");

	glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)get_proc_address("glGetActiveUniform");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)get_proc_address("glGetUniformLocation");

	glUniform1f = (PFNGLUNIFORM1FPROC)get_proc_address("glUniform1f");
	glUniform2f = (PFNGLUNIFORM2FPROC)get_proc_address("glUniform2f");
	glUniform3f = (PFNGLUNIFORM3FPROC)get_proc_address("glUniform3f");
	glUniform4f = (PFNGLUNIFORM4FPROC)get_proc_address("glUniform4f");

	glUniform1i = (PFNGLUNIFORM1IPROC)get_proc_address("glUniform1i");
	glUniform2i = (PFNGLUNIFORM2IPROC)get_proc_address("glUniform2i");
	glUniform3i = (PFNGLUNIFORM3IPROC)get_proc_address("glUniform3i");
	glUniform4i = (PFNGLUNIFORM4IPROC)get_proc_address("glUniform4i");

	glUniform2fv = (PFNGLUNIFORM2FVPROC)get_proc_address("glUniform2fv");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)get_proc_address("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)get_proc_address("glUniform4fv");
#endif
}
