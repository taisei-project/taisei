/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "sdlgpu.h"

#include "framebuffer.h"
#include "framebuffer_async_read.h"
#include "index_buffer.h"
#include "shader_object.h"
#include "shader_program.h"
#include "texture.h"
#include "vertex_array.h"
#include "vertex_buffer.h"
#include "pipeline_cache.h"

#include "../common/matstack.h"
#include "../common/shaderlib/lang_dxbc.h"

#include "memory/scratch.h"
#include "util/env.h"

SDLGPUGlobal sdlgpu;

static bool renderpass_outputs_compatible(const RenderPassOutputs *current, const RenderPassOutputs *new) {
	if(current->num_color_attachments != new->num_color_attachments) {
		return false;
	}

	if(current->depth_format != new->depth_format) {
		return false;
	}

	if(new->depth_format != SDL_GPU_TEXTUREFORMAT_INVALID) {
		if(
			current->depth_stencil.texture != new->depth_stencil.texture ||
			new->depth_stencil.load_op == SDL_GPU_LOADOP_CLEAR ||
			new->depth_stencil.stencil_load_op == SDL_GPU_LOADOP_CLEAR
		) {
			return false;
		}
	}

	for(uint i = 0; i < current->num_color_attachments; ++i) {
		if(
			new->color[i].load_op == SDL_GPU_LOADOP_CLEAR ||
			current->color[i].texture != new->color[i].texture ||
			current->color[i].mip_level != new->color[i].mip_level ||
			current->color[i].layer_or_depth_plane != new->color[i].layer_or_depth_plane
		) {
			return false;
		}
	}

	return true;
}

static SDL_GPURenderPass *sdlgpu_begin_render_pass(RenderPassOutputs *outputs) {
	// assert(outputs->num_color_attachments || outputs->depth_format != SDL_GPU_TEXTUREFORMAT_INVALID);

	return SDL_BeginGPURenderPass(sdlgpu.frame.cbuf,
		outputs->color, outputs->num_color_attachments,
		(outputs->depth_format != SDL_GPU_TEXTUREFORMAT_INVALID) ? &outputs->depth_stencil : NULL);
}

SDL_GPURenderPass *sdlgpu_begin_or_resume_render_pass(RenderPassOutputs *outputs) {
	if(sdlgpu.copy_pass.for_cbuf[CBUF_DRAW]) {
		SDL_EndGPUCopyPass(sdlgpu.copy_pass.for_cbuf[CBUF_DRAW]);
		sdlgpu.copy_pass.for_cbuf[CBUF_DRAW] = NULL;
	}

	if(sdlgpu.render_pass.pass) {
		if(!renderpass_outputs_compatible(&sdlgpu.render_pass.outputs, outputs)) {
			SDL_EndGPURenderPass(sdlgpu.render_pass.pass);
			sdlgpu.render_pass.pass = NULL;
		}
	}

	if(!sdlgpu.render_pass.pass) {
		sdlgpu.render_pass.pass = sdlgpu_begin_render_pass(outputs);
		sdlgpu.render_pass.outputs = *outputs;
	}

	return sdlgpu.render_pass.pass;
}

SDL_GPUCopyPass *sdlgpu_begin_or_resume_copy_pass(CommandBufferID cbuf_id) {
	SDL_GPUCopyPass **pass = &sdlgpu.copy_pass.for_cbuf[cbuf_id];

	if(!*pass) {
		if(sdlgpu.render_pass.pass && cbuf_id == CBUF_DRAW) {
			SDL_EndGPURenderPass(sdlgpu.render_pass.pass);
			sdlgpu.render_pass.pass = NULL;
		}

		*pass = SDL_BeginGPUCopyPass(sdlgpu.frame.command_buffers[cbuf_id]);
	}

	return *pass;
}

void sdlgpu_stop_current_pass(CommandBufferID cbuf_id) {
	if(cbuf_id == CBUF_DRAW) {
		if(sdlgpu.render_pass.pass) {
			SDL_EndGPURenderPass(sdlgpu.render_pass.pass);
			sdlgpu.render_pass.pass = NULL;
		}
	}

	if(sdlgpu.copy_pass.for_cbuf[cbuf_id]) {
		SDL_EndGPUCopyPass(sdlgpu.copy_pass.for_cbuf[cbuf_id]);
		sdlgpu.copy_pass.for_cbuf[cbuf_id] = NULL;
	}
}

