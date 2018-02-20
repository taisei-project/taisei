/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "shader_program.h"
#include "shader_object.h"
#include "renderer.h"

ResourceHandler shader_program_res_handler = {
	.type = RES_SHADER_PROGRAM,
	.typename = "shader program",
	.subdir = SHPROG_PATH_PREFIX,

	.procs = {
		.find = shader_program_path,
		.check = check_shader_program_path,
		.begin_load = load_shader_program_begin,
		.end_load = load_shader_program_end,
		.unload = unload_shader_program,
	},
};

char* shader_program_path(const char *name) {
	return strjoin(SHPROG_PATH_PREFIX, name, SHPROG_EXT, NULL);
}

bool check_shader_program_path(const char *path) {
	return strendswith(path, SHPROG_EXT) && strstartswith(path, SHPROG_PATH_PREFIX);
}

struct shprog_load_data {
	ShaderProgram shprog;
	int num_objects;
	unsigned int load_flags;
	char *objlist;
};

static void for_each_shobject(struct shprog_load_data *ldata, void (*func)(const char *obj, struct shprog_load_data *ldata)) {
	const char *rawobjects = ldata->objlist;
	char buf[strlen(rawobjects) + 1];
	strcpy(buf, rawobjects);
	char *objname, *bufptr = buf;
	strcpy(buf, rawobjects);

	while((objname = strtok_r(NULL, " \t", &bufptr))) {
		if(*objname) {
			func(objname, ldata);
		}
	}
}

static void preload_shobject(const char *name, struct shprog_load_data *ldata) {
	preload_resource(RES_SHADER_OBJECT, name, ldata->load_flags);
	++ldata->num_objects;
}

void* load_shader_program_begin(const char *path, unsigned int flags) {
	struct shprog_load_data ldata;
	ldata.load_flags = flags;
	memset(&ldata, 0, sizeof(ldata));

	if(!parse_keyvalue_file_with_spec(path, (KVSpec[]){
		{ "glsl_objects", .out_str = &ldata.objlist },
		{ NULL }
	})) {
		free(ldata.objlist);
		return NULL;
	}

	if(ldata.objlist) {
		for_each_shobject(&ldata, preload_shobject);
	}

	if(!ldata.num_objects) {
		log_warn("%s: no shader objects to link", path);
		free(ldata.objlist);
		return NULL;
	}

	return memdup(&ldata, sizeof(ldata));
}

static void attach_shobject(const char *name, struct shprog_load_data *ldata) {
	ShaderObject *shobj = get_resource_data(RES_SHADER_OBJECT, name, ldata->load_flags);

	if(!shobj) {
		return;
	}

	glAttachShader(ldata->shprog.gl_handle, shobj->gl_handle);
	--ldata->num_objects;
}

static void print_info_log(GLuint prog) {
	GLint len = 0, alen = 0;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);

	if(len > 1) {
		char log[len];
		memset(log, 0, len);
		glGetProgramInfoLog(prog, len, &alen, log);

		log_warn(
			"\n== Shader program linkage log (%u) ==\n%s\n== End of shader program linkage log (%u) ==",
			prog, log, prog
		);
	}
}

static void cache_uniforms(ShaderProgram *prog) {
	int maxlen = 0;
	GLint tmpi;
	GLenum tmpt;
	GLint unicount;

	prog->uniforms = hashtable_new_stringkeys(13);

	glGetProgramiv(prog->gl_handle, GL_ACTIVE_UNIFORMS, &unicount);
	glGetProgramiv(prog->gl_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxlen);

	if(maxlen < 1) {
		return;
	}

	char name[maxlen];

	for(int i = 0; i < unicount; ++i) {
		glGetActiveUniform(prog->gl_handle, i, maxlen, NULL, &tmpi, &tmpt, name);
		// This +1 increment kills two youkai with one spellcard.
		// 1. We can't store 0 in the hashtable, because that's the NULL/nonexistent value.
		//    But 0 is a valid uniform location, so we need to store that in some way.
		// 2. glGetUniformLocation returns -1 for builtin uniforms, which we don't want to cache anyway.
		hashtable_set_string(prog->uniforms, name, (void*)(intptr_t)(glGetUniformLocation(prog->gl_handle, name) + 1));
	}

#ifdef DEBUG_GL
	hashtable_print_stringkeys(prog->uniforms);
#endif
}

void* load_shader_program_end(void *opaque, const char *path, unsigned int flags) {
	if(!opaque) {
		return NULL;
	}

	struct shprog_load_data ldata;
	memcpy(&ldata, opaque, sizeof(ldata));
	free(opaque);

	ldata.shprog.gl_handle = glCreateProgram();
	for_each_shobject(&ldata, attach_shobject);
	free(ldata.objlist);

	if(ldata.num_objects) {
		log_warn("Failed to attach some shaders");
		glDeleteProgram(ldata.shprog.gl_handle);
		return NULL;
	}

	GLint status;

	glLinkProgram(ldata.shprog.gl_handle);
	print_info_log(ldata.shprog.gl_handle);
	glGetProgramiv(ldata.shprog.gl_handle, GL_LINK_STATUS, &status);

	if(!status) {
		log_warn("Failed to link the shader program");
		glDeleteProgram(ldata.shprog.gl_handle);
		return NULL;
	}

	ldata.shprog.renderctx_block_idx = glGetUniformBlockIndex(ldata.shprog.gl_handle, "RenderContext");

	if(ldata.shprog.renderctx_block_idx >= 0) {
		glUniformBlockBinding(ldata.shprog.gl_handle, ldata.shprog.renderctx_block_idx, 1);
	}

	cache_uniforms(&ldata.shprog);

	return memdup(&ldata.shprog, sizeof(ldata.shprog));
}

void unload_shader_program(void *vprog) {
	ShaderProgram *prog = vprog;
	glDeleteProgram(prog->gl_handle);
	hashtable_free(prog->uniforms);
	free(prog);
}

int uniloc(ShaderProgram *prog, const char *name) {
	return (intptr_t)hashtable_get_string(prog->uniforms, name) - 1;
}

ShaderProgram* get_shader_program(const char *name) {
	return get_resource(RES_SHADER_PROGRAM, name, RESF_DEFAULT | RESF_UNSAFE)->data;
}

ShaderProgram* get_shader_program_optional(const char *name) {
	Resource *r = get_resource(RES_SHADER_PROGRAM, name, RESF_OPTIONAL | RESF_UNSAFE);

	if(!r) {
		log_warn("ShaderProgram program %s could not be loaded", name);
		return NULL;
	}

	return r->data;
}
