/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "util.h"
#include "shader_object.h"
#include "rwops/rwops_autobuf.h"

ResourceHandler shader_object_res_handler = {
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

char* shader_object_path(const char *name) {
	if(strendswith_any(name, (const char*[]){".vert", ".frag", NULL})) {
		return strjoin(SHOBJ_PATH_PREFIX, name, ".glsl", NULL);
	}

	return NULL;
}

bool check_shader_object_path(const char *path) {
	if(!strstartswith(path, SHOBJ_PATH_PREFIX)) {
		return false;
	}

	return strendswith_any(path, (const char*[]){".vert.glsl", ".frag.glsl", NULL});
}

static bool include_shader(const char *path, SDL_RWops *dest, int *include_level) {
	SDL_RWops *stream = vfs_open(path, VFS_MODE_READ);

	if(!stream) {
		log_warn("VFS error: %s", vfs_get_error());
		return false;
	}

	char linebuf[256]; // TODO: remove this dumb limitation

	while(SDL_RWgets(stream, linebuf, sizeof(linebuf))) {
		SDL_RWwrite(dest, linebuf, 1, strlen(linebuf));
	}

	return true;
}

struct shobj_load_data {
	ShaderObject shobj;
	char src[];
};

void* load_shader_object_begin(const char *path, unsigned int flags) {
	ShaderObjectType type;

	if(strendswith(path, ".frag.glsl")) {
		type = SHOBJ_FRAG;
	} else if(strendswith(path, ".vert.glsl")) {
		type = SHOBJ_VERT;
	} else {
		log_warn("%s: can't determine type of shader object", path);
		return NULL;
	}

	char *src = NULL;
	int include_level = 0;
	SDL_RWops *writer = SDL_RWAutoBuffer((void**)&src, 256);

	if(!include_shader(path, writer, &include_level)) {
		SDL_RWclose(writer);
		return NULL;
	}

	struct shobj_load_data *ldata = calloc(1, sizeof(struct shobj_load_data) + strlen(src) + 1);
	strcpy(ldata->src, src);
	SDL_RWclose(writer);
	ldata->shobj.type = type;

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

void* load_shader_object_end(void *opaque, const char *path, unsigned int flags) {
	struct shobj_load_data *ldata = opaque;

	if(!ldata) {
		return NULL;
	}

	ShaderObject *shobj = NULL;
	ldata->shobj.gl_handle = glCreateShader(ldata->shobj.type == SHOBJ_VERT ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
	GLint status;

	glShaderSource(ldata->shobj.gl_handle, 1, (const GLchar*[]){ ldata->src }, (GLint[]){ -1 });
	glCompileShader(ldata->shobj.gl_handle);
	glGetShaderiv(ldata->shobj.gl_handle, GL_COMPILE_STATUS, &status);
	print_info_log(ldata->shobj.gl_handle);

	if(!status) {
		log_warn("%s: failed to compile shader object", path);
		glDeleteShader(ldata->shobj.gl_handle);
	} else {
		shobj = memdup(&ldata->shobj, sizeof(ldata->shobj));
	}

	free(ldata);
	return shobj;
}

void unload_shader_object(void *vsha) {
	ShaderObject *shobj = vsha;
	glDeleteShader(shobj->gl_handle);
	free(shobj);
}
