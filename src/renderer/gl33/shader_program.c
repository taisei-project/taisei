/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "gl33.h"
#include "shader_program.h"
#include "shader_object.h"
#include "../glcommon/debug.h"
#include "../api.h"

static Uniform *sampler_uniforms;

Uniform *gl33_shader_uniform(ShaderProgram *prog, const char *uniform_name, hash_t uniform_name_hash) {
	return ht_get_prehashed(&prog->uniforms, uniform_name, uniform_name_hash, NULL);
}

UniformType gl33_uniform_type(Uniform *uniform) {
	return uniform->type;
}

static void uset_float(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniform1fv(uniform->location + offset, count, (float*)data);
}

static void uget_float(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i);
	}
}


static void uset_vec2(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniform2fv(uniform->location + offset, count, (float*)data);
}

static void uget_vec2(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 2);
	}
}


static void uset_vec3(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniform3fv(uniform->location + offset, count, (float*)data);
}

static void uget_vec3(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 3);
	}
}


static void uset_vec4(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniform4fv(uniform->location + offset, count, (float*)data);
}

static void uget_vec4(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 4);
	}
}


static void uset_int(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniform1iv(uniform->location + offset, count, (int*)data);
}

static void uget_int(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformiv(uniform->prog->gl_handle, uniform->location + i, ((GLint*)data) + i);
	}
}


static void uset_ivec2(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniform2iv(uniform->location + offset, count, (int*)data);
}

static void uget_ivec2(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformiv(uniform->prog->gl_handle, uniform->location + i, ((GLint*)data) + i * 2);
	}
}


static void uset_ivec3(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniform3iv(uniform->location + offset, count, (int*)data);
}

static void uget_ivec3(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformiv(uniform->prog->gl_handle, uniform->location + i, ((GLint*)data) + i * 3);
	}
}


static void uset_ivec4(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniform4iv(uniform->location + offset, count, (int*)data);
}

static void uget_ivec4(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformiv(uniform->prog->gl_handle, uniform->location + i, ((GLint*)data) + i * 4);
	}
}


static void uset_mat3(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniformMatrix3fv(uniform->location + offset, count, false, (float*)data);
}

static void uget_mat3(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 9);
	}
}


static void uset_mat4(Uniform *uniform, uint offset, uint count, const void *data) {
	glUniformMatrix4fv(uniform->location + offset, count, false, (float*)data);
}

static void uget_mat4(Uniform *uniform, uint count, void *data) {
	for(uint i = 0; i < count; ++i) {
		glGetUniformfv(uniform->prog->gl_handle, uniform->location + i, ((GLfloat*)data) + i * 16);
	}
}


