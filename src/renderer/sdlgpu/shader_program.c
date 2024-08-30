/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "shader_program.h"
#include "shader_object.h"  // IWYU pragma: keep
#include "pipeline_cache.h"

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

	ShaderProgram *prog = mem_dup(&p, sizeof(p));
	marena_init(&prog->arena, 1 << 11);
	ht_create(&prog->uniforms);
	prog->id = ++sdlgpu.ids.shader_programs;

	snprintf(prog->debug_label, sizeof(prog->debug_label), "Shader program #%u", prog->id);

	return prog;
}

void sdlgpu_shader_program_destroy(ShaderProgram *prog) {
	sdlgpu_pipecache_unref_shader_program(prog->id);
	sdlgpu_shader_object_destroy(prog->stages.fragment);
	sdlgpu_shader_object_destroy(prog->stages.vertex);
	marena_deinit(&prog->arena);
	ht_destroy(&prog->uniforms);
	mem_free(prog);
}

void sdlgpu_shader_program_set_debug_label(ShaderProgram *prog, const char *label) {
	strlcpy(prog->debug_label, label, sizeof(prog->debug_label));
}

const char* sdlgpu_shader_program_get_debug_label(ShaderProgram *prog) {
	return prog->debug_label;
}

typedef struct PerStageUniforms {
	ShaderObjectUniform *vert;
	ShaderObjectUniform *frag;
} PerStageUniforms;

static PerStageUniforms sdlgpu_shader_uniform_get_per_stage(const Uniform *uni) {
	auto prog = uni->shader;

	return (PerStageUniforms) {
		.vert = ht_get_prehashed(&prog->stages.vertex->uniforms, uni->name, uni->hash, NULL),
		.frag = ht_get_prehashed(&prog->stages.fragment->uniforms, uni->name, uni->hash, NULL),
	};
}

static bool sdlgpu_shader_uniform_validate(Uniform *uni, PerStageUniforms *out_stguniforms) {
	auto stg = sdlgpu_shader_uniform_get_per_stage(uni);

	if(!stg.vert && !stg.frag) {
		return false;
	}

	if(stg.vert && stg.frag && !sdlgpu_shader_object_uniform_types_compatible(stg.vert, stg.frag)) {
		log_error("Uniform %s has different types in vertex and fragment stages, this is not supported", uni->name);
		return false;
	}

	if(out_stguniforms) {
		*out_stguniforms = stg;
	}

	return true;
}

Uniform *sdlgpu_shader_uniform(ShaderProgram *prog, const char *name, hash_t name_hash) {
	Uniform *uni = NULL;
	if(!ht_lookup_prehashed(&prog->uniforms, name, name_hash, (void**)&uni)) {
		Uniform u = {
			.shader = prog,
			.name = name,
			.hash = name_hash,
		};

		if(sdlgpu_shader_uniform_validate(&u, NULL)) {
			u.name = marena_strdup(&prog->arena, u.name);
			uni = ARENA_ALLOC(&prog->arena, typeof(*uni), u);
			ht_set(&prog->uniforms, name, uni);
		}
	}

	return uni;
}

UniformType sdlgpu_uniform_type(Uniform *uniform) {
	PerStageUniforms stg;

	if(!sdlgpu_shader_uniform_validate(uniform, &stg)) {
		return UNIFORM_UNKNOWN;
	}

	return (stg.frag ?: stg.vert)->type;
}

void sdlgpu_uniform(Uniform *uni, uint offset, uint count, const void *data) {
	PerStageUniforms stg;

	if(!sdlgpu_shader_uniform_validate(uni, &stg)) {
		return;
	}

	sdlgpu_shader_object_uniform_set_data(uni->shader->stages.vertex, stg.vert, offset, count, data);
	sdlgpu_shader_object_uniform_set_data(uni->shader->stages.fragment, stg.frag, offset, count, data);
}

void sdlgpu_unref_texture_from_samplers(Texture *tex) {
	UNREACHABLE;
}

void sdlgpu_uniforms_handle_texture_pointer_renamed(Texture *pold, Texture *pnew) {
	UNREACHABLE;
}

bool sdlgpu_shader_program_transfer(ShaderProgram *dst, ShaderProgram *src) {
	// *dst = *src;
	log_error("FIXME FIXME FIXME");
	return false;
}
