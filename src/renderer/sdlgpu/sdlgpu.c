/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "sdlgpu.h"

#include "framebuffer.h"
#include "index_buffer.h"
#include "shader_object.h"
#include "shader_program.h"
#include "texture.h"
#include "vertex_array.h"
#include "vertex_buffer.h"
#include "pipeline_cache.h"

SDLGPUGlobal sdlgpu;

void sdlgpu_renew_swapchain_texture(void) {
	uint w, h;

	if((sdlgpu.frame.swapchain.tex = SDL_GpuAcquireSwapchainTexture(sdlgpu.frame.cbuf, sdlgpu.window, &w, &h))) {
		sdlgpu.frame.swapchain.width = w;
		sdlgpu.frame.swapchain.height = h;
		sdlgpu.frame.swapchain.fmt = SDL_GpuGetSwapchainTextureFormat(sdlgpu.device, sdlgpu.window);
		// log_debug("%u %u", w, h);
	}
}

SDL_GpuTexture *sdlgpu_get_swapchain_texture(void) {
	if(!sdlgpu.frame.swapchain.tex) {
		sdlgpu_renew_swapchain_texture();
	}

	return sdlgpu.frame.swapchain.tex;
}

static void sdlgpu_submit_frame(void);

static void sdlgpu_begin_frame(void) {
	sdlgpu.frame.cbuf = NOT_NULL(SDL_GpuAcquireCommandBuffer(sdlgpu.device));
	sdlgpu_renew_swapchain_texture();
}

static void sdlgpu_submit_frame(void) {
	if(LIKELY(sdlgpu.frame.fence)) {
		SDL_GpuWaitForFences(sdlgpu.device, false, &sdlgpu.frame.fence, 1);
		SDL_GpuReleaseFence(sdlgpu.device, sdlgpu.frame.fence);
	}

	sdlgpu.frame.fence = SDL_GpuSubmitAndAcquireFence(sdlgpu.frame.cbuf);
}

static void sdlgpu_init(void) {
	sdlgpu = (SDLGPUGlobal) {
		.device = SDL_GpuCreateDevice(SDL_GPU_BACKEND_ALL, true, false),
	};

	if(!sdlgpu.device) {
		log_sdl_error(LOG_FATAL, "SDL_GpuCreateDevice");
	}

	sdlgpu.st.blend = BLEND_NONE;

	sdlgpu_pipecache_init();
}

static void sdlgpu_post_init(void) {
}

static void sdlgpu_shutdown(void) {
	sdlgpu_pipecache_deinit();
	SDL_GpuUnclaimWindow(sdlgpu.device, sdlgpu.window);
	SDL_GpuDestroyDevice(sdlgpu.device);
	sdlgpu.device = NULL;
}

static SDL_Window *sdlgpu_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	if(sdlgpu.window) {
		SDL_GpuUnclaimWindow(sdlgpu.device, sdlgpu.window);  // FIXME
	}

	sdlgpu.window = SDL_CreateWindow(title, w, h, flags);

	if(sdlgpu.window && !SDL_GpuClaimWindow(
		sdlgpu.device, sdlgpu.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC
	)) {
		log_sdl_error(LOG_FATAL, "SDL_GpuClaimWindow");
	}

	int pw, ph;
	SDL_GetWindowSizeInPixels(sdlgpu.window, &pw, &ph);
	sdlgpu.frame.swapchain.width = pw;
	sdlgpu.frame.swapchain.height = ph;

	sdlgpu_begin_frame();
	return sdlgpu.window;
}

static void sdlgpu_unclaim_window(SDL_Window *window) {
	sdlgpu_submit_frame();
	SDL_GpuUnclaimWindow(sdlgpu.device, window);
	sdlgpu.window = NULL;
}

static void sdlgpu_vsync(VsyncMode mode) {
	// TODO
}

static void sdlgpu_framebuffer(Framebuffer *fb) {
	sdlgpu.st.framebuffer = fb;
}

static Framebuffer *sdlgpu_framebuffer_current(void) {
	return sdlgpu.st.framebuffer;
}

static void sdlgpu_shader(ShaderProgram *prog) {
	sdlgpu.st.shader = prog;
}

static ShaderProgram *sdlgpu_shader_current(void) {
	return sdlgpu.st.shader;
}