static struct {
	void (*setter)(Uniform *uniform, uint offset, uint count, const void *data);
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

static void gl33_update_uniform(Uniform *uniform, uint offset, uint count, const void *data) {
	// these are validated properly in gl33_uniform
	assert(offset < uniform->array_size);
	assert(offset + count <= uniform->array_size);

	uint idx_first = offset;
	uint idx_last = offset + count - 1;

	assert(idx_last < uniform->array_size);
	assert(idx_first <= idx_last);

	memcpy(uniform->cache.pending + offset, data, count * uniform->elem_size);

	if(idx_first < uniform->cache.update_first_idx) {
		uniform->cache.update_first_idx = idx_first;
	}

	if(idx_last > uniform->cache.update_last_idx) {
		uniform->cache.update_last_idx = idx_last;
	}
}

static void gl33_commit_uniform(Uniform *uniform) {
	if(uniform->cache.update_first_idx > uniform->cache.update_last_idx) {
		return;
	}

	uint update_count = uniform->cache.update_last_idx - uniform->cache.update_first_idx + 1;
	size_t update_ofs = uniform->cache.update_first_idx * uniform->elem_size;
	size_t update_sz  = update_count * uniform->elem_size;

	assert(update_sz + update_ofs <= uniform->elem_size * uniform->array_size);

	if(memcmp(uniform->cache.commited + update_ofs, uniform->cache.pending + update_ofs, update_sz)) {
		memcpy(uniform->cache.commited + update_ofs, uniform->cache.pending + update_ofs, update_sz);

		type_to_accessors[uniform->type].setter(
			uniform,
			uniform->cache.update_first_idx,
			update_count,
			uniform->cache.commited + update_ofs
		);
	}

	uniform->cache.update_first_idx = uniform->array_size;
	uniform->cache.update_last_idx = 0;
}

static void *gl33_sync_uniform(const char *key, void *value, void *arg) {
	Uniform *uniform = value;

	// special case: for sampler uniforms, we have to construct the actual data from the texture pointers array.
	if(uniform->type == UNIFORM_SAMPLER) {
		Uniform *size_uniform = uniform->size_uniform;

		for(uint i = 0; i < uniform->array_size; ++i) {
			Texture *tex = uniform->textures[i];

			if(tex == NULL) {
				continue;
			}

			if(glext.issues.avoid_sampler_uniform_updates) {
				int preferred_unit = CASTPTR_ASSUME_ALIGNED(uniform->cache.pending, int)[i];
				attr_unused GLuint unit = gl33_bind_texture(tex, true, preferred_unit);
				assert(unit == preferred_unit);
			} else {
				GLuint unit = gl33_bind_texture(tex, true, -1);
				gl33_update_uniform(uniform, i, 1, &unit);
			}

			if(size_uniform) {
				uint w, h;
				r_texture_get_size(tex, 0, &w, &h);
				vec2_noalign size = { w, h };
				gl33_update_uniform(size_uniform, i, 1, &size);
				gl33_commit_uniform(size_uniform);
			}
		}
	}

	gl33_commit_uniform(uniform);
	return NULL;
}

void gl33_sync_uniforms(ShaderProgram *prog) {
	ht_foreach(&prog->uniforms, gl33_sync_uniform, NULL);
}

void gl33_uniform(Uniform *uniform, uint offset, uint count, const void *data) {
	assert(count > 0);
	assert(uniform != NULL);
	assert(uniform->prog != NULL);
	assert(uniform->type >= 0 && uniform->type < sizeof(type_to_accessors)/sizeof(*type_to_accessors));

	if(offset >= uniform->array_size) {
		// completely out of range
		return;
	}

	if(offset + count > uniform->array_size) {
		// partially out of range
		count = uniform->array_size - offset;
	}

	// special case: for sampler uniforms, data is an array of Texture pointers that we'll have to bind later.
	if(uniform->type == UNIFORM_SAMPLER) {
		Texture **textures = (Texture**)data;
		memcpy(uniform->textures + offset, textures, sizeof(Texture*) * count);
	} else {
		gl33_update_uniform(uniform, offset, count, data);
	}
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
	int sampler_binding = 0;

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

		for(int j = 0; j < sizeof(magical_unfiroms)/sizeof(MagicalUniform); ++j) {
			MagicalUniform *m = magical_unfiroms + j;

			if(!strcmp(name, m->name) && uni.type != m->type) {
				log_error("Magical uniform '%s' must be of type '%s'", name, m->typename);
				return false;
			}
		}

		const UniformTypeInfo *typeinfo = r_uniform_type_info(uni.type);

		uni.location = loc;
		uni.array_size = size;

		if(uni.type == UNIFORM_SAMPLER) {
			uni.elem_size = sizeof(GLint);
			uni.textures = calloc(uni.array_size, sizeof(Texture*));
		} else {
			uni.elem_size = typeinfo->element_size * typeinfo->elements;
		}

		uni.cache.commited = calloc(uni.array_size, uni.elem_size);
		uni.cache.pending = calloc(uni.array_size, uni.elem_size);
		uni.cache.update_first_idx = uni.array_size;

		if(glext.version.is_webgl) {
			// Some browsers are pedantic about getting a null in GLctx.getUniform(),
			// so we'd have to be very careful and query each array index with
			// glGetUniformLocation in order to avoid an exception. Which is too much
			// hassle, so instead here's a hack that fills initial cache state with
			// some garbage that we'll not likely want to actually set.
			//
			// TODO: Might want to fix this properly if this issue ever actually
			// affects cases where we write to an array with an offset. But that's
			// probably not going to happen.
			memset(uni.cache.commited, 0xf0, uni.array_size * uni.elem_size);
		} else {
			type_to_accessors[uni.type].getter(&uni, size, uni.cache.commited);
		}

		Uniform *new_uni = memdup(&uni, sizeof(uni));

		if(uni.type == UNIFORM_SAMPLER) {
			list_push(&sampler_uniforms, new_uni);

			if(glext.issues.avoid_sampler_uniform_updates) {
				// Bind each sampler to a different texturing unit.
				// This way we can change textures by binding them to the right texturing units, and never update the shader's samplers again.

				int payload[new_uni->array_size];
				assert(sizeof(payload) == new_uni->array_size * new_uni->elem_size);

				for(int j = 0; j < ARRAY_SIZE(payload); ++j) {
					payload[j] = sampler_binding++;
				}

				gl33_update_uniform(new_uni, 0, new_uni->array_size, payload);
			}
		}

		ht_set(&prog->uniforms, name, new_uni);
		log_debug("%s = %i [array elements: %i; size: %zi bytes]", name, loc, uni.array_size, uni.array_size * uni.elem_size);
	}

