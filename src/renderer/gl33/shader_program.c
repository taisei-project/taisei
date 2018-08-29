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
#include "../glcommon/debug.h"
#include "../api.h"

Uniform* gl33_shader_uniform(ShaderProgram *prog, const char *uniform_name) {
	return ht_get(&prog->uniforms, uniform_name, NULL);
}

UniformType gl33_uniform_type(Uniform *uniform) {
	return uniform->type;
}

static void uset_float(Uniform *uniform, uint count, const void *data) {
	glUniform1fv(uniform->location, count, (float*)data);
}

static void uget_float(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i);
	}
}


static void uset_vec2(Uniform *uniform, uint count, const void *data) {
	glUniform2fv(uniform->location, count, (float*)data);
}

static void uget_vec2(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 2);
	}
}


static void uset_vec3(Uniform *uniform, uint count, const void *data) {
	glUniform3fv(uniform->location, count, (float*)data);
}

static void uget_vec3(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 3);
	}
}


static void uset_vec4(Uniform *uniform, uint count, const void *data) {
	glUniform4fv(uniform->location, count, (float*)data);
}

static void uget_vec4(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 4);
	}
}


static void uset_int(Uniform *uniform, uint count, const void *data) {
	glUniform1iv(uniform->location, count, (int*)data);
}

static void uget_int(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformiv(uniform->prog->gl_handle, uniform->location + i, ((GLint*)data) + i);
	}
}


static void uset_ivec2(Uniform *uniform, uint count, const void *data) {
	glUniform2iv(uniform->location, count, (int*)data);
}

static void uget_ivec2(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformiv(uniform->prog->gl_handle, uniform->location + i, ((GLint*)data) + i * 2);
	}
}


static void uset_ivec3(Uniform *uniform, uint count, const void *data) {
	glUniform3iv(uniform->location, count, (int*)data);
}

static void uget_ivec3(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformiv(uniform->prog->gl_handle, uniform->location + i, ((GLint*)data) + i * 3);
	}
}


static void uset_ivec4(Uniform *uniform, uint count, const void *data) {
	glUniform4iv(uniform->location, count, (int*)data);
}

static void uget_ivec4(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformiv(uniform->prog->gl_handle, uniform->location + i, ((GLint*)data) + i * 4);
	}
}


static void uset_mat3(Uniform *uniform, uint count, const void *data) {
	glUniformMatrix3fv(uniform->location, count, false, (float*)data);
}

static void uget_mat3(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 9);
	}
}


static void uset_mat4(Uniform *uniform, uint count, const void *data) {
	glUniformMatrix4fv(uniform->location, count, false, (float*)data);
}

static void uget_mat4(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 16);
	}
}


static struct {
	void (*setter)(Uniform *uniform, uint count, const void *data);
	void (*getter)(Uniform *uniform, uint count, void *data);
} type_to_accessors[] = {
	[UNIFORM_FLOAT]   = { uset_float, uget_float },
	[UNIFORM_VEC2]    = { uset_vec2,  uget_vec2 },
	[UNIFORM_VEC3]    = { uset_vec3,  uget_vec3 },
	[UNIFORM_VEC4]    = { uset_vec4,  uget_vec4 },
	[UNIFORM_INT]     = { uset_int,   uget_int },
	[UNIFORM_IVEC2]   = { uset_ivec2, uget_ivec2 },
	[UNIFORM_IVEC3]   = { uset_ivec3, uget_ivec3 },
	[UNIFORM_IVEC4]   = { uset_ivec4, uget_ivec4 },
	[UNIFORM_SAMPLER] = { uset_int,   uget_int },
	[UNIFORM_MAT3]    = { uset_mat3,  uget_mat3 },
	[UNIFORM_MAT4]    = { uset_mat4,  uget_mat4 },
};

typedef struct MagicalUniform {
	const char *name;
	const char *typename;
	UniformType type;
} MagicalUniform;

static MagicalUniform magical_unfiroms[] = {
	{ "r_modelViewMatrix",  "mat4", UNIFORM_MAT4 },
	{ "r_projectionMatrix", "mat4", UNIFORM_MAT4 },
	{ "r_textureMatrix",    "mat4", UNIFORM_MAT4 },
	{ "r_color",            "vec4", UNIFORM_VEC4 },
};
static void gl33_commit_uniform(Uniform *uniform);
static void gl33_update_uniform(Uniform *uniform, uint count, const void *data, size_t datasize) {
	if(datasize > uniform->buffer_size) {
		// might happen when unused array elements get optimized out
		datasize = uniform->buffer_size;
	}

	memcpy(uniform->cache.pending, data, datasize);

	if(datasize > uniform->cache.update_size) {
		uniform->cache.update_size = datasize;
	}
}

