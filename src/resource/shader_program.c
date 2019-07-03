/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "util.h"
#include "shader_program.h"
#include "renderer/api.h"

static char* shader_program_path(const char *name) {
	return strjoin(SHPROG_PATH_PREFIX, name, SHPROG_EXT, NULL);
}

static bool check_shader_program_path(const char *path) {
	return strendswith(path, SHPROG_EXT) && strstartswith(path, SHPROG_PATH_PREFIX);
}

struct shprog_load_data {
	int num_objects;
	uint load_flags;
	char *objlist;
};

static void* load_shader_program_begin(const char *path, uint flags) {
	struct shprog_load_data ldata;
	memset(&ldata, 0, sizeof(ldata));
	ldata.load_flags = flags;

	char *strobjects = NULL;

	if(!parse_keyvalue_file_with_spec(path, (KVSpec[]){
		{ "glsl_objects", .out_str = &strobjects, KVSPEC_DEPRECATED("objects") },
		{ "objects",      .out_str = &strobjects },
		{ NULL }
	})) {
		free(ldata.objlist);
		return NULL;
	}

	if(strobjects) {
		ldata.objlist = calloc(1, strlen(strobjects) + 1);
		char *listptr = ldata.objlist;
		char *objname, *srcptr = strobjects;

		while((objname = strtok_r(NULL, " \t", &srcptr))) {
			if(*objname) {
				preload_resource(RES_SHADER_OBJECT, objname, ldata.load_flags);
				++ldata.num_objects;
				strcpy(listptr, objname);
				listptr += strlen(objname) + 1;
			}
		}

		free(strobjects);
	}

	if(!ldata.num_objects) {
		log_error("%s: no shader objects to link", path);
		free(ldata.objlist);
		return NULL;
	}

	return memdup(&ldata, sizeof(ldata));
}

static void* load_shader_program_end(void *opaque, const char *path, uint flags) {
	if(!opaque) {
		return NULL;
	}

	struct shprog_load_data ldata;
	memcpy(&ldata, opaque, sizeof(ldata));
	free(opaque);

	ShaderObject *objs[ldata.num_objects];
	char *objname = ldata.objlist;

	for(int i = 0; i < ldata.num_objects; ++i) {
		if(!(objs[i] = get_resource_data(RES_SHADER_OBJECT, objname, ldata.load_flags))) {
			log_error("%s: couldn't load shader object '%s'", path, objname);
			free(ldata.objlist);
			return NULL;
		}

		objname += strlen(objname) + 1;
	}

	free(ldata.objlist);
	ShaderProgram *prog = r_shader_program_link(ldata.num_objects, objs);

	if(prog) {
		char *basename = resource_util_basename(SHPROG_PATH_PREFIX, path);
		r_shader_program_set_debug_label(prog, basename);
		free(basename);
	} else {
		log_error("%s: couldn't link shader program", path);
	}

	return prog;
}

static void unload_shader_program(void *vprog) {
	r_shader_program_destroy(vprog);
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
