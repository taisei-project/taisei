/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "pipeline_cache.h"

#include "shader_program.h"
#include "shader_object.h"
#include "vertex_array.h"

static hash_t pkey_hash(const PipelineCacheKey *k) {
	union {
		PipelineCacheKey k;
		uint64_t u[(sizeof(PipelineCacheKey) - 1) / sizeof(uint64_t) + 1];
	} u = { .k = *k };

	hash_t result = 0;

	for(int i = 0; i < ARRAY_SIZE(u.u); ++i) {
		result ^= htutil_hashfunc_uint64(u.u[i]);
	}

	return result;
}

static bool pkeys_eq(const PipelineCacheKey *restrict a, const PipelineCacheKey *restrict b) {
	return !memcmp(a, b, sizeof(*a));
}

#define HT_SUFFIX                      pcache
#define HT_KEY_TYPE                    PipelineCacheKey
#define HT_VALUE_TYPE                  SDL_GPUGraphicsPipeline *
#define HT_FUNC_HASH_KEY(key)          pkey_hash(&(key))
#define HT_FUNC_KEYS_EQUAL(key1, key2) pkeys_eq(&(key1), &(key2))
#define HT_KEY_FMT                     PRIx32
#define HT_KEY_PRINTABLE(key)          pkey_hash(&(key))
#define HT_VALUE_FMT                   "p"
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_DECL
#define HT_IMPL
#include "hashtable_incproxy.inc.h"

static struct {
	ht_pcache_t cache;
} pcache;

void sdlgpu_pipecache_init(void) {
	ht_pcache_create(&pcache.cache);
}

void sdlgpu_pipecache_wipe(void) {
	ht_pcache_iter_t iter;
	ht_pcache_iter_begin(&pcache.cache, &iter);
	for(;iter.has_data; ht_pcache_iter_next(&iter)) {
		SDL_ReleaseGPUGraphicsPipeline(sdlgpu.device, iter.value);
	}
	ht_pcache_iter_end(&iter);

	ht_pcache_unset_all(&pcache.cache);
}

static PipelineCacheKey sdlgpu_pipecache_construct_key(PipelineDescription *restrict pd) {
	PipelineCacheKey k = {};

	bool enable_cull_face attr_unused = pd->cap_bits & r_capability_bit(RCAP_CULL_FACE);
	bool enable_depth_test = pd->cap_bits & r_capability_bit(RCAP_DEPTH_TEST);
	bool enable_depth_write = pd->cap_bits & r_capability_bit(RCAP_DEPTH_WRITE);

	k.blend = pd->blend_mode;
	k.shader_program = pd->shader_program->id;
	k.vertex_array = pd->vertex_array->layout_id;
	k.primitive = pd->primitive;
	k.cull_mode = pd->cull_mode;
	k.depth_func = pd->depth_func;
	k.depth_test = enable_depth_test;
	k.depth_write = enable_depth_write;
	k.depth_format = pd->depth_format & PIPECACHE_FMT_MASK;
	k.front_face_ccw = (pd->front_face == SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE);

	if(pd->depth_format == SDL_GPU_TEXTUREFORMAT_INVALID) {
		assert(k.depth_format == PIPECACHE_FMT_MASK);
	} else {
		assert(k.depth_format == pd->depth_format);
	}

	assert(k.cull_mode || !enable_cull_face);

	for(uint i = 0; i < ARRAY_SIZE(pd->outputs); ++i) {
		uint fmt = pd->outputs[i].format;
		assert(fmt == (fmt & PIPECACHE_FMT_MASK));

		if(i < pd->num_outputs) {
			assert(fmt != PIPECACHE_FMT_MASK);
		} else {
			assert(fmt == PIPECACHE_FMT_MASK);
		}

		k.attachment_formats |= (fmt << (i * PIPECACHE_KEY_TEXFMT_BITS));
	}

	return k;
}

