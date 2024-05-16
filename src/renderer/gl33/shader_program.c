/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "gl33.h"
#include "shader_program.h"
#include "shader_object.h"
#include "texture.h"
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
} type_to_accessors[]      = {
	[UNIFORM_FLOAT]        = { uset_float, uget_float },
	[UNIFORM_VEC2]         = { uset_vec2,  uget_vec2 },
	[UNIFORM_VEC3]         = { uset_vec3,  uget_vec3 },
	[UNIFORM_VEC4]         = { uset_vec4,  uget_vec4 },
	[UNIFORM_INT]          = { uset_int,   uget_int },
	[UNIFORM_IVEC2]        = { uset_ivec2, uget_ivec2 },
	[UNIFORM_IVEC3]        = { uset_ivec3, uget_ivec3 },
	[UNIFORM_IVEC4]        = { uset_ivec4, uget_ivec4 },
	[UNIFORM_SAMPLER_2D]   = { uset_int,   uget_int },
	[UNIFORM_SAMPLER_CUBE] = { uset_int,   uget_int },
	[UNIFORM_MAT3]         = { uset_mat3,  uget_mat3 },
	[UNIFORM_MAT4]         = { uset_mat4,  uget_mat4 },
};

typedef struct MagicUniformSpec {
	const char *name;
	const char *typename;
	UniformType type;
} MagicUniformSpec;

static MagicUniformSpec magic_unfiroms[] = {
	[UMAGIC_MATRIX_MV]       = { "r_modelViewMatrix",     "mat4", UNIFORM_MAT4 },
	[UMAGIC_MATRIX_PROJ]     = { "r_projectionMatrix",    "mat4", UNIFORM_MAT4 },
	[UMAGIC_MATRIX_TEX]      = { "r_textureMatrix",       "mat4", UNIFORM_MAT4 },
	[UMAGIC_COLOR]           = { "r_color",               "vec4", UNIFORM_VEC4 },
	[UMAGIC_VIEWPORT]        = { "r_viewport",            "vec4", UNIFORM_VEC4 },
	[UMAGIC_COLOR_OUT_SIZES] = { "r_colorOutputSizes[0]", "vec2", UNIFORM_VEC2 },
	[UMAGIC_DEPTH_OUT_SIZE]  = { "r_depthOutputSize",     "vec2", UNIFORM_VEC2 },
};
static_assert(ARRAY_SIZE(magic_unfiroms) == NUM_MAGIC_UNIFORMS);