static void sdlgpu_draw(
	VertexArray *varr, Primitive prim, uint firstvert, uint count, uint instances, uint base_instance
) {
	RenderPassOutputs outputs;
	sdlgpu_framebuffer_setup_outputs(sdlgpu.st.framebuffer, &outputs);

	if(!(outputs.num_color_attachments + outputs.have_depth_stencil)) {
		return;
	}

	sdlgpu_vertex_array_flush_buffers(varr);

	PipelineDescription pd = {
		.blend_mode = sdlgpu.st.blend,
		.cap_bits = sdlgpu.st.caps,
		.cull_mode = sdlgpu.st.cull,
		.depth_func = sdlgpu.st.depth_func,
		.num_outputs = outputs.num_color_attachments,
		.primitive = prim,
		.shader_program = sdlgpu.st.shader,
		.vertex_array = varr,
	};

	if(!(pd.cap_bits & r_capability_bit(RCAP_CULL_FACE))) {
		pd.cull_mode = 0;
	}

	if(!(pd.cap_bits & r_capability_bit(RCAP_DEPTH_TEST))) {
		pd.depth_func = DEPTH_ALWAYS;
	}

	{
		assert(outputs.num_color_attachments <= ARRAY_SIZE(pd.outputs));

		uint i;

		for(i = 0; i < outputs.num_color_attachments; ++i) {
			pd.outputs[i].format = outputs.color_formats[i];
		}

		for(; i < ARRAY_SIZE(pd.outputs); ++i) {
			pd.outputs[i].format = PIPECACHE_FMT_MASK;
		}
	}

	auto active_program = sdlgpu.st.shader;
	auto v_shader = active_program->stages.vertex;
	auto f_shader = active_program->stages.fragment;


	SDL_GpuBufferBinding bindings[varr->vertex_input_state.vertexBindingCount];

	for(uint i = 0; i < ARRAY_SIZE(bindings); ++i) {
		uint slot = varr->binding_to_attachment_map[i];
		bindings[i] = (SDL_GpuBufferBinding) {
			.buffer = dynarray_get(&varr->attachments, slot)->cbuf.gpubuf,
		};
	}

	SDL_GpuRenderPass *pass = SDL_GpuBeginRenderPass(
		sdlgpu.frame.cbuf,
		outputs.color, outputs.num_color_attachments,
		outputs.have_depth_stencil ? &outputs.depth_stencil : NULL);

	if(sdlgpu.st.scissor.w && sdlgpu.st.scissor.h) {
		SDL_GpuSetScissor(pass, &(SDL_GpuRect) {
			.w = sdlgpu.st.scissor.w,
			.h = sdlgpu.st.scissor.h,
			.x = sdlgpu.st.scissor.x,
			.y = sdlgpu.st.scissor.y,
		});
	}

	SDL_GpuBindGraphicsPipeline(pass, sdlgpu_pipecache_get(&pd));
	SDL_GpuBindVertexBuffers(pass, 0, bindings, ARRAY_SIZE(bindings));
	SDL_GpuDrawPrimitives(pass, firstvert, count);

	if(v_shader->uniform_buffer.data) {
		SDL_GpuPushVertexUniformData(sdlgpu.frame.cbuf,
			v_shader->uniform_buffer.binding, v_shader->uniform_buffer.data, v_shader->uniform_buffer.size);
	}

	if(f_shader->uniform_buffer.data) {
		SDL_GpuPushFragmentUniformData(sdlgpu.frame.cbuf,
			f_shader->uniform_buffer.binding, f_shader->uniform_buffer.data, f_shader->uniform_buffer.size);
	}

	SDL_GpuEndRenderPass(pass);
}

static void sdlgpu_draw_indexed(
	VertexArray *varr, Primitive prim, uint firstidx, uint count, uint instances, uint base_instance
) {
	UNREACHABLE;
}

static void sdlgpu_swap(SDL_Window *window) {
	assert(window == sdlgpu.window);
	sdlgpu_submit_frame();
	sdlgpu_begin_frame();
}

static r_feature_bits_t sdlgpu_features(void) {
	return
		r_feature_bit(RFEAT_DRAW_INSTANCED) |
		r_feature_bit(RFEAT_DEPTH_TEXTURE) |
		r_feature_bit(RFEAT_FRAMEBUFFER_MULTIPLE_OUTPUTS) |
		// r_feature_bit(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN) |
		r_feature_bit(RFEAT_PARTIAL_MIPMAPS) |
		0;
}

static void sdlgpu_capabilities(r_capability_bits_t capbits) {
	sdlgpu.st.caps = capbits;
}

static r_capability_bits_t sdlgpu_capabilities_current(void) {
	return sdlgpu.st.caps;
}

static void sdlgpu_color4(float r, float g, float b, float a) {
	sdlgpu.st.default_color = (Color) { r, g, b, a };
}

static const Color *sdlgpu_color_current(void) {
	return &sdlgpu.st.default_color;
}

static void sdlgpu_blend(BlendMode mode) {
	sdlgpu.st.blend = mode;
}

static BlendMode sdlgpu_blend_current(void) {
	return sdlgpu.st.blend;
}

static void sdlgpu_depth_func(DepthTestFunc func) {
	sdlgpu.st.depth_func = func;
}

static DepthTestFunc sdlgpu_depth_func_current(void) {
	return sdlgpu.st.depth_func;
}

static void sdlgpu_scissor(IntRect scissor) {
	sdlgpu.st.scissor = scissor;
}

static void sdlgpu_scissor_current(IntRect *scissor) {
	*scissor = sdlgpu.st.scissor;
}