static void sdlgpu_update_faux_backbuffer(void) {
	if(sdlgpu.frame.faux_backbuffer.fmt == SDL_GPU_TEXTUREFORMAT_INVALID) {
		// faux backbuffer is unused
		return;
	}

	if(
		(
			sdlgpu.frame.faux_backbuffer.fmt != sdlgpu.frame.swapchain.fmt ||
			sdlgpu.frame.faux_backbuffer.width != sdlgpu.frame.swapchain.width ||
			sdlgpu.frame.faux_backbuffer.height != sdlgpu.frame.swapchain.height
		) && sdlgpu.frame.faux_backbuffer.tex != NULL
	) {
		SDL_ReleaseGPUTexture(sdlgpu.device, sdlgpu.frame.faux_backbuffer.tex);
		sdlgpu.frame.faux_backbuffer.tex = NULL;
	}

	if(sdlgpu.frame.faux_backbuffer.tex == NULL) {
		SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

		if(sdlgpu.frame.swapchain.fmt == SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM) {
			// If we're going to download this texture, we'll need to blit to fix the channel order
			usage |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
		}

		sdlgpu.frame.faux_backbuffer.fmt = sdlgpu.frame.swapchain.fmt;
		sdlgpu.frame.faux_backbuffer.width = sdlgpu.frame.swapchain.width;
		sdlgpu.frame.faux_backbuffer.height = sdlgpu.frame.swapchain.height;
		sdlgpu.frame.faux_backbuffer.tex = SDL_CreateGPUTexture(sdlgpu.device, &(SDL_GPUTextureCreateInfo) {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = sdlgpu.frame.faux_backbuffer.fmt,
			.width = sdlgpu.frame.faux_backbuffer.width,
			.height = sdlgpu.frame.faux_backbuffer.height,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.usage = usage,
		});

		if(UNLIKELY(!sdlgpu.frame.faux_backbuffer.tex)) {
			log_sdl_error(LOG_ERROR, "SDL_CreateGPUTexture");
		} else {
			SDL_SetGPUTextureName(
				sdlgpu.device, sdlgpu.frame.faux_backbuffer.tex, "Faux backbuffer");
			log_debug("CREATE FAUX BACKBUFFER");
		}
	}
}

void sdlgpu_renew_swapchain_texture(void) {
	if(sdlgpu.frame.swapchain.present_mode != sdlgpu.frame.swapchain.next_present_mode) {
		if(SDL_SetGPUSwapchainParameters(
			sdlgpu.device, sdlgpu.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, sdlgpu.frame.swapchain.next_present_mode)
		) {
			sdlgpu.frame.swapchain.present_mode = sdlgpu.frame.swapchain.next_present_mode;
		} else {
			sdlgpu.frame.swapchain.next_present_mode = sdlgpu.frame.swapchain.present_mode;
		}

		switch(sdlgpu.frame.swapchain.present_mode) {
			case SDL_GPU_PRESENTMODE_VSYNC:
				log_debug("Present mode changed to VSYNC");
				break;
			case SDL_GPU_PRESENTMODE_IMMEDIATE:
				log_debug("Present mode changed to IMMEDIATE");
				break;
			case SDL_GPU_PRESENTMODE_MAILBOX:
				log_debug("Present mode changed to MAILBOX");
				break;
		}
	}

	if(SDL_WaitAndAcquireGPUSwapchainTexture(sdlgpu.frame.cbuf, sdlgpu.window,
		&sdlgpu.frame.swapchain.tex, &sdlgpu.frame.swapchain.width, &sdlgpu.frame.swapchain.height)
	) {
		if(sdlgpu.frame.swapchain.tex) {
			sdlgpu.frame.swapchain.fmt = SDL_GetGPUSwapchainTextureFormat(sdlgpu.device, sdlgpu.window);

			sdlgpu_cmdbuf_debug(sdlgpu.frame.cbuf, "Acquired swapchan %ux%u, format=%u",
				sdlgpu.frame.swapchain.width, sdlgpu.frame.swapchain.height, sdlgpu.frame.swapchain.fmt);

			sdlgpu_update_faux_backbuffer();
		} else {
			sdlgpu_cmdbuf_debug(sdlgpu.frame.cbuf, "Swapchain acquisition failed");
		}
	} else {
		auto scratch = acquire_scratch_arena();
		auto error = marena_strdup(scratch, SDL_GetError());
		log_sdl_error(LOG_ERROR, "SDL_WaitAndAcquireGPUSwapchainTexture");
		sdlgpu_cmdbuf_debug(sdlgpu.frame.cbuf, "Swapchain acquisition failed: %s", error);
		release_scratch_arena(scratch);
	}
}

