/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shader_object.h"

#include "renderer/api.h"
#include "util/io.h"

struct shobj_type {
	const char *ext;
	ShaderLanguage lang;
	ShaderStage stage;
};

struct shobj_load_data {
	MemArena arena;
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

static struct shobj_type *get_shobj_type(const char *name) {
	for(struct shobj_type *type = shobj_type_table; type->ext; ++type) {
		if(strendswith(name, type->ext)) {
			return type;
		}
	}

	return NULL;
}

static char *shader_object_path(const char *name) {
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

static void load_shader_object_stage1(ResourceLoadState *st);
static void load_shader_object_stage2(ResourceLoadState *st);

static SDL_IOStream *glsl_open_callback(const char *path, void *userdata) {
	ResourceLoadState *st = userdata;
	return res_open_file(st, path, VFS_MODE_READ);
}

static void load_shader_object_stage1(ResourceLoadState *st) {
	struct shobj_type *type = get_shobj_type(st->path);

	if(type == NULL) {
		log_error("%s: can not determine shading language and/or shader stage from the filename", st->path);
		res_load_failed(st);
		return;
	}

	auto ldata = ALLOC(struct shobj_load_data);
	marena_init(&ldata->arena, 0);

	char backend_macro[32] = "BACKEND_";
	{
		const char *backend_name = r_backend_name();
		char *out = backend_macro + sizeof("BACKEND_") - 1;
		for(const char *in = backend_name; *in;) {
			*out++ = toupper(*in++);
		}
		*out = 0;
	}

	ShaderMacro macros[] = {
		{ backend_macro, "1" },
		{ NULL, },
	};

	switch(type->lang) {
		case SHLANG_GLSL: {
			GLSLSourceOptions opts = {
				.version = { 330, GLSL_PROFILE_CORE },
				.stage = type->stage,
				.macros = macros,
				.file_open_callback = glsl_open_callback,
				.file_open_callback_userdata = st,
			};

			if(!glsl_load_source(st->path, &ldata->source, &ldata->arena, &opts)) {
				goto fail;
			}

			break;
		}

		default: UNREACHABLE;
	}

	SPIRVTranspileOptions transpile_opts = {
		.compile = {
			.optimization_level = SPIRV_OPTIMIZE_PERFORMANCE,
			.filename = st->path,
		},
	};

	if(!r_shader_language_supported(&ldata->source.lang, &transpile_opts)) {
		if(!transpile_opts.decompile.lang) {
			log_error("%s: shading language not supported by backend", st->path);
			goto fail;
		}

		log_warn("%s: shading language not supported by backend, attempting to translate", st->path);

		assert(r_shader_language_supported(transpile_opts.decompile.lang, NULL));

		ShaderSource newsrc;
		bool result = spirv_transpile(&ldata->source, &newsrc, &ldata->arena, &transpile_opts);

		if(!result) {
			log_error("%s: translation failed", st->path);
			goto fail;
		}

		ldata->source = newsrc;
	}

	res_load_continue_on_main(st, load_shader_object_stage2, ldata);
	return;

fail:
	marena_deinit(&ldata->arena);
	mem_free(ldata);
	res_load_failed(st);
}

static void load_shader_object_stage2(ResourceLoadState *st) {
	struct shobj_load_data *ldata = NOT_NULL(st->opaque);

	ShaderObject *shobj = r_shader_object_compile(&ldata->source);
	marena_deinit(&ldata->arena);
	mem_free(ldata);

	if(shobj) {
		r_shader_object_set_debug_label(shobj, st->name);
		res_load_finished(st, shobj);
	} else {
		log_error("%s: failed to compile shader object", st->path);
		res_load_failed(st);
	}
}

static void unload_shader_object(void *vsha) {
	r_shader_object_destroy(vsha);
}

static bool transfer_shader_object(void *dst, void *src) {
	return r_shader_object_transfer(dst, src);
}

ResourceHandler shader_object_res_handler = {
	.type = RES_SHADER_OBJECT,
	.typename = "shader object",
	.subdir = SHOBJ_PATH_PREFIX,

	.procs = {
		.init = spirv_init_compiler,
		.shutdown = spirv_shutdown_compiler,
		.find = shader_object_path,
		.check = check_shader_object_path,
		.load = load_shader_object_stage1,
		.unload = unload_shader_object,
		.transfer = transfer_shader_object,
	},
};
