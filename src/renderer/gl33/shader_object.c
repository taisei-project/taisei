/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "util.h"
#include "core.h"
#include "shader_object.h"
#include "../glcommon/debug.h"
#include "../common/shader_glsl.h"
#include "vfs/pathutil.h"
#include "rwops/rwops_autobuf.h"

/*
 * Resource API
 */

static char* shader_object_path(const char *name) {
	if(strendswith_any(name, (const char*[]){".vert", ".frag", NULL})) {
		return strjoin(SHOBJ_PATH_PREFIX, name, ".glsl", NULL);
	}

	return NULL;
}

static bool check_shader_object_path(const char *path) {
	if(!strstartswith(path, SHOBJ_PATH_PREFIX)) {
		return false;
	}

	return strendswith_any(path, (const char*[]){".vert.glsl", ".frag.glsl", NULL});
}

struct shobj_load_data {
	ShaderObject shobj;
	GLSLSource source;
};

static void* load_shader_object_begin(const char *path, uint flags) {
	ShaderStage stage;

	if(strendswith(path, ".frag.glsl")) {
		stage = SHADER_STAGE_FRAGMENT;
	} else if(strendswith(path, ".vert.glsl")) {
		stage = SHADER_STAGE_VERTEX;
	} else {
		log_warn("%s: can't determine type of shader object", path);
		return NULL;
	}

	struct shobj_load_data *ldata = calloc(1, sizeof(struct shobj_load_data));
	GLSLSourceOptions opts = {
		.version = { 330, GLSL_PROFILE_CORE },
		.stage = stage,
	};

	if(!glsl_load_source(path, &ldata->source, &opts)) {
		free(ldata);
		return NULL;
	}

	ldata->shobj.stage = ldata->source.stage;
	ldata->shobj.impl = calloc(1, sizeof(ShaderObjectImpl));

	return ldata;
}

static void print_info_log(GLuint shader) {
	GLint len = 0, alen = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

	if(len > 1) {
		char log[len];
		memset(log, 0, len);
		glGetShaderInfoLog(shader, len, &alen, log);

		log_warn(
			"\n== Shader object compilation log (%u) ==\n%s\n== End of shader object compilation log (%u) ==",
				 shader, log, shader
		);
	}
}

static void* load_shader_object_end(void *opaque, const char *path, uint flags) {
	struct shobj_load_data *ldata = opaque;

	if(!ldata) {
		return NULL;
	}

	ShaderObject *shobj = NULL;
	ldata->shobj.impl->gl_handle = glCreateShader(
		ldata->shobj.stage == SHADER_STAGE_VERTEX
			? GL_VERTEX_SHADER
			: GL_FRAGMENT_SHADER
	);
	GLint status;
	GLuint gl_handle = ldata->shobj.impl->gl_handle;

	char *basename = resource_util_basename(SHOBJ_PATH_PREFIX, path);
	glcommon_debug_object_label(GL_SHADER, gl_handle, basename);
	free(basename);

	// log_debug("Source code for %s:\n%s", path, ldata->source.content);

	glShaderSource(
		gl_handle, 1,
		(const GLchar*[]) { ldata->source.content },
		(GLint[])         { ldata->source.content_size - 1 }
	);
	glCompileShader(gl_handle);
	glGetShaderiv(gl_handle, GL_COMPILE_STATUS, &status);
	print_info_log(gl_handle);

	if(!status) {
		log_warn("%s: failed to compile shader object", path);
		glDeleteShader(gl_handle);
		free(ldata->shobj.impl);
	} else {
		shobj = memdup(&ldata->shobj, sizeof(ldata->shobj));
	}

	free(ldata->source.content);
	free(ldata);
	return shobj;
}

static void unload_shader_object(void *vsha) {
	ShaderObject *shobj = vsha;
	glDeleteShader(shobj->impl->gl_handle);
	free(shobj->impl);
	free(shobj);
}

ResourceHandler gl33_shader_object_res_handler = {
	.type = RES_SHADER_OBJECT,
	.typename = "shader object",
	.subdir = SHOBJ_PATH_PREFIX,

	.procs = {
		.find = shader_object_path,
		.check = check_shader_object_path,
		.begin_load = load_shader_object_begin,
		.end_load = load_shader_object_end,
		.unload = unload_shader_object,
	},
};