static const char *command_buffer_name(CommandBufferID id) {
	switch(id) {
		case CBUF_UPLOAD: return "UPLOAD";
		case CBUF_DRAW: return "DRAW";
		default: return "UNKNOWN";
	}
}

void sdlgpu_cmdbuf_debug(SDL_GPUCommandBuffer *cbuf, const char *format, ...) {
	if(!sdlgpu.debug) {
		return;
	}

	static uint debug_event_id = 0;

	strbuf_printf(&sdlgpu.debug_buf, "(%u) ", debug_event_id);
	++debug_event_id;

	va_list args;
	va_start(args, format);
	strbuf_vprintf(&sdlgpu.debug_buf, format, args);
	va_end(args);

	SDL_InsertGPUDebugLabel(cbuf, sdlgpu.debug_buf.start);
	strbuf_clear(&sdlgpu.debug_buf);
}

static SDL_GPUCommandBuffer *sdlgpu_acquire_command_buffer(CommandBufferID id) {
	SDL_GPUCommandBuffer *cbuf = NOT_NULL(SDL_AcquireGPUCommandBuffer(sdlgpu.device));

	if(UNLIKELY(!cbuf)) {
		log_sdl_error(LOG_FATAL, "SDL_AcquireGPUCommandBuffer");
		UNREACHABLE;
	}

	sdlgpu_cmdbuf_debug(cbuf, "Created %s command buffer", command_buffer_name(id));
	return cbuf;
}

static void finish_all_passes(void) {
	for(uint i = 0; i < ARRAY_SIZE(sdlgpu.copy_pass.for_cbuf); ++i) {
		if(sdlgpu.copy_pass.for_cbuf[i]) {
			SDL_EndGPUCopyPass(sdlgpu.copy_pass.for_cbuf[i]);
			sdlgpu.copy_pass.for_cbuf[i] = NULL;
		}
	}

	if(sdlgpu.render_pass.pass) {
		SDL_EndGPURenderPass(sdlgpu.render_pass.pass);
		sdlgpu.render_pass.pass = NULL;
	}
}

static void ensure_command_buffer(CommandBufferID id) {
	if(!sdlgpu.frame.command_buffers[id]) {
		sdlgpu.frame.command_buffers[id] = sdlgpu_acquire_command_buffer(id);
	}
}

static void ensure_command_buffers(void) {
	for(uint i = 0; i < ARRAY_SIZE(sdlgpu.frame.command_buffers); ++i) {
		ensure_command_buffer(i);
	}
}

static void sdlgpu_begin_frame(void) {
	finish_all_passes();

	for(uint i = 0; i < ARRAY_SIZE(sdlgpu.frame.command_buffers); ++i) {
		ensure_command_buffer(i);
		sdlgpu_cmdbuf_debug(sdlgpu.frame.command_buffers[i], "Begin frame %u", sdlgpu.frame.counter);
	}

	if(!sdlgpu.frame.swapchain.tex) {
		sdlgpu_renew_swapchain_texture();
	}
}