static void gl33_update_uniform(Uniform *uniform, uint offset, uint count, const void *data) {
	// these are validated properly in gl33_uniform
	assert(offset < uniform->array_size);
	assert(offset + count <= uniform->array_size);

	uint idx_first = offset;
	uint idx_last = offset + count - 1;

	assert(idx_last < uniform->array_size);
	assert(idx_first <= idx_last);

	memcpy(uniform->cache.pending + offset * uniform->elem_size, data, count * uniform->elem_size);

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

static GLuint get_texture_target(Texture *tex, UniformType utype) {
	if(tex) {
		return tex->bind_target;
	}

	switch(utype) {
		case UNIFORM_SAMPLER_2D:    return GL_TEXTURE_2D;
		case UNIFORM_SAMPLER_CUBE:  return GL_TEXTURE_CUBE_MAP;
		default: UNREACHABLE;
	}
}

static void *gl33_sync_uniform(const char *key, void *value, void *arg) {
	Uniform *uniform = value;

	// special case: for sampler uniforms, we have to construct the actual data from the texture pointers array.
	UniformType utype = uniform->type;
	if(UNIFORM_TYPE_IS_SAMPLER(utype)) {
		Uniform *size_uniform = uniform->size_uniform;

		for(uint i = 0; i < uniform->array_size; ++i) {
			Texture *tex = uniform->textures[i];
			GLuint preferred_unit = CASTPTR_ASSUME_ALIGNED(uniform->cache.pending, int)[i];
			GLuint unit = gl33_bind_texture(tex, get_texture_target(tex, utype), preferred_unit);

			if(unit != preferred_unit) {
				gl33_update_uniform(uniform, i, 1, &unit);
			}

			if(size_uniform) {
				uint w, h;

				if(tex) {
					r_texture_get_size(tex, 0, &w, &h);
				} else {
					w = h = 0;
				}

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
	if(UNIFORM_TYPE_IS_SAMPLER(uniform->type)) {
		Texture **textures = (Texture**)data;

		for(uint i = 0; i < count; ++i) {
			if(textures[i]) {
				assert(gl33_texture_sampler_compatible(textures[i], uniform->type));
			}
		}

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
			case GL_FLOAT:        uni.type = UNIFORM_FLOAT;        break;
			case GL_FLOAT_VEC2:   uni.type = UNIFORM_VEC2;         break;
			case GL_FLOAT_VEC3:   uni.type = UNIFORM_VEC3;         break;
			case GL_FLOAT_VEC4:   uni.type = UNIFORM_VEC4;         break;
			case GL_INT:          uni.type = UNIFORM_INT;          break;
			case GL_INT_VEC2:     uni.type = UNIFORM_IVEC2;        break;
			case GL_INT_VEC3:     uni.type = UNIFORM_IVEC3;        break;
			case GL_INT_VEC4:     uni.type = UNIFORM_IVEC4;        break;
			case GL_SAMPLER_2D:   uni.type = UNIFORM_SAMPLER_2D;   break;
			case GL_SAMPLER_CUBE: uni.type = UNIFORM_SAMPLER_CUBE; break;
			case GL_FLOAT_MAT3:   uni.type = UNIFORM_MAT3;         break;
			case GL_FLOAT_MAT4:   uni.type = UNIFORM_MAT4;         break;

			default:
				log_warn("Uniform '%s' is of an unsupported type 0x%04x and will be ignored.", name, type);
				continue;
		}

		MagicUniformIndex magic_index = UMAGIC_INVALID;

		for(int j = 0; j < ARRAY_SIZE(magic_unfiroms); ++j) {
			MagicUniformSpec *m = magic_unfiroms + j;

			if(strcmp(name, m->name)) {
				continue;
			}

			if(uni.type != m->type) {
				log_error("Magic uniform '%s' must be of type '%s'", name, m->typename);
				return false;
			}

			magic_index = j;
			break;
		}

		const UniformTypeInfo *typeinfo = r_uniform_type_info(uni.type);

		uni.location = loc;
		uni.array_size = size;

		if(UNIFORM_TYPE_IS_SAMPLER(uni.type)) {
			uni.elem_size = sizeof(GLint);
			uni.textures = ALLOC_ARRAY(uni.array_size, typeof(*uni.textures));
		} else {
			uni.elem_size = typeinfo->element_size * typeinfo->elements;
		}

		uni.cache.commited = mem_alloc_array(uni.array_size, uni.elem_size);
		uni.cache.pending = mem_alloc_array(uni.array_size, uni.elem_size);
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

		if(UNIFORM_TYPE_IS_SAMPLER(uni.type)) {
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

		if(magic_index != UMAGIC_INVALID) {
			assume((uint)magic_index < ARRAY_SIZE(prog->magic_uniforms));
			assert(prog->magic_uniforms[magic_index] == NULL);
			prog->magic_uniforms[magic_index] = new_uni;
		}

		ht_set(&prog->uniforms, name, new_uni);
		log_debug("%s = %i [array elements: %i; size: %zi bytes]", name, loc, uni.array_size, uni.array_size * uni.elem_size);
	}

	ht_str2ptr_iter_t iter;
	ht_iter_begin(&prog->uniforms, &iter);
	while(iter.has_data) {
		Uniform *u = iter.value;

		if(UNIFORM_TYPE_IS_SAMPLER(u->type)) {
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
		assert(UNIFORM_TYPE_IS_SAMPLER(u->type));

		if(!u->textures) {
			continue;
		}

		for(Texture **slot = u->textures; slot < u->textures + u->array_size; ++slot) {
			assert(slot != NULL);
			if(*slot == tex) {
				*slot = NULL;
			}
		}
	}
}

void gl33_uniforms_handle_texture_pointer_renamed(Texture *pold, Texture *pnew) {
	for(Uniform *u = sampler_uniforms; u; u = u->next) {
		assert(UNIFORM_TYPE_IS_SAMPLER(u->type));

		for(Texture **slot = u->textures; slot < u->textures + u->array_size; ++slot) {
			assert(slot != NULL);
			if(*slot == pold) {
				*slot = pnew;
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

	if(UNIFORM_TYPE_IS_SAMPLER(uniform->type)) {
		list_unlink(&sampler_uniforms, uniform);
	}

	mem_free(uniform->textures);
	mem_free(uniform->cache.commited);
	mem_free(uniform->cache.pending);
	mem_free(uniform);
	return NULL;
}

void gl33_shader_program_destroy(ShaderProgram *prog) {
	gl33_shader_deleted(prog);
	glDeleteProgram(prog->gl_handle);
	ht_foreach(&prog->uniforms, free_uniform, NULL);
	ht_destroy(&prog->uniforms);
	mem_free(prog);
}

ShaderProgram *gl33_shader_program_link(uint num_objects, ShaderObject *shobjs[num_objects]) {
	auto prog = ALLOC(ShaderProgram);

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
		mem_free(prog);
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

bool gl33_shader_program_transfer(ShaderProgram *dst, ShaderProgram *src) {
	ht_ptr2ptr_t old_new_map;  // bidirectional
	ht_create(&old_new_map);

	ht_str2ptr_iter_t iter;
	ht_iter_begin(&src->uniforms, &iter);

	bool fail = false;

	for(; iter.has_data; ht_iter_next(&iter)) {
		Uniform *unew = NOT_NULL(iter.value);
		Uniform *uold = ht_get(&dst->uniforms, iter.key, NULL);

		if(!uold) {
			continue;
		}

		if(unew->type != uold->type || unew->elem_size != uold->elem_size) {
			log_error(
				"Can't update shader program '%s': uniform %s changed type",
				dst->debug_label, iter.key
			);
			fail = true;
			break;
		}

		ht_set(&old_new_map, uold, unew);
		ht_set(&old_new_map, unew, uold);
	}

	ht_iter_end(&iter);

	if(fail) {
		ht_destroy(&old_new_map);
		gl33_shader_program_destroy(src);
		return false;
	}

	// Update existing uniforms

	ht_iter_begin(&dst->uniforms, &iter);

	for(; iter.has_data; ht_iter_next(&iter)) {
		Uniform *uold = NOT_NULL(iter.value);
		Uniform *unew = ht_get(&old_new_map, uold, NULL);

		mem_free(uold->textures);
		mem_free(uold->cache.pending);
		mem_free(uold->cache.commited);

		if(unew) {
			uold->textures = unew->textures;
			assert(uold->elem_size == unew->elem_size);
			uold->array_size = unew->array_size;
			uold->location = unew->location;
			assert(uold->type == unew->type);
			uold->size_uniform = ht_get(&old_new_map, unew->size_uniform, unew->size_uniform);
			uold->cache = unew->cache;

			if(UNIFORM_TYPE_IS_SAMPLER(unew->type)) {
				list_unlink(&sampler_uniforms, unew);
			}

			ht_unset(&src->uniforms, iter.key);
			mem_free(unew);
		} else {
			// Deactivate, but keep the object around, because user code may be referencing it.
			// We also need to keep type information, in case the uniform gets re-introduced.
			uold->location = INVALID_UNIFORM_LOCATION;
			uold->size_uniform = NULL;
			uold->array_size = 0;
			uold->textures = NULL;
			uold->cache.pending = NULL;
			uold->cache.commited = NULL;
		}
	}

	ht_iter_end(&iter);

	// Add new uniforms

	ht_iter_begin(&src->uniforms, &iter);

	for(; iter.has_data; ht_iter_next(&iter)) {
		Uniform *unew = NOT_NULL(iter.value);
		assert(ht_get(&old_new_map, unew, NULL) == NULL);
		assert(ht_get(&dst->uniforms, iter.key, NULL) == NULL);

		unew->prog = dst;
		unew->size_uniform = ht_get(&old_new_map, unew->size_uniform, unew->size_uniform);
		ht_set(&dst->uniforms, iter.key, unew);
	}

	ht_iter_end(&iter);

	dst->gl_handle = src->gl_handle;
	memcpy(dst->debug_label, src->debug_label, sizeof(dst->debug_label));

	for(int i = 0; i < ARRAY_SIZE(dst->magic_uniforms); ++i) {
		Uniform *unew = src->magic_uniforms[i];
		dst->magic_uniforms[i] = ht_get(&old_new_map, unew, unew);
	}

	ht_destroy(&old_new_map);
	ht_destroy(&src->uniforms);
	mem_free(src);

	return true;
}