RendererBackend _r_backend_sdlgpu = {
	.name = "sdlgpu",
	.funcs = {
		.scissor = sdlgpu_scissor,
		.scissor_current = sdlgpu_scissor_current,
		.depth_func = sdlgpu_depth_func,
		.depth_func_current = sdlgpu_depth_func_current,
		.blend = sdlgpu_blend,
		.blend_current = sdlgpu_blend_current,
		.color4 = sdlgpu_color4,
		.color_current = sdlgpu_color_current,
		.features = sdlgpu_features,
		.capabilities = sdlgpu_capabilities,
		.capabilities_current = sdlgpu_capabilities_current,
		.create_window = sdlgpu_create_window,
		.unclaim_window = sdlgpu_unclaim_window,
		.draw = sdlgpu_draw,
		.draw_indexed = sdlgpu_draw_indexed,
		.framebuffer = sdlgpu_framebuffer,
		.framebuffer_attach = sdlgpu_framebuffer_attach,
		.framebuffer_clear = sdlgpu_framebuffer_clear,
		.framebuffer_copy = sdlgpu_framebuffer_copy,
		.framebuffer_create = sdlgpu_framebuffer_create,
		.framebuffer_current = sdlgpu_framebuffer_current,
		.framebuffer_destroy = sdlgpu_framebuffer_destroy,
		.framebuffer_get_debug_label = sdlgpu_framebuffer_get_debug_label,
		.framebuffer_get_size = sdlgpu_framebuffer_get_size,
		.framebuffer_outputs = sdlgpu_framebuffer_outputs,
		.framebuffer_set_debug_label = sdlgpu_framebuffer_set_debug_label,
		.framebuffer_viewport = sdlgpu_framebuffer_viewport,
		.framebuffer_viewport_current = sdlgpu_framebuffer_viewport_current,
		.index_buffer_add_indices = sdlgpu_index_buffer_add_indices,
		.index_buffer_create = sdlgpu_index_buffer_create,
		.index_buffer_destroy = sdlgpu_index_buffer_destroy,
		.index_buffer_get_capacity = sdlgpu_index_buffer_get_capacity,
		.index_buffer_get_debug_label = sdlgpu_index_buffer_get_debug_label,
		.index_buffer_get_index_size = sdlgpu_index_buffer_get_index_size,
		.index_buffer_get_offset = sdlgpu_index_buffer_get_offset,
		.index_buffer_invalidate = sdlgpu_index_buffer_invalidate,
		.index_buffer_set_debug_label = sdlgpu_index_buffer_set_debug_label,
		.index_buffer_set_offset = sdlgpu_index_buffer_set_offset,
		.init = sdlgpu_init,
		.post_init = sdlgpu_post_init,
		.shader = sdlgpu_shader,
		.shader_current = sdlgpu_shader_current,
		.shader_language_supported = sdlgpu_shader_language_supported,
		.shader_object_compile = sdlgpu_shader_object_compile,
		.shader_object_destroy = sdlgpu_shader_object_destroy,
		.shader_object_get_debug_label = sdlgpu_shader_object_get_debug_label,
		.shader_object_set_debug_label = sdlgpu_shader_object_set_debug_label,
		.shader_object_transfer = sdlgpu_shader_object_transfer,
		.shader_program_destroy = sdlgpu_shader_program_destroy,
		.shader_program_get_debug_label = sdlgpu_shader_program_get_debug_label,
		.shader_program_link = sdlgpu_shader_program_link,
		.shader_program_set_debug_label = sdlgpu_shader_program_set_debug_label,
		.shader_program_transfer = sdlgpu_shader_program_transfer,
		.shader_uniform = sdlgpu_shader_uniform,
		.uniform = sdlgpu_uniform,
		.uniform_type = sdlgpu_uniform_type,
		.shutdown = sdlgpu_shutdown,
		.vertex_array_attach_index_buffer = sdlgpu_vertex_array_attach_index_buffer,
		.vertex_array_attach_vertex_buffer = sdlgpu_vertex_array_attach_vertex_buffer,
		.vertex_array_create = sdlgpu_vertex_array_create,
		.vertex_array_destroy = sdlgpu_vertex_array_destroy,
		.vertex_array_get_debug_label = sdlgpu_vertex_array_get_debug_label,
		.vertex_array_get_index_attachment = sdlgpu_vertex_array_get_index_attachment,
		.vertex_array_get_vertex_attachment = sdlgpu_vertex_array_get_vertex_attachment,
		.vertex_array_layout = sdlgpu_vertex_array_layout,
		.vertex_array_set_debug_label = sdlgpu_vertex_array_set_debug_label,
		.vertex_buffer_create = sdlgpu_vertex_buffer_create,
		.vertex_buffer_destroy = sdlgpu_vertex_buffer_destroy,
		.vertex_buffer_get_debug_label = sdlgpu_vertex_buffer_get_debug_label,
		.vertex_buffer_get_stream = sdlgpu_vertex_buffer_get_stream,
		.vertex_buffer_invalidate = sdlgpu_vertex_buffer_invalidate,
		.vertex_buffer_set_debug_label = sdlgpu_vertex_buffer_set_debug_label,
		.vsync = sdlgpu_vsync,
		.swap = sdlgpu_swap,
	}
};