static void sdlgpu_submit_frame(void) {
	if(sdlgpu.frame.faux_backbuffer.tex && sdlgpu.frame.swapchain.tex) {
		assert(sdlgpu.frame.swapchain.width == sdlgpu.frame.faux_backbuffer.width);
		assert(sdlgpu.frame.swapchain.height == sdlgpu.frame.faux_backbuffer.height);
		assert(sdlgpu.frame.swapchain.fmt == sdlgpu.frame.faux_backbuffer.fmt);
		SDL_CopyGPUTextureToTexture(sdlgpu_begin_or_resume_copy_pass(CBUF_DRAW),
			&(SDL_GPUTextureLocation) { .texture = sdlgpu.frame.faux_backbuffer.tex, },
			&(SDL_GPUTextureLocation) { .texture = sdlgpu.frame.swapchain.tex,
		}, sdlgpu.frame.swapchain.width, sdlgpu.frame.swapchain.height, 1, false);
	}

	finish_all_passes();

	for(uint i = 0; i < ARRAY_SIZE(sdlgpu.frame.command_buffers); ++i) {
		sdlgpu_cmdbuf_debug(sdlgpu.frame.command_buffers[i], "Submit frame %u", sdlgpu.frame.counter);
	}

	if(UNLIKELY(!SDL_SubmitGPUCommandBuffer(sdlgpu.frame.upload_cbuf))) {
		log_sdl_error(LOG_ERROR, "SDL_SubmitGPUCommandBuffer");
	}

	if(UNLIKELY(!SDL_SubmitGPUCommandBuffer(sdlgpu.frame.cbuf))) {
		log_sdl_error(LOG_ERROR, "SDL_SubmitGPUCommandBuffer");
	}

	sdlgpu.frame.upload_cbuf = NULL;
	sdlgpu.frame.cbuf = NULL;
	sdlgpu.frame.swapchain.tex = NULL;

	++sdlgpu.frame.counter;
	ensure_command_buffers();
}

static Texture *sdlgpu_create_null_texture(TextureClass cls) {
	uint num_layers = cls == TEXTURE_CLASS_CUBEMAP ? 6 : 1;

	auto null_tex = sdlgpu_texture_create(&(TextureParams) {
		.class = cls,
		.type = TEX_TYPE_R_8,
		.filter.min = TEX_FILTER_NEAREST,
		.filter.mag = TEX_FILTER_NEAREST,
		.wrap.s = TEX_WRAP_REPEAT,
		.wrap.t = TEX_WRAP_REPEAT,
		.anisotropy = 1,
		.flags = 0,
		.width = 1,
		.height = 1,
		.layers = num_layers,
		.mipmap_mode = TEX_MIPMAP_MANUAL,
		.mipmaps = 1,
	});

	switch(cls) {
		case TEXTURE_CLASS_2D:
			sdlgpu_texture_set_debug_label(null_tex, "<NULL TEXTURE 2D>");
			break;
		case TEXTURE_CLASS_CUBEMAP:
			sdlgpu_texture_set_debug_label(null_tex, "<NULL TEXTURE CUBE>");
			break;
		default: UNREACHABLE;
	}

	alignas(max_align_t) PixelR8 pixel = { .r = 0 };
	Pixmap content = {
		.format = PIXMAP_FORMAT_R8,
		.data.r8 = &pixel,
		.data_size = sizeof(pixel),
		.width = 1,
		.height = 1,
		.origin = PIXMAP_ORIGIN_TOPLEFT,
	};

	for(uint i = 0; i < num_layers; ++i) {
		sdlgpu_texture_fill(null_tex, 0, i, &content);
	}

	return null_tex;
}

