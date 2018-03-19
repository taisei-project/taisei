/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "core.h"
#include "shader_program.h"
#include "shader_object.h"
#include "../api.h"

Uniform* r_shader_uniform(ShaderProgram *prog, const char *uniform_name) {
	return hashtable_get_string(prog->uniforms, uniform_name);
}

static void uset_float(Uniform *uniform, uint count, const void *data) {
	glUniform1fv(uniform->location, count, (float*)data);
}

static void uset_vec2(Uniform *uniform, uint count, const void *data) {
	glUniform2fv(uniform->location, count, (float*)data);
}

static void uset_vec3(Uniform *uniform, uint count, const void *data) {
	glUniform3fv(uniform->location, count, (float*)data);
}

static void uset_vec4(Uniform *uniform, uint count, const void *data) {
	glUniform4fv(uniform->location, count, (float*)data);
}

static void uset_int(Uniform *uniform, uint count, const void *data) {
	glUniform1iv(uniform->location, count, (int*)data);
}

static void uset_ivec2(Uniform *uniform, uint count, const void *data) {
	glUniform2iv(uniform->location, count, (int*)data);
}

static void uset_ivec3(Uniform *uniform, uint count, const void *data) {
	glUniform3iv(uniform->location, count, (int*)data);
}

static void uset_ivec4(Uniform *uniform, uint count, const void *data) {
	glUniform4iv(uniform->location, count, (int*)data);
}

static void uset_mat3(Uniform *uniform, uint count, const void *data) {
	glUniformMatrix3fv(uniform->location, count, false, (float*)data);
}

static void uset_mat4(Uniform *uniform, uint count, const void *data) {
	glUniformMatrix4fv(uniform->location, count, false, (float*)data);
}

void r_uniform_ptr(Uniform *uniform, uint count, const void *data) {
	assert(count > 0);
	assert(uniform != NULL);
	assert(uniform->prog != NULL);

	if(uniform == NULL) {
		return;
	}

	static UniformSetter type_to_setter[] = {
		[UNIFORM_FLOAT]   = uset_float,
		[UNIFORM_VEC2]    = uset_vec2,
		[UNIFORM_VEC3]    = uset_vec3,
		[UNIFORM_VEC4]    = uset_vec4,
		[UNIFORM_INT]     = uset_int,
		[UNIFORM_IVEC2]   = uset_ivec2,
		[UNIFORM_IVEC3]   = uset_ivec3,
		[UNIFORM_IVEC4]   = uset_ivec4,
		[UNIFORM_SAMPLER] = uset_int,
		[UNIFORM_MAT3]    = uset_mat3,
		[UNIFORM_MAT4]    = uset_mat4,
	};

	assert(uniform->type >= 0 && uniform->type < sizeof(type_to_setter)/sizeof(UniformSetter));

	ShaderProgram *prev_prog = r_shader_current();
	r_shader_ptr(uniform->prog);
	r_flush();
	type_to_setter[uniform->type](uniform, count, data);
	r_shader_ptr(prev_prog);
}

static void cache_uniforms(ShaderProgram *prog) {
	int maxlen = 0;
	GLint unicount;

	prog->uniforms = hashtable_new_stringkeys(13);

	glGetProgramiv(prog->gl_handle, GL_ACTIVE_UNIFORMS, &unicount);
	glGetProgramiv(prog->gl_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxlen);

	if(maxlen < 1) {
		return;
	}

	char name[maxlen];

	for(int i = 0; i < unicount; ++i) {
		GLenum type;
		GLint size, loc;
		Uniform uni = { .prog = prog };

		glGetActiveUniform(prog->gl_handle, i, maxlen, NULL, &size, &type, name);
		loc = glGetUniformLocation(prog->gl_handle, name);

		if(loc < 0) {
			// builtin uniform
			continue;
		}

		switch(type) {
			case GL_FLOAT:      uni.type = UNIFORM_FLOAT;   break;
			case GL_FLOAT_VEC2: uni.type = UNIFORM_VEC2;    break;
			case GL_FLOAT_VEC3: uni.type = UNIFORM_VEC3;    break;
			case GL_FLOAT_VEC4: uni.type = UNIFORM_VEC4;    break;
			case GL_INT:        uni.type = UNIFORM_INT;     break;
			case GL_INT_VEC2:   uni.type = UNIFORM_IVEC2;   break;
			case GL_INT_VEC3:   uni.type = UNIFORM_IVEC3;   break;
			case GL_INT_VEC4:   uni.type = UNIFORM_IVEC4;   break;
			case GL_SAMPLER_2D: uni.type = UNIFORM_SAMPLER; break;
			case GL_FLOAT_MAT3: uni.type = UNIFORM_MAT3;    break;
			case GL_FLOAT_MAT4: uni.type = UNIFORM_MAT4;    break;

			default:
				log_warn("Uniform '%s' is of an unsupported type 0x%04x and will be ignored.", name, type);
				continue;
		}

		uni.location = loc;
		hashtable_set_string(prog->uniforms, name, memdup(&uni, sizeof(uni)));
	}

	#ifdef DEBUG_GL
	hashtable_print_stringkeys(prog->uniforms);
	#endif
}

/*
 * Resource API
 */

static char* shader_program_path(const char *name) {
	return strjoin(SHPROG_PATH_PREFIX, name, SHPROG_EXT, NULL);
}

static bool check_shader_program_path(const char *path) {
	return strendswith(path, SHPROG_EXT) && strstartswith(path, SHPROG_PATH_PREFIX);
}

struct shprog_load_data {
	ShaderProgram shprog;
	int num_objects;
	uint load_flags;
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

static void* load_shader_program_begin(const char *path, uint flags) {
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

	glAttachShader(ldata->shprog.gl_handle, shobj->impl->gl_handle);
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

static void* load_shader_program_end(void *opaque, const char *path, uint flags) {
	if(!opaque) {
		return NULL;
	}

	struct shprog_load_data ldata;
	memcpy(&ldata, opaque, sizeof(ldata));
	free(opaque);

	ldata.shprog.gl_handle = glCreateProgram();

	char *basename = resource_util_basename(SHPROG_PATH_PREFIX, path);
	gl33_debug_object_label(GL_PROGRAM, ldata.shprog.gl_handle, basename);
	free(basename);

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

	ShaderProgram *prog = memdup(&ldata.shprog, sizeof(ldata.shprog));
	cache_uniforms(prog);
	return prog;
}

void unload_shader_program(void *vprog) {
	ShaderProgram *prog = vprog;
	glDeleteProgram(prog->gl_handle);
	hashtable_foreach(prog->uniforms, hashtable_iter_free_data, NULL);
	hashtable_free(prog->uniforms);
	free(prog);
}

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