	ht_str2ptr_iter_t iter;
	ht_iter_begin(&prog->uniforms, &iter);
	while(iter.has_data) {
		Uniform *u = iter.value;

		if(u->type == UNIFORM_SAMPLER) {
			const char *sampler_name = iter.key;
			const char size_suffix[] = "_SIZE";
			char size_uniform_name[strlen(sampler_name) + sizeof(size_suffix)];
			snprintf(size_uniform_name, sizeof(size_uniform_name), "%s%s", sampler_name, size_suffix);
			u->size_uniform = ht_get(&prog->uniforms, size_uniform_name, NULL);

			if(u->size_uniform) {
				Uniform *size_uniform = u->size_uniform;

				if(size_uniform->type != UNIFORM_VEC2) {
					log_warn("Size uniform %s has invalid type (should be vec2), ignoring", size_uniform_name);
					u->size_uniform = NULL;
				} else if(size_uniform->array_size != u->array_size) {
					log_warn("Size uniform %s has invalid array size (should be %i), ignoring", size_uniform_name, u->array_size);
					u->size_uniform = NULL;
				} else {
					log_debug("Bound size uniform: %s --> %s", sampler_name, size_uniform_name);
				}

				assert(u->size_uniform != NULL);  // fix your shader!
			}
		}

		ht_iter_next(&iter);
	}
	ht_iter_end(&iter);

	return true;
}

void gl33_unref_texture_from_samplers(Texture *tex) {
	for(Uniform *u = sampler_uniforms; u; u = u->next) {
		assert(u->type == UNIFORM_SAMPLER);
		assert(u->textures != NULL);

		for(Texture **slot = u->textures; slot < u->textures + u->array_size; ++slot) {
			if(*slot == tex) {
				*slot = NULL;
			}
		}
	}
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

static void *free_uniform(const char *key, void *data, void *arg) {
	Uniform *uniform = data;

	if(uniform->type == UNIFORM_SAMPLER) {
		list_unlink(&sampler_uniforms, uniform);
	}

	free(uniform->textures);
	free(uniform->cache.commited);
	free(uniform->cache.pending);
	free(uniform);
	return NULL;
}

void gl33_shader_program_destroy(ShaderProgram *prog) {
	gl33_shader_deleted(prog);
	glDeleteProgram(prog->gl_handle);
	ht_foreach(&prog->uniforms, free_uniform, NULL);
	ht_destroy(&prog->uniforms);
	free(prog);
}

ShaderProgram *gl33_shader_program_link(uint num_objects, ShaderObject *shobjs[num_objects]) {
	ShaderProgram *prog = calloc(1, sizeof(*prog));

	prog->gl_handle = glCreateProgram();
	snprintf(prog->debug_label, sizeof(prog->debug_label), "Shader program #%i", prog->gl_handle);

	for(int i = 0; i < num_objects; ++i) {
		ShaderObject *shobj = shobjs[i];
		glAttachShader(prog->gl_handle, shobj->gl_handle);

		for(int a = 0; a < shobj->num_attribs; ++a) {
			GLSLAttribute *attr = shobj->attribs + a;
			log_debug("Binding attribute %s to location %i", attr->name, attr->location);
			glBindAttribLocation(prog->gl_handle, attr->location, attr->name);
		}
	}

	glLinkProgram(prog->gl_handle);
	print_info_log(prog->gl_handle);

	GLint link_status;
	glGetProgramiv(prog->gl_handle, GL_LINK_STATUS, &link_status);

	if(!link_status) {
		log_error("Failed to link the shader program");
		glDeleteProgram(prog->gl_handle);
		free(prog);
		return NULL;
	}

	if(!cache_uniforms(prog)) {
		gl33_shader_program_destroy(prog);
		return NULL;
	}

	return prog;
}

void gl33_shader_program_set_debug_label(ShaderProgram *prog, const char *label) {
	glcommon_set_debug_label(prog->debug_label, "Shader program", GL_PROGRAM, prog->gl_handle, label);
}

const char *gl33_shader_program_get_debug_label(ShaderProgram *prog) {
	return prog->debug_label;
}
