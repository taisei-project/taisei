/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "debug.h"
#include "util.h"
#include "../api.h"

#ifndef STATIC_GLES3

static void APIENTRY glcommon_debug(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *arg
) {
	char *strsrc = "unknown";
	char *strtype = "unknown";
	char *strsev = "unknown";
	LogLevel lvl0 = LOG_DEBUG;
	LogLevel lvl1 = LOG_DEBUG;
	LogLevel lvl;

	switch(source) {
		case GL_DEBUG_SOURCE_API:               strsrc  = "api";                           break;
		case GL_DEBUG_SOURCE_APPLICATION:       strsrc  = "app";                           break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:   strsrc  = "shaderc";                       break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:       strsrc  = "extern";                        break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     strsrc  = "winsys";                        break;
		case GL_DEBUG_SOURCE_OTHER:             strsrc  = "other";                         break;
	}

	switch(type) {
		case GL_DEBUG_TYPE_ERROR:               strtype = "error";       lvl0 = LOG_FATAL; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: strtype = "deprecated";  lvl0 = LOG_WARN;  break;
		case GL_DEBUG_TYPE_PORTABILITY:         strtype = "portability"; lvl0 = LOG_WARN;  break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  strtype = "undefined";   lvl0 = LOG_WARN;  break;
		case GL_DEBUG_TYPE_PERFORMANCE:         strtype = "performance"; lvl0 = LOG_WARN;  break;
		case GL_DEBUG_TYPE_OTHER:               strtype = "other";                         break;
	}

	switch(severity) {
		case GL_DEBUG_SEVERITY_LOW:             strsev  = "low";         lvl1 = LOG_INFO;  break;
		case GL_DEBUG_SEVERITY_MEDIUM:          strsev  = "medium";      lvl1 = LOG_WARN;  break;
		case GL_DEBUG_SEVERITY_HIGH:            strsev  = "high";        lvl1 = LOG_FATAL; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:    strsev  = "notify";                        break;
	}

	if(source == GL_DEBUG_SOURCE_SHADER_COMPILER && lvl1 == LOG_FATAL) {
		// XXX: ignore a nonsense warning from mesa triggered by some shaders cross-compiled with SPIRV-cross
		// warning: `fragColor' used uninitialized
		lvl1 = LOG_WARN;
	}

	lvl = lvl1 > lvl0 ? lvl1 : lvl0;

	if(source == GL_DEBUG_SOURCE_SHADER_COMPILER && type == GL_DEBUG_TYPE_ERROR) {
		lvl = LOG_WARN;
	}

	if(lvl == LOG_FATAL && glext.version.is_ANGLE && !strcmp(message, "Invalid operation.")) {
		// HACK: Workaround for an ANGLE bug. Seems like it always reports the original array size for
		// uniform arrays, even if the arrays has been partially optimized out. Accessing the non-existent
		// elements then generates a GL_INVALID_OPERATION error, which should be safe to ignore.
		lvl = LOG_ERROR;
	}

	if(lvl == LOG_FATAL && glext.version.is_ANGLE && !strcmp(message, "Invalid uniform location")) {
		// seriously?
		lvl = LOG_WARN;
	}

	log_custom(lvl, "[%s, %s, %s, id:%i] %s", strsrc, strtype, strsev, id, message);
}

void glcommon_debug_enable(void) {
	if(!glext.debug_output) {
		log_error("OpenGL debugging is not supported on your system");
		return;
	}

	GLuint unused = 0;
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glcommon_debug, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, true);
	log_info("Enabled OpenGL debugging");
}

void glcommon_set_debug_label_gl(GLenum identifier, GLuint name, const char *label) {
	if(glext.debug_output & (TSGL_EXTFLAG_NATIVE | TSGL_EXTFLAG_KHR)) {
		glObjectLabel(identifier, name, strlen(label), label);
	}
}

#else

void glcommon_set_debug_label_gl(GLenum identifier, GLuint name, const char *label) { }

void glcommon_debug_enable(void) {
	log_error("OpenGL debugging is not supported on your system");
}

#endif

bool glcommon_debug_requested(void) {
	return env_get("TAISEI_GL_DEBUG", false);
}

void glcommon_set_debug_label_local(char *label_storage, const char *kind_name, GLuint gl_handle, const char *label) {
	if(label) {
		log_debug("%s \"%s\" renamed to \"%s\"", kind_name, label_storage, label);
		strlcpy(label_storage, label, R_DEBUG_LABEL_SIZE);
	} else {
		char tmp[R_DEBUG_LABEL_SIZE];
		snprintf(tmp, sizeof(tmp), "%s #%i", kind_name, gl_handle);
		log_debug("%s \"%s\" renamed to \"%s\"", kind_name, label_storage, tmp);
		strlcpy(label_storage, tmp, R_DEBUG_LABEL_SIZE);
	}
}

void glcommon_set_debug_label(char *label_storage, const char *kind_name, GLenum gl_enum, GLuint gl_handle, const char *label) {
	glcommon_set_debug_label_local(label_storage, kind_name, gl_handle, label);
	glcommon_set_debug_label_gl(gl_enum, gl_handle, label_storage);
}
