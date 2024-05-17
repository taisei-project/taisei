/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shader_program.h"

#include "renderer/api.h"
#include "util/kvparser.h"

static char *shader_program_path(const char *name) {
	return strjoin(SHPROG_PATH_PREFIX, name, SHPROG_EXT, NULL);
}

static bool check_shader_program_path(const char *path) {
	return strendswith(path, SHPROG_EXT) && strstartswith(path, SHPROG_PATH_PREFIX);
}

struct shprog_load_data {
	int num_objects;
	char *objlist;
};

static void load_shader_program_stage1(ResourceLoadState *st);
static void load_shader_program_stage2(ResourceLoadState *st);

static void load_shader_program_stage1(ResourceLoadState *st) {
	struct shprog_load_data ldata;
	memset(&ldata, 0, sizeof(ldata));

	char *strobjects = NULL;

	SDL_RWops *rw = res_open_file(st, st->path, VFS_MODE_READ);

	if(UNLIKELY(!rw)) {
		log_error("VFS error: %s", vfs_get_error());
		res_load_failed(st);
		return;
	}

	if(!parse_keyvalue_stream_with_spec(rw, (KVSpec[]){
		{ "glsl_objects", .out_str = &strobjects, KVSPEC_DEPRECATED("objects") },
		{ "objects",      .out_str = &strobjects },
		{ NULL }
	})) {
		SDL_RWclose(rw);
		mem_free(strobjects);
		res_load_failed(st);
		return;
	}

	SDL_RWclose(rw);

	if(strobjects) {
		ldata.objlist = mem_alloc(strlen(strobjects) + 1);
		char *listptr = ldata.objlist;
		char *objname, *srcptr = strobjects;

		while((objname = strtok_r(NULL, " \t", &srcptr))) {
			if(*objname) {
				res_load_dependency(st, RES_SHADER_OBJECT, objname);
				++ldata.num_objects;
				strcpy(listptr, objname);
				listptr += strlen(objname) + 1;
			}
		}

		mem_free(strobjects);
	}

	if(ldata.num_objects) {
		res_load_continue_on_main(st, load_shader_program_stage2, memdup(&ldata, sizeof(ldata)));
	} else {
		log_error("%s: no shader objects to link", st->path);
		mem_free(ldata.objlist);
		res_load_failed(st);
	}
}

static void load_shader_program_stage2(ResourceLoadState *st) {
	struct shprog_load_data ldata = *(struct shprog_load_data*)NOT_NULL(st->opaque);
	mem_free(st->opaque);

	ShaderObject *objs[ldata.num_objects];
	char *objname = ldata.objlist;

	for(int i = 0; i < ldata.num_objects; ++i) {
		if(!(objs[i] = res_get_data(RES_SHADER_OBJECT, objname, st->flags & ~RESF_RELOAD))) {
			log_error("%s: couldn't load shader object '%s'", st->path, objname);
			mem_free(ldata.objlist);
			res_load_failed(st);
			return;
		}

		objname += strlen(objname) + 1;
	}

	mem_free(ldata.objlist);
	ShaderProgram *prog = r_shader_program_link(ldata.num_objects, objs);

	if(prog) {
		r_shader_program_set_debug_label(prog, st->name);
		res_load_finished(st, prog);
	} else {
		log_error("%s: couldn't link shader program", st->path);
		res_load_failed(st);
	}
}

static void unload_shader_program(void *vprog) {
	r_shader_program_destroy(vprog);
}

static bool transfer_shader_program(void *dst, void *src) {
	return r_shader_program_transfer(dst, src);
}

ResourceHandler shader_program_res_handler = {
	.type = RES_SHADER_PROGRAM,
	.typename = "shader program",
	.subdir = SHPROG_PATH_PREFIX,

	.procs = {
		.find = shader_program_path,
		.check = check_shader_program_path,
		.load = load_shader_program_stage1,
		.unload = unload_shader_program,
		.transfer = transfer_shader_program,
	},
};
