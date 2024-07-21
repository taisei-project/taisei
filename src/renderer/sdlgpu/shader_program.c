/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shader_program.h"
#include "shader_object.h"  // IWYU pragma: keep

#include "util.h"

ShaderProgram *sdlgpu_shader_program_link(uint num_objects, ShaderObject *shobjs[num_objects]) {
	ShaderProgram p = {};

	for(uint i = 0; i < num_objects; ++i) {
		auto shobj = shobjs[i];
		uint stage_idx = shobj->stage - (SHADER_STAGE_INVALID + 1);
		assume(stage_idx < ARRAY_SIZE(p.stages.as_array));
		ShaderObject **target = &p.stages.as_array[stage_idx];

		if(*target) {
			log_error("Linking more than one shader object per stage is not supported");
			return NULL;
		}

		log_debug("Link stage %i: %p %s", shobj->stage, shobj, shobj->debug_label);

		*target = shobj;
		SDL_AtomicIncRef(&shobj->refs);
		log_debug("v = %p; f = %p", p.stages.vertex, p.stages.fragment);
	}

	if(!p.stages.vertex) {
		log_error("Missing shader for the vertex stage");
		return NULL;
	}

	if(!p.stages.fragment) {
		log_error("Missing shader for the fragment stage");
		return NULL;
	}

	return mem_dup(&p, sizeof(p));
}

void sdlgpu_shader_program_destroy(ShaderProgram *prog) {
	sdlgpu_shader_object_destroy(prog->stages.fragment);
	sdlgpu_shader_object_destroy(prog->stages.vertex);
	mem_free(prog);
}

void sdlgpu_shader_program_set_debug_label(ShaderProgram *prog, const char *label) {
	strlcpy(prog->debug_label, label, sizeof(prog->debug_label));
}

const char* sdlgpu_shader_program_get_debug_label(ShaderProgram *prog) {
	return prog->debug_label;
}

Uniform *sdlgpu_shader_uniform(ShaderProgram *prog, const char *uniform_name, hash_t uniform_name_hash) {
	log_error("TODO %s %s %x", prog->debug_label, uniform_name, uniform_name_hash);
	return NULL;
}

UniformType sdlgpu_uniform_type(Uniform *uniform) {
	return UNIFORM_UNKNOWN;
}

void sdlgpu_uniform(Uniform *uniform, uint offset, uint count, const void *data) {
	UNREACHABLE;
}

void sdlgpu_unref_texture_from_samplers(Texture *tex) {
	UNREACHABLE;
}

void sdlgpu_uniforms_handle_texture_pointer_renamed(Texture *pold, Texture *pnew) {
	UNREACHABLE;
}

bool sdlgpu_shader_program_transfer(ShaderProgram *dst, ShaderProgram *src) {
	*dst = *src;
	return true;
}