static void gl33_commit_uniform(Uniform *uniform) {
	memcpy(uniform->cache.commited, uniform->cache.pending, uniform->buffer_size);
	type_to_accessors[uniform->type].setter(uniform, uniform->array_size, uniform->cache.commited);

	uniform->cache.update_size = 0;
}

static void* gl33_sync_uniform(const char *key, void *value, void *arg) {
	attr_unused const char *name = key;
	Uniform *uniform = value;

	if(uniform->cache.update_size == 0) {
		return NULL;
	}

	if(memcmp(uniform->cache.commited, uniform->cache.pending, uniform->cache.update_size)) {
		gl33_commit_uniform(uniform);
	} else {
		// log_debug("uniform %u:%s (shader %u) update of size %zu ignored", uniform->location, name, uniform->prog->gl_handle, uniform->cache.update_size);
	}

	return NULL;
}

void gl33_sync_uniforms(ShaderProgram *prog) {
	ht_foreach(&prog->uniforms, gl33_sync_uniform, NULL);
}

void gl33_uniform(Uniform *uniform, uint count, const void *data) {
	assert(count > 0);

	if(uniform == NULL) {
		return;
	}

	assert(uniform->prog != NULL);
	assert(uniform->type >= 0 && uniform->type < sizeof(type_to_accessors)/sizeof(*type_to_accessors));
	const UniformTypeInfo *typeinfo = r_uniform_type_info(uniform->type);
	size_t datasize = typeinfo->elements * typeinfo->element_size * count;
	gl33_update_uniform(uniform, count, data, datasize);
}

static bool cache_uniforms(ShaderProgram *prog) {
	int maxlen = 0;
	GLint unicount;

	ht_create(&prog->uniforms);

	glGetProgramiv(prog->gl_handle, GL_ACTIVE_UNIFORMS, &unicount);
	glGetProgramiv(prog->gl_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxlen);

	if(maxlen < 1 || unicount < 1) {
		return true;
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

		for(int i = 0; i < sizeof(magical_unfiroms)/sizeof(MagicalUniform); ++i) {
			MagicalUniform *m = magical_unfiroms + i;

			if(!strcmp(name, m->name) && uni.type != m->type) {
				log_warn("Magical uniform '%s' must be of type '%s'", name, m->typename);
				return false;
			}
		}

		const UniformTypeInfo *typeinfo = r_uniform_type_info(uni.type);

		uni.location = loc;
		uni.array_size = size;
		uni.buffer_size = size * typeinfo->element_size * typeinfo->elements;
		uni.cache.commited = malloc(uni.buffer_size);
		uni.cache.pending = malloc(uni.buffer_size);

		type_to_accessors[uni.type].getter(&uni, size, uni.cache.commited);

		ht_set(&prog->uniforms, name, memdup(&uni, sizeof(uni)));
		log_debug("%s = %i [array elements: %i; size: %zi bytes]", name, loc, size, uni.buffer_size);
	}

	return true;
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
	memset(&ldata, 0, sizeof(ldata));
	ldata.load_flags = flags;

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

static void* free_uniform(const char *key, void *data, void *arg) {
	Uniform *uniform = data;
	free(uniform->cache.commited);
	free(uniform->cache.pending);
	free(uniform);
	return NULL;
}

static void unload_shader_program(void *vprog) {
	ShaderProgram *prog = vprog;
	gl33_shader_deleted(prog);
	glDeleteProgram(prog->gl_handle);
	ht_foreach(&prog->uniforms, free_uniform, NULL);
	ht_destroy(&prog->uniforms);
	free(prog);
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
	glcommon_debug_object_label(GL_PROGRAM, ldata.shprog.gl_handle, basename);
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

	ShaderProgram *prog = memdup(&ldata.shprog, sizeof(ldata.shprog));

	if(!cache_uniforms(prog)) {
		unload_shader_program(prog);
		prog = NULL;
	}

	return prog;
}

ResourceHandler gl33_shader_program_res_handler = {
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
