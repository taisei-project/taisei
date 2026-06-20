/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "model.h"

#include "memory/scratch.h"
#include "renderer/api.h"
#include "resource.h"

#include "iqm_loader/iqm_loader.h"

#define MDL_PATH_PREFIX "res/models/"
#define MDL_EXTENSION ".iqm"

typedef struct ModelLoadData {
	IQMModel iqm;
} ModelLoadData;

static void load_model_stage1(ResourceLoadState *st);
static void load_model_stage2(ResourceLoadState *st);

static void load_model_stage1(ResourceLoadState *st) {
	const char *path = st->path;
	SDL_IOStream *rw = vfs_open(path, VFS_MODE_READ);

	if(!rw) {
		log_error("VFS error: %s", vfs_get_error());
		res_load_failed(st);
		return;
	}

	IQMModel iqm;
	bool loaded = iqm_load_stream(&iqm, path, rw);
	SDL_CloseIO(rw);

	if(loaded) {
		res_load_continue_on_main(st, load_model_stage2, ALLOC(ModelLoadData, {
			.iqm = iqm,
		}));
	} else {
		res_load_failed(st);
	}
}

static void load_model_stage2(ResourceLoadState *st) {
	ModelLoadData *ldata = NOT_NULL(st->opaque);
	auto mdl = ALLOC(Model);

	r_model_add_static(
		mdl,
		PRIM_TRIANGLES,
		ldata->iqm.num_vertices,
		ldata->iqm.vertices,
		ldata->iqm.num_indices,
		ldata->iqm.indices
	);

	mem_free(ldata->iqm.vertices);
	mem_free(ldata->iqm.indices);
	mem_free(ldata);

	res_load_finished(st, mdl);
}

static char *model_path(const char *name) {
	StringBuffer buf = { acquire_scratch_arena() };
	strbuf_cat(&buf, MDL_PATH_PREFIX);
	strbuf_cat(&buf, name);
	strbuf_cat(&buf, MDL_EXTENSION);
	char *s = mem_strdup(buf.start);
	release_scratch_arena(buf.arena);
	return s;
}

static bool check_model_path(const char *path) {
	return strendswith(path, MDL_EXTENSION);
}

static void unload_model(void *model) { // Does not delete elements from the VBO, so doing this at runtime is leaking VBO space
	mem_free(model);
}

ResourceHandler model_res_handler = {
	.type = RES_MODEL,
	.typename = "model",
	.subdir = MDL_PATH_PREFIX,

	.procs = {
		.find = model_path,
		.check = check_model_path,
		.load = load_model_stage1,
		.unload = unload_model,
	},
};
