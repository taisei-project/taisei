/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <SDL.h>

#include "util.h"
#include "postprocess.h"
#include "resource.h"
#include "renderer/api.h"

#define PP_PATH_PREFIX SHPROG_PATH_PREFIX
#define PP_EXTENSION ".pp"

typedef struct PostprocessLoadData {
	PostprocessShader *list;
	ResourceFlags resflags;
} PostprocessLoadData;

#define SAMPLER_TAG 0xf0f0f0f0

static bool postprocess_load_callback(const char *key, const char *value, void *data) {
	PostprocessLoadData *ldata = data;
	PostprocessShader **slist = &ldata->list;
	PostprocessShader *current = *slist;

	if(!strcmp(key, "@shader")) {
		current = malloc(sizeof(PostprocessShader));
		current->uniforms = NULL;

		// if loading this fails, get_resource will print a warning
		current->shader = get_resource_data(RES_SHADER_PROGRAM, value, ldata->resflags);

		list_append(slist, current);
		return true;
	}

	for(PostprocessShader *c = current; c; c = c->next) {
		current = c;
	}

	if(!current) {
		log_error("Uniform '%s' ignored: no active shader", key);
		return true;
	}

	if(!current->shader) {
		// If loading the shader failed, just discard the uniforms.
		// We will get rid of empty shader definitions later.
		return true;
	}

	const char *name = key;
	Uniform *uni = r_shader_uniform(current->shader, name);

	if(!uni) {
		log_error("No active uniform '%s' in shader", name);
		return true;
	}

	UniformType type = r_uniform_type(uni);
	const UniformTypeInfo *type_info = r_uniform_type_info(type);

	if(UNIFORM_TYPE_IS_SAMPLER(type)) {
		Texture *tex = get_resource_data(RES_TEXTURE, value, ldata->resflags);

		if(tex) {
			PostprocessShaderUniform *psu = calloc(1, sizeof(PostprocessShaderUniform));
			psu->uniform = uni;
			psu->texture = tex;
			psu->elements = SAMPLER_TAG;
			list_append(&current->uniforms, psu);
		}

		return true;
	}

	bool integer_type;

	switch(type) {
		case UNIFORM_INT:
		case UNIFORM_IVEC2:
		case UNIFORM_IVEC3:
		case UNIFORM_IVEC4:
			integer_type = true;
			break;

		default:
			integer_type = false;
			break;
	}

	uint array_size = 0;

	PostprocessShaderUniformValue *values = NULL;
	assert(sizeof(*values) == type_info->element_size);
	uint val_idx = 0;

	while(true) {
		PostprocessShaderUniformValue v;
		char *next = (char*)value;

		if(integer_type) {
			v.i = strtol(value, &next, 10);
		} else {
			v.f = strtof(value, &next);
		}

		if(value == next) {
			break;
		}

		value = next;
		values = realloc(values, (++array_size) * type_info->elements * type_info->element_size);
		values[val_idx] = v;
		val_idx++;

		for(int i = 1; i < type_info->elements; ++i, ++val_idx) {
			PostprocessShaderUniformValue *v = values + val_idx;

			if(integer_type) {
				v->i = strtol(value, (char**)&value, 10);
			} else {
				v->f = strtof(value, (char**)&value);
			}
		}
	}

	if(val_idx == 0) {
		log_error("No values defined for uniform '%s'", name);
		return true;
	}

	assert(val_idx % type_info->elements == 0);

	PostprocessShaderUniform *psu = calloc(1, sizeof(PostprocessShaderUniform));
	psu->uniform = uni;
	psu->values = values;
	psu->elements = val_idx / type_info->elements;

	list_append(&current->uniforms, psu);

	for(int i = 0; i < val_idx; ++i) {
		log_debug("u[%i] = (f: %f; i: %i)", i, psu->values[i].f, psu->values[i].i);
	}

	return true;
}

static void* delete_uniform(List **dest, List *data, void *arg) {
	PostprocessShaderUniform *uni = (PostprocessShaderUniform*)data;
	if(uni->elements != SAMPLER_TAG) {
		free(uni->values);
	}
	free(list_unlink(dest, data));
	return NULL;
}

static void* delete_shader(List **dest, List *data, void *arg) {
	PostprocessShader *ps = (PostprocessShader*)data;
	list_foreach(&ps->uniforms, delete_uniform, NULL);
	free(list_unlink(dest, data));
	return NULL;
}

PostprocessShader* postprocess_load(const char *path, ResourceFlags flags) {
	PostprocessLoadData *ldata = calloc(1, sizeof(PostprocessLoadData));
	ldata->resflags = flags;
	parse_keyvalue_file_cb(path, postprocess_load_callback, ldata);
	PostprocessShader *list = ldata->list;
	free(ldata);

	for(PostprocessShader *s = list, *next; s; s = next) {
		next = s->next;

		if(!s->shader) {
			delete_shader((List**)&list, (List*)s, NULL);
		}
	}

	return list;
}

void postprocess_unload(PostprocessShader **list) {
	list_foreach(list, delete_shader, NULL);
}

void postprocess(PostprocessShader *ppshaders, FBPair *fbos, PostprocessPrepareFuncPtr prepare, PostprocessDrawFuncPtr draw, double width, double height, void *arg) {
	if(!ppshaders) {
		return;
	}

	ShaderProgram *shader_saved = r_shader_current();
	BlendMode blend_saved = r_blend_current();

	r_blend(BLEND_NONE);

	for(PostprocessShader *pps = ppshaders; pps; pps = pps->next) {
		ShaderProgram *s = pps->shader;

		r_framebuffer(fbos->back);
		r_shader_ptr(s);

		if(prepare) {
			prepare(fbos->back, s, arg);
		}

		for(PostprocessShaderUniform *u = pps->uniforms; u; u = u->next) {
			if(u->elements == SAMPLER_TAG) {
				r_uniform_sampler(u->uniform, u->texture);
			} else {
				r_uniform_ptr_unsafe(u->uniform, 0, u->elements, u->values);
			}
		}

		draw(fbos->front, width, height);
		fbpair_swap(fbos);
	}

	r_shader_ptr(shader_saved);
	r_blend(blend_saved);
}

/*
 *  Glue for resources api
 */

static char *postprocess_path(const char *name) {
	return strjoin(PP_PATH_PREFIX, name, PP_EXTENSION, NULL);
}

static bool check_postprocess_path(const char *path) {
	return strendswith(path, PP_EXTENSION);
}

static void load_postprocess_stage2(ResourceLoadState *st) {
	PostprocessShader *pp = postprocess_load(st->path, st->flags);

	if(pp) {
		res_load_finished(st, pp);
	} else {
		res_load_failed(st);
	}
}

static void load_postprocess_stage1(ResourceLoadState *st) {
	res_load_continue_on_main(st, load_postprocess_stage2, NULL);
}

static void unload_postprocess(void *vlist) {
	postprocess_unload((PostprocessShader**)&vlist);
}

ResourceHandler postprocess_res_handler = {
	.type = RES_POSTPROCESS,
	.typename = "postprocessing pipeline",
	.subdir = PP_PATH_PREFIX,

	.procs = {
		.find = postprocess_path,
		.check = check_postprocess_path,
		.load = load_postprocess_stage1,
		.unload = unload_postprocess,
	},
};
