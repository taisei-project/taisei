/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "debug.h"
#include "util.h"

static void APIENTRY gl33_debug(
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

	log_custom(lvl, "[%s, %s, %s, id:%i] %s", strsrc, strtype, strsev, id, message);
}

void gl33_debug_enable(void) {
	if(!glext.debug_output) {
		log_warn("OpenGL debugging is not supported on your system");
		return;
	}

	GLuint unused = 0;
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(gl33_debug, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, true);
	log_info("Enabled OpenGL debugging");
}

void gl33_debug_object_label(GLenum identifier, GLuint name, const char *label) {
	if(glext.KHR_debug) {
		glObjectLabel(identifier, name, strlen(label), label);
	}
}

bool gl33_debug_requested(void) {
	return getenvint("TAISEI_GL_DEBUG", DEBUG_GL_DEFAULT);
}