static void sdlgpu_init(void) {
	SDL_GPUShaderFormat shader_formats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL;

	if(dxbc_init_compiler()) {
		shader_formats |= SDL_GPU_SHADERFORMAT_DXBC;
	}

	SDL_PropertiesID props = SDL_CreateProperties();

	sdlgpu.debug = env_get("TAISEI_SDLGPU_DEBUG", false);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, sdlgpu.debug);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN,
		env_get("TAISEI_SDLGPU_PREFER_LOWPOWER", false));
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN,
		(bool)(shader_formats & SDL_GPU_SHADERFORMAT_SPIRV));
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXBC_BOOLEAN,
		(bool)(shader_formats & SDL_GPU_SHADERFORMAT_DXBC));
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN,
		(bool)(shader_formats & SDL_GPU_SHADERFORMAT_DXIL));
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN,
		(bool)(shader_formats & SDL_GPU_SHADERFORMAT_MSL));
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_METALLIB_BOOLEAN,
		(bool)(shader_formats & SDL_GPU_SHADERFORMAT_METALLIB));

	sdlgpu = (SDLGPUGlobal) {
		.device = SDL_CreateGPUDeviceWithProperties(props),
	};

	if(!sdlgpu.device) {
		log_sdl_error(LOG_FATAL, "SDL_CreateGPUDeviceWithProperties");
	}

	if(!(SDL_GetGPUShaderFormats(sdlgpu.device) & SDL_GPU_SHADERFORMAT_DXBC)) {
		dxbc_shutdown_compiler();
	}

	sdlgpu.st.blend = BLEND_NONE;
	sdlgpu.frame.swapchain.present_mode = SDL_GPU_PRESENTMODE_VSYNC;
	sdlgpu.frame.swapchain.next_present_mode = SDL_GPU_PRESENTMODE_VSYNC;
	ensure_command_buffers();
	sdlgpu_pipecache_init();
	sdlgpu_texture_init_type_remap_table();

	for(uint i = 0; i < ARRAY_SIZE(sdlgpu.null_textures); ++i) {
		sdlgpu.null_textures[i] = sdlgpu_create_null_texture(i);
	}

	if(env_get("TAISEI_SDLGPU_FAUX_BACKBUFFER", true)) {
		sdlgpu.frame.faux_backbuffer.fmt = (SDL_GPUTextureFormat)-1;
	} else {
		sdlgpu.frame.faux_backbuffer.fmt = SDL_GPU_TEXTUREFORMAT_INVALID;
	}
}

static void sdlgpu_post_init(void) {
}

static void sdlgpu_shutdown(void) {
	sdlgpu_framebuffer_finalize_read_requests();
	finish_all_passes();

	for(uint i = 0; i < ARRAY_SIZE(sdlgpu.frame.command_buffers); ++i) {
		SDL_CancelGPUCommandBuffer(sdlgpu.frame.command_buffers[i]);
	}

	assert(!sdlgpu.render_pass.pass);
	assert(!sdlgpu.copy_pass.for_cbuf[CBUF_DRAW]);
	assert(!sdlgpu.copy_pass.for_cbuf[CBUF_UPLOAD]);

	for(uint i = 0; i < ARRAY_SIZE(sdlgpu.null_textures); ++i) {
		sdlgpu_texture_destroy(sdlgpu.null_textures[i]);
	}

	SDL_ReleaseGPUTexture(sdlgpu.device, sdlgpu.frame.faux_backbuffer.tex);

	sdlgpu_pipecache_deinit();
	SDL_ReleaseWindowFromGPUDevice(sdlgpu.device, sdlgpu.window);
	SDL_DestroyGPUDevice(sdlgpu.device);
	sdlgpu.device = NULL;
	strbuf_free(&sdlgpu.debug_buf);
	dxbc_shutdown_compiler();
}

static SDL_Window *sdlgpu_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	assert(!sdlgpu.window);

	sdlgpu.window = SDL_CreateWindow(title, w, h, flags);

	if(sdlgpu.window && !SDL_ClaimWindowForGPUDevice(sdlgpu.device, sdlgpu.window)) {
		log_sdl_error(LOG_FATAL, "SDL_ClaimGPUWindow");
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
	SDL_ReleaseWindowFromGPUDevice(sdlgpu.device, window);
	sdlgpu.window = NULL;
}

static void sdlgpu_vsync(VsyncMode mode) {
	SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;

	switch(mode) {
		case VSYNC_NONE:
			present_mode = SDL_GPU_PRESENTMODE_MAILBOX;

			if(!SDL_WindowSupportsGPUPresentMode(sdlgpu.device, sdlgpu.window, present_mode)) {
				present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;

				if(!SDL_WindowSupportsGPUPresentMode(sdlgpu.device, sdlgpu.window, present_mode)) {
					present_mode = SDL_GPU_PRESENTMODE_VSYNC;
				}
			}

		case VSYNC_NORMAL:
		case VSYNC_ADAPTIVE:
			break;
	}

	sdlgpu.frame.swapchain.next_present_mode = present_mode;
}

