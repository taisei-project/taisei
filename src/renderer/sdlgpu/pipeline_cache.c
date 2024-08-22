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
#define HT_VALUE_TYPE                  SDL_GpuGraphicsPipeline *
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
		SDL_GpuReleaseGraphicsPipeline(sdlgpu.device, iter.value);
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

static SDL_GpuGraphicsPipeline *sdlgpu_pipecache_create_pipeline(const PipelineDescription *restrict pd) {
	auto v_shader = pd->shader_program->stages.vertex;
	auto f_shader = pd->shader_program->stages.fragment;

	SDL_GpuColorAttachmentDescription color_attachment_descriptions[FRAMEBUFFER_MAX_OUTPUTS] = {};

	SDL_GpuGraphicsPipelineCreateInfo pci = {
		.vertexShader = v_shader->shader,
		.fragmentShader = f_shader->shader,
		.vertexInputState = pd->vertex_array->vertex_input_state,
		.primitiveType = sdlgpu_primitive_ts2sdl(pd->primitive),
		.rasterizerState = {
			.cullMode = sdlgpu.st.caps & r_capability_bit(RCAP_CULL_FACE)
				? sdlgpu_cullmode_ts2sdl(sdlgpu.st.cull)
				: SDL_GPU_CULLMODE_NONE,
			.fillMode = SDL_GPU_FILLMODE_FILL,
			.frontFace = SDL_GPU_FRONTFACE_CLOCKWISE,
		},
		.multisampleState = {
			.sampleMask = 0xFFFF,
		},
		.depthStencilState = {
			.depthTestEnable = sdlgpu.st.caps & r_capability_bit(RCAP_DEPTH_TEST),
			.depthWriteEnable = sdlgpu.st.caps & r_capability_bit(RCAP_DEPTH_WRITE),
			.stencilTestEnable = false,
			.writeMask = 0x0,
			.compareOp = sdlgpu.st.caps & r_capability_bit(RCAP_DEPTH_TEST)
				? SDL_GPU_COMPAREOP_ALWAYS
				: sdlgpu_cmpop_ts2sdl(sdlgpu.st.depth_func),
		},
		.attachmentInfo = {
			.colorAttachmentDescriptions = color_attachment_descriptions,
			.colorAttachmentCount = pd->num_outputs,
		},
	};

	SDL_GpuColorAttachmentBlendState blend = {
		.colorWriteMask = 0xF,
		.blendEnable = sdlgpu.st.blend != BLEND_NONE,
		.alphaBlendOp = sdlgpu_blendop_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_ALPHA_OP)),
		.colorBlendOp = sdlgpu_blendop_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_COLOR_OP)),
		.srcColorBlendFactor = sdlgpu_blendop_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_SRC_COLOR)),
		.srcAlphaBlendFactor = sdlgpu_blendop_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_SRC_ALPHA)),
		.dstColorBlendFactor = sdlgpu_blendop_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_DST_COLOR)),
		.dstAlphaBlendFactor = sdlgpu_blendop_ts2sdl(r_blend_component(sdlgpu.st.blend, BLENDCOMP_DST_ALPHA)),
	};

	for(int i = 0; i < pd->num_outputs; ++i) {
		color_attachment_descriptions[i] = (SDL_GpuColorAttachmentDescription) {
			.format = pd->outputs[i].format,
			.blendState = blend,
		};
	}

	return SDL_GpuCreateGraphicsPipeline(sdlgpu.device, &pci);
}

static SDL_GpuGraphicsPipeline *sdlgpu_pipecache_create_pipeline_callback(ht_pcache_value_t v) {
	return sdlgpu_pipecache_create_pipeline((PipelineDescription*)v);
}

SDL_GpuGraphicsPipeline *sdlgpu_pipecache_get(PipelineDescription *pd) {
	auto key = sdlgpu_pipecache_construct_key(pd);
	SDL_GpuGraphicsPipeline *result = NULL;

	bool attr_unused created = ht_pcache_try_set(
		&pcache.cache, key, (ht_pcache_value_t)pd, sdlgpu_pipecache_create_pipeline_callback, &result);

#ifdef DEBUG
	if(created) {
		char pipe_repr[sizeof(PipelineCacheKey) * 2 + 1] = {};
		hexdigest((void*)&key, sizeof(key), pipe_repr, sizeof(pipe_repr));
		log_debug("Created pipeline %s (%u total pipelines cached)", pipe_repr, pcache.cache.num_elements_occupied);
	}
#endif

	return NOT_NULL(result);
}

void sdlgpu_pipecache_deinit(void) {
	sdlgpu_pipecache_wipe();
	ht_pcache_destroy(&pcache.cache);
}
