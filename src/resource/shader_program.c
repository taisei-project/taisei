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

static char *shader_program_path(const char *name) {
	return strjoin(RES_PATHPREFIX_SHADERPROG, name, SHPROG_EXT, NULL);
}

static bool check_shader_program_path(const char *path) {
	return strendswith(path, SHPROG_EXT) && strstartswith(path, RES_PATHPREFIX_SHADERPROG);
}

static void *load_shader_program_begin(ResourceLoadInfo li) {
	char *strobjects = NULL;

	if(!parse_keyvalue_file_with_spec(li.path, (KVSpec[]){
		{ "glsl_objects", .out_str = &strobjects, KVSPEC_DEPRECATED("objects") },
		{ "objects",      .out_str = &strobjects },
		{ NULL }
	})) {
		free(strobjects);
		return NULL;
	}

	ResourceRefGroup *shader_objs = NULL;

	if(strobjects) {
		shader_objs = calloc(1, sizeof(*shader_objs));
		res_group_init(shader_objs, 2);
		char *objname, *srcptr = strobjects;

		while((objname = strtok_r(NULL, " \t", &srcptr))) {
			if(*objname) {
				res_group_add_ref(shader_objs, res_ref(RES_SHADEROBJ, objname, li.flags));
			}
		}

		free(strobjects);
	}

	if(!shader_objs || !shader_objs->num_used) {
		log_error("%s: no shader objects to link", li.path);
		res_group_destroy(shader_objs);
		return NULL;
	}

	return shader_objs;
}

attr_nonnull(2)
static void *load_shader_program_end(ResourceLoadInfo li, void *opaque) {
	ResourceRefGroup *shader_objs = opaque;

	ShaderObject *objs[shader_objs->num_used];

	for(uint i = 0; i < shader_objs->num_used; ++i) {
		ResourceRef *ref = shader_objs->refs + i;

		if(!(objs[i] = res_ref_data(*ref))) {
			log_error("%s: couldn't load shader object '%s'", li.path, res_ref_name(*ref));
			res_group_destroy(shader_objs);
			free(shader_objs);
			return NULL;
		}
	}

	ShaderProgram *prog = r_shader_program_link(shader_objs->num_used, objs);
	res_group_destroy(shader_objs);
	free(shader_objs);

	if(prog) {
		r_shader_program_set_debug_label(prog, li.name);
	} else {
		log_error("%s: couldn't link shader program", li.path);
	}

	return prog;
}

static void unload_shader_program(void *vprog) {
	r_shader_program_destroy(vprog);
}

ResourceHandler shader_program_res_handler = {
	.type = RES_SHADERPROG,

	.procs = {
		.find = shader_program_path,
		.check = check_shader_program_path,
		.begin_load = load_shader_program_begin,
		.end_load = load_shader_program_end,
		.unload = unload_shader_program,
	},
};