static void dump_vertex_input_state(SDL_GPUVertexInputState *st) {
	log_debug(" *** BINDINGS:");
	for(uint i = 0; i < st->vertexBindingCount; ++i) {
			log_debug("[%u] binding:   %u", i, st->vertexBindings[i].binding);
			log_debug("[%u] stride:    %u", i, st->vertexBindings[i].stride);
			log_debug("[%u] inputRate: %u", i, st->vertexBindings[i].inputRate);
			log_debug("[%u] stepRate:  %u", i, st->vertexBindings[i].instanceStepRate);
			// log_debug("[%u] attachment map: %u", i, varr->binding_to_attachment_map[i]);
	}

	log_debug(" *** ATTRIBS:");
	for(uint i = 0; i < st->vertexAttributeCount; ++i) {
			log_debug("[%u] location:  %u", i, st->vertexAttributes[i].location);
			log_debug("[%u] binding:   %u", i, st->vertexAttributes[i].binding);
			log_debug("[%u] format:    %u", i, st->vertexAttributes[i].format);
			log_debug("[%u] offset:    %u", i, st->vertexAttributes[i].offset);
	}
}

#define PIPECACHE_KEY_REPR_SIZE (sizeof(PipelineCacheKey) * 2 + 1)

attr_unused
static inline void sdlgpu_pipecache_key_repr(const PipelineCacheKey key, size_t buf_size, char buf[buf_size]) {
	assume(buf_size >= PIPECACHE_KEY_REPR_SIZE);
	hexdigest((const uint8_t*)&key, sizeof(key), buf, buf_size);
}

static SDL_GPUGraphicsPipeline *sdlgpu_pipecache_create_pipeline(const PipelineDescription *restrict pd) {
	auto v_shader = pd->shader_program->stages.vertex;
	auto f_shader = pd->shader_program->stages.fragment;

	log_debug("BEGIN CREATE PIPELINE");

	dump_vertex_input_state(&pd->vertex_array->vertex_input_state);

	SDL_GPUColorAttachmentDescription color_attachment_descriptions[FRAMEBUFFER_MAX_OUTPUTS] = {};

	SDL_GPUGraphicsPipelineCreateInfo pci = {
		.vertexShader = v_shader->shader,
		.fragmentShader = f_shader->shader,
		.vertexInputState = pd->vertex_array->vertex_input_state,
		.primitiveType = sdlgpu_primitive_ts2sdl(pd->primitive),
		.rasterizerState = {
			.cullMode = sdlgpu.st.caps & r_capability_bit(RCAP_CULL_FACE)
				? sdlgpu_cullmode_ts2sdl(sdlgpu.st.cull)
				: SDL_GPU_CULLMODE_NONE,
			.fillMode = SDL_GPU_FILLMODE_FILL,
			.frontFace = pd->front_face,
		},
		.multisampleState = {
			.sampleMask = 0xFFFF,
		},
		.depthStencilState = {
			.depthTestEnable = (bool)(sdlgpu.st.caps & r_capability_bit(RCAP_DEPTH_TEST)),
			.depthWriteEnable = (bool)(sdlgpu.st.caps & r_capability_bit(RCAP_DEPTH_WRITE)),
			.stencilTestEnable = false,
			.writeMask = 0x0,
			.compareOp = sdlgpu.st.caps & r_capability_bit(RCAP_DEPTH_TEST)
				? sdlgpu_cmpop_ts2sdl(sdlgpu.st.depth_func)
				: SDL_GPU_COMPAREOP_ALWAYS,
		},
		.attachmentInfo = {
			.colorAttachmentDescriptions = color_attachment_descriptions,
			.colorAttachmentCount = pd->num_outputs,
			.hasDepthStencilAttachment = pd->depth_format != SDL_GPU_TEXTUREFORMAT_INVALID,
			.depthStencilFormat = pd->depth_format,
		},
	};

	SDL_GPUColorAttachmentBlendState blend = {
		.colorWriteMask = 0xF,
		.blendEnable = sdlgpu.st.blend != BLEND_NONE,
		.alphaBlendOp = sdlgpu_blendop_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_ALPHA_OP)),
		.colorBlendOp = sdlgpu_blendop_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_COLOR_OP)),
		.srcColorBlendFactor = sdlgpu_blendfactor_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_SRC_COLOR)),
		.srcAlphaBlendFactor = sdlgpu_blendfactor_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_SRC_ALPHA)),
		.dstColorBlendFactor = sdlgpu_blendfactor_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_DST_COLOR)),
		.dstAlphaBlendFactor = sdlgpu_blendfactor_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_DST_ALPHA)),
	};

	for(int i = 0; i < pd->num_outputs; ++i) {
		color_attachment_descriptions[i] = (SDL_GPUColorAttachmentDescription) {
			.format = pd->outputs[i].format,
			.blendState = blend,
		};
	}

	auto r = SDL_CreateGPUGraphicsPipeline(sdlgpu.device, &pci);
	log_debug("END CREATE PIPELINE = %p", r);
	return r;
}

