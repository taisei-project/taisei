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
#include "vfs/pathutil.h"
#include "rwops/rwops_autobuf.h"

/*
 * Resource API
 */

enum {
	SHOBJ_MAX_INCLUDE_DEPTH = 42,
};

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

static bool include_shader(const char *path, SDL_RWops *dest, int include_level, ShaderObjectType shader_type) {
	if(include_level > SHOBJ_MAX_INCLUDE_DEPTH) {
		log_warn("%s: max include depth reached. Maybe there is an include cycle?",path);	
		return false;
	}

	log_debug("[%i] %s", include_level, path);

	SDL_RWops *stream = vfs_open(path, VFS_MODE_READ);

	if(!stream) {
		log_warn("VFS error: %s", vfs_get_error());
		return false;
	}

	char linebuf[1024]; // TODO: remove this dumb limitation
	int lineno = 1;
	while(SDL_RWgets(stream, linebuf, sizeof(linebuf))) {
		const char include[] = "#include";

		char *p = linebuf;
		while(*p != 0 && isspace(*p)) {
			p++;
		}

		if(strstartswith(p,include)) {
			char *filename;
			p += sizeof(include)-1;
			while(*p != 0 && isspace(*p)) {
				p++;
			}
			if(*p != '"') {
				log_warn("%s:%d: malformed #include statement: \" expected.",path,lineno);
				SDL_RWclose(stream);
				return false;
			}
			if(*p != 0) {
				p++;
			}
			filename = p;
			while(*p != 0 && *p != '"') {
				p++;
			}
			if(*p != '"') {
				log_warn("%s:%d: malformed #include statement: second \" expected.",path,lineno);	
				SDL_RWclose(stream);
				return false;
			}
			*p=0;
			
			// I have no idea how all the undocumented helper functions scattered about the code work.
			char *q = strrchr(path, VFS_PATH_SEP)+1;
			char tmp = *q;
			if(q != 0) {
				*q = 0;
			}
			char *pathcopy = strdup(path);
			char *newpath = strjoin(pathcopy, filename, NULL);
			*p = '"';
			*q = tmp;
			
			free(pathcopy);
			SDL_RWprintf(dest, "#line 1 %i\n", include_level + 1);
			bool rc = include_shader(newpath, dest, include_level+1, shader_type);
			free(newpath);

			if(!rc) {
				SDL_RWclose(stream);
				return false;
			}

			SDL_RWprintf(dest, "#line %i %i\n", lineno + 1, include_level);
		} else {
			SDL_RWwrite(dest, linebuf, 1, strlen(linebuf));
		}

		if(!strendswith(p, "\n")) {
			SDL_WriteU8(dest, '\n');
		}

		lineno++;

		if(strstartswith(p, "#version")) {
			SDL_RWprintf(dest, "#define %s_STAGE\n#line %i %i\n",
				shader_type == SHOBJ_FRAG ? "FRAG" : "VERT",
				lineno,
				include_level
			);
		}
	}

	SDL_RWclose(stream);
	return true;
}

struct shobj_load_data {
	ShaderObject shobj;
	char src[];
};

static void* load_shader_object_begin(const char *path, uint flags) {
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
	SDL_RWops *writer = SDL_RWAutoBuffer((void**)&src, 256);

	if(!include_shader(path, writer, 0, type)) {
		SDL_RWclose(writer);
		return NULL;
	}

	SDL_WriteU8(writer, 0);

	struct shobj_load_data *ldata = calloc(1, sizeof(struct shobj_load_data) + strlen(src) + 1);
	strcpy(ldata->src, src);
	SDL_RWclose(writer);
	ldata->shobj.type = type;
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
		ldata->shobj.type == SHOBJ_VERT
			? GL_VERTEX_SHADER
			: GL_FRAGMENT_SHADER
	);
	GLint status;
	GLuint gl_handle = ldata->shobj.impl->gl_handle;

	char *basename = resource_util_basename(SHOBJ_PATH_PREFIX, path);
	glcommon_debug_object_label(GL_SHADER, gl_handle, basename);
	free(basename);

	// log_debug("Source code for %s:\n%s", path, ldata->src);

	glShaderSource(gl_handle, 1, (const GLchar*[]){ ldata->src }, (GLint[]){ -1 });
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