static VsyncMode sdlgpu_vsync_current(void) {
	switch(sdlgpu.frame.swapchain.present_mode) {
		case SDL_GPU_PRESENTMODE_VSYNC:
			return VSYNC_NORMAL;
		case SDL_GPU_PRESENTMODE_IMMEDIATE:
		case SDL_GPU_PRESENTMODE_MAILBOX:
			return VSYNC_NONE;
		default: UNREACHABLE;
	}
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

static void fill_sampler_bindings(ShaderObject *shader, SDL_GPUTextureSamplerBinding bindings[]) {
	for(uint i = 0; i < shader->num_sampler_bindings; ++i) {
		Texture *tex = shader->sampler_bindings[i];
		assert(tex != NULL);

		if(tex->is_virgin) {
			log_error("%p (%s) is a virgin texture! Used as sampler %i for shader %s",
				tex, tex->debug_label, i, shader->debug_label);
			// assert(0);
		}

		sdlgpu_texture_update_sampler(tex);
		sdlgpu_texture_prepare(tex);

		bindings[i] = (SDL_GPUTextureSamplerBinding) {
			.texture = tex->gpu_texture,
			.sampler = tex->sampler,
		};
	};
}

static void sdlgpu_set_magic_uniforms(
	ShaderObject *shobj, const FloatRect *viewport, mat4 projection_mat
) {
	sdlgpu_shader_object_uniform_set_data(
		shobj, shobj->magic_unfiroms[UMAGIC_MATRIX_MV], 0, 1,
		_r_matrices.modelview.head);
	sdlgpu_shader_object_uniform_set_data(
		shobj, shobj->magic_unfiroms[UMAGIC_MATRIX_PROJ], 0, 1,
		projection_mat);
	sdlgpu_shader_object_uniform_set_data(
		shobj, shobj->magic_unfiroms[UMAGIC_MATRIX_TEX], 0, 1,
		_r_matrices.texture.head);
	sdlgpu_shader_object_uniform_set_data(
		shobj, shobj->magic_unfiroms[UMAGIC_COLOR], 0, 1,
		&sdlgpu.st.default_color);
	sdlgpu_shader_object_uniform_set_data(
		shobj, shobj->magic_unfiroms[UMAGIC_VIEWPORT], 0, 1,
		viewport);

	// TODO UMAGIC_COLOR_OUT_SIZES, UMAGIC_DEPTH_OUT_SIZE
	// We really should have only 1 output size
	// Attaching mismatched textures to the same framebuffer is not supported
}

static void sdlgpu_draw_generic(
	VertexArray *varr,
	Primitive prim,
	uint first,
	uint count,
	uint instances,
	bool is_indexed
) {
	if(sdlgpu.st.prev_framebuffer && sdlgpu.st.prev_framebuffer != sdlgpu.st.framebuffer) {
		sdlgpu_framebuffer_flush(sdlgpu.st.prev_framebuffer, ~0);
		sdlgpu.st.prev_framebuffer = NULL;
	}

	r_flush_sprites();

	RenderPassOutputs outputs;
	sdlgpu_framebuffer_setup_outputs(sdlgpu.st.framebuffer, &outputs);

	if(instances < 1) {
		instances = 1;  // FIXME stop passing this incorrectly
	}

	bool have_depth = outputs.depth_format != SDL_GPU_TEXTUREFORMAT_INVALID;

	if(!outputs.num_color_attachments && !have_depth) {
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
		.depth_format = outputs.depth_format,
		.front_face = sdlgpu.st.framebuffer ? SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE : SDL_GPU_FRONTFACE_CLOCKWISE,
	};

	if(!(pd.cap_bits & r_capability_bit(RCAP_CULL_FACE))) {
		pd.cull_mode = 0;
	}

	if(!have_depth || !(pd.cap_bits & r_capability_bit(RCAP_DEPTH_TEST))) {
		pd.depth_func = DEPTH_ALWAYS;
		pd.cap_bits &= ~(r_capability_bit(RCAP_DEPTH_TEST) | r_capability_bit(RCAP_DEPTH_WRITE));
	}

	assert(outputs.num_color_attachments <= ARRAY_SIZE(pd.outputs));

	for(uint i = 0; i < outputs.num_color_attachments; ++i) {
		pd.outputs[i].format = outputs.color_formats[i];
	}

	auto pipe = sdlgpu_pipecache_get(&pd);

	if(UNLIKELY(!pipe)) {
		return;
	}

	auto active_program = sdlgpu.st.shader;
	auto v_shader = active_program->stages.vertex;
	auto f_shader = active_program->stages.fragment;

	SDL_GPUBufferBinding vbuf_bindings[varr->vertex_input_state.num_vertex_buffers];

	for(uint i = 0; i < ARRAY_SIZE(vbuf_bindings); ++i) {
		uint slot = varr->binding_to_attachment_map[i];
		vbuf_bindings[i] = (SDL_GPUBufferBinding) {
			.buffer = dynarray_get(&varr->attachments, slot)->cbuf.gpubuf,
		};
	}

	// Set up samplers before beginning the render pass, because this may trigger mipmap regen.
	// XXX: force at least 1 element to shut UBSan up
	SDL_GPUTextureSamplerBinding vert_samplers[v_shader->num_sampler_bindings ?: 1];
	SDL_GPUTextureSamplerBinding frag_samplers[f_shader->num_sampler_bindings ?: 1];

	fill_sampler_bindings(v_shader, vert_samplers);
	fill_sampler_bindings(f_shader, frag_samplers);

	SDL_GPURenderPass *pass = sdlgpu_begin_or_resume_render_pass(&outputs);

	if(sdlgpu.st.scissor.w && sdlgpu.st.scissor.h) {
		SDL_SetGPUScissor(pass, &(SDL_Rect) {
			.w = sdlgpu.st.scissor.w,
			.h = sdlgpu.st.scissor.h,
			.x = sdlgpu.st.scissor.x,
			.y = sdlgpu.st.scissor.y,
		});
	}

	FloatRect *vp = sdlgpu_framebuffer_viewport_pointer(sdlgpu.st.framebuffer);
	SDL_SetGPUViewport(pass, &(SDL_GPUViewport) {
		.x = vp->x,
		.y = vp->y,
		.h = vp->h,
		.w = vp->w,
		.min_depth = 0 ,
		.max_depth = 1,
	});

	mat4 proj;
	mat4 clip_conversion = {
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.5f, 0.0f },
		{ 0.0f, 0.0f, 0.5f, 1.0f },
	};

	if(sdlgpu.st.framebuffer != NULL) {
		clip_conversion[1][1] = -1.0f;
	}

	glm_mat4_mul(clip_conversion, *_r_matrices.projection.head, proj);

	sdlgpu_set_magic_uniforms(v_shader, vp, proj);
	sdlgpu_set_magic_uniforms(f_shader, vp, proj);

	SDL_BindGPUGraphicsPipeline(pass, pipe);
	SDL_BindGPUVertexBuffers(pass, 0, vbuf_bindings, ARRAY_SIZE(vbuf_bindings));

	if(v_shader->uniform_buffer.data) {
		SDL_PushGPUVertexUniformData(sdlgpu.frame.cbuf,
			v_shader->uniform_buffer.binding, v_shader->uniform_buffer.data, v_shader->uniform_buffer.size);
	}

	if(f_shader->uniform_buffer.data) {
		SDL_PushGPUFragmentUniformData(sdlgpu.frame.cbuf,
			f_shader->uniform_buffer.binding, f_shader->uniform_buffer.data, f_shader->uniform_buffer.size);
	}

	if(v_shader->num_sampler_bindings) {
		SDL_BindGPUVertexSamplers(pass, 0, vert_samplers, v_shader->num_sampler_bindings);
	}

	if(f_shader->num_sampler_bindings) {
		SDL_BindGPUFragmentSamplers(pass, 0, frag_samplers, f_shader->num_sampler_bindings);
	}

	if(is_indexed) {
		SDL_GPUIndexElementSize isize;

		switch(NOT_NULL(varr->index_attachment)->index_size) {
			case 2:
				isize = SDL_GPU_INDEXELEMENTSIZE_16BIT;
				break;

			case 4:
				isize = SDL_GPU_INDEXELEMENTSIZE_32BIT;
				break;

			default:
				UNREACHABLE;
		}

		SDL_BindGPUIndexBuffer(pass, &(SDL_GPUBufferBinding) {
			.buffer = varr->index_attachment->cbuf.gpubuf,
			.offset = 0,
		}, isize);

		SDL_DrawGPUIndexedPrimitives(pass, count, instances, first, 0, 0);
	} else {
		SDL_DrawGPUPrimitives(pass, count, instances, first, 0);
	}

	if(sdlgpu.st.framebuffer) {
		sdlgpu_framebuffer_taint(sdlgpu.st.framebuffer);
	}

	sdlgpu.st.prev_framebuffer = sdlgpu.st.framebuffer;
}

