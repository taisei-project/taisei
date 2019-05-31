/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
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
		if((path = try_path(RES_PATHPREFIX_SHADEROBJ, name, *ext))) {
			return path;
		}
	}

	return path;
}

static bool check_shader_object_path(const char *path) {
	return strstartswith(path, RES_PATHPREFIX_SHADEROBJ) && get_shobj_type(path);
}

static void *load_shader_object_begin(ResourceLoadInfo li) {
	struct shobj_type *type = get_shobj_type(li.path);

	if(type == NULL) {
		log_error("%s: can not determine shading language and/or shader stage from the filename", li.path);
		return NULL;
	}

	struct shobj_load_data *ldata = calloc(1, sizeof(struct shobj_load_data));

	switch(type->lang) {
		case SHLANG_GLSL: {
			GLSLSourceOptions opts = {
				.version = { 330, GLSL_PROFILE_CORE },
				.stage = type->stage,
			};

			if(!glsl_load_source(li.path, &ldata->source, &opts)) {
				goto fail;
			}

			break;
		}

		default: UNREACHABLE;
	}

	ShaderLangInfo altlang = { SHLANG_INVALID };

	if(!r_shader_language_supported(&ldata->source.lang, &altlang)) {
		if(altlang.lang == SHLANG_INVALID) {
			log_error("%s: shading language not supported by backend", li.path);
			goto fail;
		}

		log_warn("%s: shading language not supported by backend, attempting to translate", li.path);

		assert(r_shader_language_supported(&altlang, NULL));

		ShaderSource newsrc;
		bool result = spirv_transpile(&ldata->source, &newsrc, &(SPIRVTranspileOptions) {
			.lang = &altlang,
			.optimization_level = SPIRV_OPTIMIZE_PERFORMANCE,
			.filename = li.path,
		});

		if(!result) {
			log_error("%s: translation failed", li.path);
			goto fail;
		}

		free(ldata->source.content);
		ldata->source = newsrc;
	}

	return ldata;

fail:
	free(ldata->source.content);
	free(ldata);
	return NULL;
}

attr_nonnull(2)
static void *load_shader_object_end(ResourceLoadInfo loadinfo, void *opaque) {
	struct shobj_load_data *ldata = opaque;

	ShaderObject *shobj = r_shader_object_compile(&ldata->source);
	free(ldata->source.content);

	if(shobj) {
		r_shader_object_set_debug_label(shobj, loadinfo.name);
	} else {
		log_error("%s: failed to compile shader object", loadinfo.path);
	}

	free(ldata);
	return shobj;
}

static void unload_shader_object(void *vsha) {
	r_shader_object_destroy(vsha);
}

ResourceHandler shader_object_res_handler = {
	.type = RES_SHADEROBJ,

	.procs = {
		.init = spirv_init_compiler,
		.shutdown = spirv_shutdown_compiler,
		.find = shader_object_path,
		.check = check_shader_object_path,
		.begin_load = load_shader_object_begin,
		.end_load = load_shader_object_end,
		.unload = unload_shader_object,
	},
};
