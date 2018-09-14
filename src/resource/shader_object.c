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
#include "renderer/api.h"

struct shobj_type {
	const char *ext;
	ShaderLanguage lang;
	ShaderStage stage;
};

struct shobj_load_data {
	ShaderSource source;
};

static const char *const shobj_exts[] = {
	".glsl",
	NULL,
};

static struct shobj_type shobj_type_table[] = {
	{ ".vert.glsl", SHLANG_GLSL, SHADER_STAGE_VERTEX },
	{ ".frag.glsl", SHLANG_GLSL, SHADER_STAGE_FRAGMENT },
	{ NULL }
};

static struct shobj_type* get_shobj_type(const char *name) {
	for(struct shobj_type *type = shobj_type_table; type->ext; ++type) {
		if(strendswith(name, type->ext)) {
			return type;
		}
	}

	return NULL;
}

static char* shader_object_path(const char *name) {
	char *path = NULL;

	for(const char *const *ext = shobj_exts; *ext; ++ext) {
		if((path = try_path(SHOBJ_PATH_PREFIX, name, *ext))) {
			return path;
		}
	}

	return path;
}

static bool check_shader_object_path(const char *path) {
	return strstartswith(path, SHOBJ_PATH_PREFIX) && get_shobj_type(path);
}

static void* load_shader_object_begin(const char *path, uint flags) {
	struct shobj_type *type = get_shobj_type(path);
	struct shobj_load_data *ldata = calloc(1, sizeof(struct shobj_load_data));

	switch(type->lang) {
		case SHLANG_GLSL: {
			GLSLSourceOptions opts = {
				.version = { 330, GLSL_PROFILE_CORE },
				.stage = type->stage,
			};

			if(!glsl_load_source(path, &ldata->source, &opts)) {
				free(ldata);
				return NULL;
			}

			break;
		}

		default: UNREACHABLE;
	}

	if(!r_shader_language_supported(&ldata->source.lang, NULL)) {
		log_warn("%s: shading language not supported by backend", path);
		free(ldata->source.content);
		free(ldata);
		return NULL;
	}

	return ldata;
}

static void* load_shader_object_end(void *opaque, const char *path, uint flags) {
	struct shobj_load_data *ldata = opaque;

	if(!ldata) {
		return NULL;
	}

	ShaderObject *shobj = r_shader_object_compile(&ldata->source);
	free(ldata->source.content);

	if(shobj) {
		char *basename = resource_util_basename(SHOBJ_PATH_PREFIX, path);
		r_shader_object_set_debug_label(shobj, basename);
		free(basename);
	} else {
		log_warn("%s: failed to compile shader object", path);
	}

	free(ldata);
	return shobj;
}

static void unload_shader_object(void *vsha) {
	r_shader_object_destroy(vsha);
}

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