static void sdlgpu_draw(
	VertexArray *varr, Primitive prim, uint firstvert, uint count, uint instances, uint base_instance
) {
	assert(base_instance == 0);
	sdlgpu_draw_generic(varr, prim, firstvert, count, instances, false);
}

static void sdlgpu_draw_indexed(
	VertexArray *varr, Primitive prim, uint firstidx, uint count, uint instances, uint base_instance
) {
	assert(base_instance == 0);
	sdlgpu_draw_generic(varr, prim, firstidx, count, instances, true);
}

static void sdlgpu_swap(SDL_Window *window) {
	assert(window == sdlgpu.window);
	sdlgpu_submit_frame();
	sdlgpu_framebuffer_process_read_requests();
}

static r_feature_bits_t sdlgpu_features(void) {
	return
		r_feature_bit(RFEAT_DRAW_INSTANCED) |
		r_feature_bit(RFEAT_DEPTH_TEXTURE) |
		r_feature_bit(RFEAT_FRAMEBUFFER_MULTIPLE_OUTPUTS) |
		r_feature_bit(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN) |
		r_feature_bit(RFEAT_PARTIAL_MIPMAPS) |
		(sdlgpu.frame.faux_backbuffer.tex ?
			r_feature_bit(RFEAT_DEFAULT_FRAMEBUFFER_READBACK) : 0) |
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

static void sdlgpu_cull(CullFaceMode mode) {
	sdlgpu.st.cull = mode;
}

static CullFaceMode sdlgpu_cull_current(void) {
	return sdlgpu.st.cull;
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
		.framebuffer_query_attachment = sdlgpu_framebuffer_query_attachment,
		.framebuffer_read_async = sdlgpu_framebuffer_read_async,
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
		.texture_create = sdlgpu_texture_create,
		.texture_get_size = sdlgpu_texture_get_size,
		.texture_get_params = sdlgpu_texture_get_params,
		.texture_get_debug_label = sdlgpu_texture_get_debug_label,
		.texture_set_debug_label = sdlgpu_texture_set_debug_label,
		.texture_set_filter = sdlgpu_texture_set_filter,
		.texture_set_wrap = sdlgpu_texture_set_wrap,
		.texture_destroy = sdlgpu_texture_destroy,
		.texture_invalidate = sdlgpu_texture_invalidate,
		.texture_fill = sdlgpu_texture_fill,
		.texture_fill_region = sdlgpu_texture_fill_region,
		.texture_clear = sdlgpu_texture_clear,
		.texture_type_query = sdlgpu_texture_type_query,
		.texture_dump = sdlgpu_texture_dump,
		.texture_transfer = sdlgpu_texture_transfer,
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
		.vsync_current = sdlgpu_vsync_current,
		.begin_frame = sdlgpu_begin_frame,
		.swap = sdlgpu_swap,
		.cull = sdlgpu_cull,
		.cull_current = sdlgpu_cull_current,
	}
};