static SDL_GPUGraphicsPipeline *sdlgpu_pipecache_create_pipeline_callback(ht_pcache_value_t v) {
	return sdlgpu_pipecache_create_pipeline((PipelineDescription*)v);
}

SDL_GPUGraphicsPipeline *sdlgpu_pipecache_get(PipelineDescription *pd) {
	auto key = sdlgpu_pipecache_construct_key(pd);
	SDL_GPUGraphicsPipeline *result = NULL;

	bool attr_unused created = ht_pcache_try_set(
		&pcache.cache, key, (ht_pcache_value_t)pd, sdlgpu_pipecache_create_pipeline_callback, &result);

#ifdef DEBUG
	if(created) {
		char pipe_repr[PIPECACHE_KEY_REPR_SIZE] = {};
		sdlgpu_pipecache_key_repr(key, sizeof(pipe_repr), pipe_repr);
		log_debug("Created pipeline %s (%u total pipelines cached)", pipe_repr, pcache.cache.num_elements_occupied);
	}
#endif

	return NOT_NULL(result);
}

void sdlgpu_pipecache_deinit(void) {
	sdlgpu_pipecache_wipe();
	ht_pcache_destroy(&pcache.cache);
}

static void sdlgpu_pipecache_remove_matching(bool (*filter)(PipelineCacheKey, sdlgpu_id_t), sdlgpu_id_t id) {
	if(UNLIKELY(pcache.cache.num_elements_occupied < 1)) {
		return;
	}

	PipelineCacheKey keys_to_delete[pcache.cache.num_elements_occupied];
	uint num_keys_to_delete = 0;

	ht_pcache_iter_t iter;
	ht_pcache_iter_begin(&pcache.cache, &iter);
	for(;iter.has_data; ht_pcache_iter_next(&iter)) {
		if(filter(iter.key, id)) {
			keys_to_delete[num_keys_to_delete++] = iter.key;
		}
	}
	ht_pcache_iter_end(&iter);

	for(uint i = 0; i < num_keys_to_delete; ++i) {
		PipelineCacheKey key = keys_to_delete[i];
		SDL_ReleaseGPUGraphicsPipeline(sdlgpu.device, NOT_NULL(ht_pcache_get(&pcache.cache, key, NULL)));
		ht_pcache_unset(&pcache.cache, key);

#ifdef DEBUG
		char pipe_repr[PIPECACHE_KEY_REPR_SIZE] = {};
		sdlgpu_pipecache_key_repr(key, sizeof(pipe_repr), pipe_repr);
		log_debug("Released pipeline %s (%u total pipelines cached)", pipe_repr, pcache.cache.num_elements_occupied);
#endif
	}
}

static bool match_shader_program(PipelineCacheKey key, sdlgpu_id_t id) {
	return key.shader_program == id;
}

static bool match_vertex_array(PipelineCacheKey key, sdlgpu_id_t id) {
	return key.vertex_array == id;
}

void sdlgpu_pipecache_unref_shader_program(sdlgpu_id_t shader_id) {
	sdlgpu_pipecache_remove_matching(match_shader_program, shader_id);
}

void sdlgpu_pipecache_unref_vertex_array(sdlgpu_id_t va_id) {
	sdlgpu_pipecache_remove_matching(match_vertex_array, va_id);
}
