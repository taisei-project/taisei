/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "core.h"
#include "../api.h"
#include "../common/matstack.h"
#include "../common/backend.h"
#include "texture.h"
#include "shader_object.h"
#include "shader_program.h"
#include "framebuffer.h"
#include "vertex_buffer.h"
#include "vertex_array.h"
#include "../glcommon/debug.h"
#include "../glcommon/vtable.h"
#include "resource/resource.h"
#include "resource/model.h"
#include "util/glm.h"

typedef struct TextureUnit {
	struct {
		GLuint gl_handle;
		Texture *active;
		Texture *pending;
	} tex2d;
} TextureUnit;

static struct {
	struct {
		TextureUnit indexed[R_MAX_TEXUNITS];
		TextureUnit *active;
		TextureUnit *pending;
	} texunits;

	struct {
		Framebuffer *active;
		Framebuffer *pending;
	} framebuffer;

	struct {
		VertexArray *active;
		VertexArray *pending;
	} vertex_array;

	struct {
		GLuint active;
		GLuint pending;
	} vbo;

	struct {
		GLuint active;
		GLuint pending;
	} vao;

	struct {
		GLuint gl_prog;
		ShaderProgram *active;
		ShaderProgram *pending;
	} progs;

	struct {
		struct {
			BlendMode active;
			BlendMode pending;
		} mode;
		bool enabled;
	} blend;

	struct {
		struct {
			CullFaceMode active;
			CullFaceMode pending;
		} mode;
	} cull_face;

	struct {
		struct {
			DepthTestFunc active;
			DepthTestFunc pending;
		} func;
	} depth_test;

	struct {
		r_capability_bits_t active;
		r_capability_bits_t pending;
	} capabilities;

	struct {
		IntRect active;
		IntRect default_framebuffer;
	} viewport;

	vec4 color;
	vec4 clear_color;
	GLuint pbo;
	r_feature_bits_t features;

	SDL_GLContext *gl_context;

	#ifdef GL33_DRAW_STATS
	struct {
		hrtime_t last_draw;
		hrtime_t draw_time;
		uint draw_calls;
	} stats;
	#endif
} R;

static GLenum prim_to_gl_prim[] = {
	[PRIM_POINTS]         = GL_POINTS,
	[PRIM_LINE_STRIP]     = GL_LINE_STRIP,
	[PRIM_LINE_LOOP]      = GL_LINE_LOOP,
	[PRIM_LINES]          = GL_LINES,
	[PRIM_TRIANGLE_STRIP] = GL_TRIANGLE_STRIP,
	[PRIM_TRIANGLE_FAN]   = GL_TRIANGLE_FAN,
	[PRIM_TRIANGLES]      = GL_TRIANGLES,
};

/*
 * Internal functions
 */

static GLenum blendop_to_gl_blendop(BlendOp op) {
	switch(op) {
		case BLENDOP_ADD:     return GL_FUNC_ADD;
		case BLENDOP_SUB:     return GL_FUNC_SUBTRACT;
		case BLENDOP_REV_SUB: return GL_FUNC_REVERSE_SUBTRACT;
		case BLENDOP_MAX:     return GL_MAX;
		case BLENDOP_MIN:     return GL_MIN;
	}

	UNREACHABLE;
}

static GLenum blendfactor_to_gl_blendfactor(BlendFactor factor) {
	switch(factor) {
		case BLENDFACTOR_DST_ALPHA:     return GL_DST_ALPHA;
		case BLENDFACTOR_INV_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
		case BLENDFACTOR_DST_COLOR:     return GL_DST_COLOR;
		case BLENDFACTOR_INV_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
		case BLENDFACTOR_INV_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
		case BLENDFACTOR_INV_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
		case BLENDFACTOR_ONE:           return GL_ONE;
		case BLENDFACTOR_SRC_ALPHA:     return GL_SRC_ALPHA;
		case BLENDFACTOR_SRC_COLOR:     return GL_SRC_COLOR;
		case BLENDFACTOR_ZERO:          return GL_ZERO;
	}

	UNREACHABLE;
}

static inline GLenum r_cull_to_gl_cull(CullFaceMode mode) {
	switch(mode) {
		case CULL_BACK:
			return GL_BACK;

		case CULL_FRONT:
			return GL_FRONT;

		case CULL_BOTH:
			return GL_FRONT_AND_BACK;

		default: UNREACHABLE;
	}
}

static inline void gl33_stats_pre_draw(void) {
	#ifdef GL33_DRAW_STATS
	R.stats.last_draw = time_get();
	R.stats.draw_calls++;
	#endif
}

static inline void gl33_stats_post_draw(void) {
	#ifdef GL33_DRAW_STATS
	R.stats.draw_time += (time_get() - R.stats.last_draw);
	#endif
}

static inline void gl33_stats_post_frame(void) {
	#ifdef GL33_DRAW_STATS
	log_debug("%.20gs spent in %u draw calls", (double)R.stats.draw_time, R.stats.draw_calls);
	memset(&R.stats, 0, sizeof(R.stats));
	#endif
}

static void gl33_init_context(SDL_Window *window) {
	R.gl_context = SDL_GL_CreateContext(window);

	if(!R.gl_context) {
		log_fatal("Could not create the OpenGL context: %s", SDL_GetError());
	}

	glcommon_load_functions();
	glcommon_check_extensions();

	if(glcommon_debug_requested()) {
		glcommon_debug_enable();
	}

	R.texunits.active = R.texunits.indexed;

	// TODO: move this into generic r_init
	r_enable(RCAP_DEPTH_TEST);
	r_enable(RCAP_DEPTH_WRITE);
	r_enable(RCAP_CULL_FACE);
	r_depth_func(DEPTH_LEQUAL);
	r_cull(CULL_BACK);
	r_blend(BLEND_ALPHA);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetIntegerv(GL_VIEWPORT, &R.viewport.default_framebuffer.x);

	R.viewport.active = R.viewport.default_framebuffer;

	r_clear_color4(0, 0, 0, 1);
	r_clear(CLEAR_ALL);

	if(glext.instanced_arrays) {
		R.features |= r_feature_bit(RFEAT_DRAW_INSTANCED);

		if(glext.base_instance) {
			R.features |= r_feature_bit(RFEAT_DRAW_INSTANCED_BASE_INSTANCE);
		}
	}

	if(glext.depth_texture) {
		R.features |= r_feature_bit(RFEAT_DEPTH_TEXTURE);
	}

	if(glext.draw_buffers) {
		R.features |= r_feature_bit(RFEAT_FRAMEBUFFER_MULTIPLE_OUTPUTS);
	}
}

static void gl33_apply_capability(RendererCapability cap, bool value) {
	switch(cap) {
		case RCAP_DEPTH_TEST:
			(value ? glEnable : glDisable)(GL_DEPTH_TEST);
			break;

		case RCAP_DEPTH_WRITE:
			glDepthMask(value);
			break;

		case RCAP_CULL_FACE:
			(value ? glEnable : glDisable)(GL_CULL_FACE);
			break;

		default: UNREACHABLE;
	}
}

static inline IntRect* get_framebuffer_viewport(Framebuffer *fb) {
	if(fb == NULL) {
		return &R.viewport.default_framebuffer;
	}

	assert(fb->impl != NULL);
	return &fb->impl->viewport;
}

static void gl33_sync_viewport(void) {
	IntRect *vp = get_framebuffer_viewport(R.framebuffer.pending);

	if(memcmp(&R.viewport.active, vp, sizeof(IntRect))) {
		R.viewport.active = *vp;
		glViewport(vp->x, vp->y, vp->w, vp->h);
	}
}

static void gl33_sync_state(void) {
	gl33_sync_capabilities();
	gl33_sync_shader();
	r_uniform("r_modelViewMatrix", 1, _r_matrices.modelview.head);
	r_uniform("r_projectionMatrix", 1, _r_matrices.projection.head);
	r_uniform("r_textureMatrix", 1, _r_matrices.texture.head);
	r_uniform("r_color", 1, R.color);
	gl33_sync_uniforms(R.progs.active);
	gl33_sync_texunits(true);
	gl33_sync_framebuffer();
	gl33_sync_viewport();
	gl33_sync_vertex_array();
	gl33_sync_blend_mode();

	if(R.capabilities.active & r_capability_bit(RCAP_CULL_FACE)) {
		gl33_sync_cull_face_mode();
	}

	if(R.capabilities.active & r_capability_bit(RCAP_DEPTH_TEST)) {
		gl33_sync_depth_test_func();
	}
}

/*
 * Exported functions
 */

void gl33_sync_capabilities(void) {
	if(R.capabilities.active == R.capabilities.pending) {
		return;
	}

	for(RendererCapability cap = 0; cap < NUM_RCAPS; ++cap) {
		r_capability_bits_t flag = r_capability_bit(cap);
		bool pending = R.capabilities.pending & flag;
		bool active = R.capabilities.active & flag;

		if(pending != active) {
			gl33_apply_capability(cap, pending);
		}
	}

	R.capabilities.active = R.capabilities.pending;
}

void gl33_sync_texunit(uint unit, bool prepare_rendering) {
	TextureUnit *u = R.texunits.indexed + unit;
	Texture *tex = u->tex2d.pending;

	if(tex == NULL) {
		if(u->tex2d.gl_handle != 0) {
			gl33_activate_texunit(unit);
			glBindTexture(GL_TEXTURE_2D, 0);
			u->tex2d.gl_handle = 0;
			u->tex2d.active = tex;
		}
	} else if(u->tex2d.gl_handle != tex->impl->gl_handle) {
		gl33_activate_texunit(unit);
		glBindTexture(GL_TEXTURE_2D, tex->impl->gl_handle);
		u->tex2d.gl_handle = tex->impl->gl_handle;
		u->tex2d.active = tex;
	}

	if(prepare_rendering && u->tex2d.active != NULL) {
		gl33_texture_prepare(u->tex2d.active);
	}
}

void gl33_sync_texunits(bool prepare_rendering) {
	for(uint i = 0; i < R_MAX_TEXUNITS; ++i) {
		gl33_sync_texunit(i, prepare_rendering);
	}
}

void gl33_sync_vertex_array(void) {
	R.vertex_array.active = R.vertex_array.pending;
	gl33_sync_vao();
}

void gl33_sync_vbo(void) {
	if(R.vbo.active != R.vbo.pending) {
		R.vbo.active = R.vbo.pending;
		glBindBuffer(GL_ARRAY_BUFFER, R.vbo.active);
	}
}

void gl33_sync_vao(void) {
	if(R.vao.active != R.vao.pending) {
		R.vao.active = R.vao.pending;
		glBindVertexArray(R.vao.active);
	}
}

void gl33_sync_cull_face_mode(void) {
	if(R.cull_face.mode.pending != R.cull_face.mode.active) {
		GLenum glcull = r_cull_to_gl_cull(R.cull_face.mode.pending);
		glCullFace(glcull);
		R.cull_face.mode.active = R.cull_face.mode.pending;
	}
}

void gl33_sync_depth_test_func(void) {
	DepthTestFunc func = R.depth_test.func.pending;

	static GLenum func_to_glfunc[] = {
		[DEPTH_NEVER]     = GL_NEVER,
		[DEPTH_ALWAYS]    = GL_ALWAYS,
		[DEPTH_EQUAL]     = GL_EQUAL,
		[DEPTH_NOTEQUAL]  = GL_NOTEQUAL,
		[DEPTH_LESS]      = GL_LESS,
		[DEPTH_LEQUAL]    = GL_LEQUAL,
		[DEPTH_GREATER]   = GL_GREATER,
		[DEPTH_GEQUAL]    = GL_GEQUAL,
	};

	uint32_t idx = func;
	assert(idx < sizeof(func_to_glfunc)/sizeof(GLenum));

	if(R.depth_test.func.active != func) {
		glDepthFunc(func_to_glfunc[idx]);
		R.depth_test.func.active = func;
	}
}

static inline GLuint fbo_num(Framebuffer *fb) {
	if(fb == NULL) {
		return 0;
	}

	assert(fb->impl != NULL);
	assert(fb->impl->gl_fbo != 0);

	return fb->impl->gl_fbo;
}

void gl33_sync_framebuffer(void) {
	if(fbo_num(R.framebuffer.active) != fbo_num(R.framebuffer.pending)) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_num(R.framebuffer.pending));
		R.framebuffer.active = R.framebuffer.pending;
	}

	if(R.framebuffer.active) {
		gl33_framebuffer_prepare(R.framebuffer.active);
	}
}

void gl33_sync_shader(void) {
	if(R.progs.pending && R.progs.gl_prog != R.progs.pending->gl_handle) {
		glUseProgram(R.progs.pending->gl_handle);
		R.progs.gl_prog = R.progs.pending->gl_handle;
		R.progs.active = R.progs.pending;
	}
}

void gl33_sync_blend_mode(void) {
	BlendMode mode = R.blend.mode.pending;

	if(mode == BLEND_NONE) {
		if(R.blend.enabled) {
			glDisable(GL_BLEND);
			R.blend.enabled = false;
		}

		return;
	}

	if(!R.blend.enabled) {
		R.blend.enabled = true;
		glEnable(GL_BLEND);
	}

	if(mode != R.blend.mode.active) {
		static UnpackedBlendMode umode;
		r_blend_unpack(mode, &umode);
		R.blend.mode.active = mode;

		// TODO: maybe cache the funcs and factors separately,
		// because the blend funcs change a lot less frequently.

		glBlendEquationSeparate(
			blendop_to_gl_blendop(umode.color.op),
			blendop_to_gl_blendop(umode.alpha.op)
		);

		glBlendFuncSeparate(
			blendfactor_to_gl_blendfactor(umode.color.src),
			blendfactor_to_gl_blendfactor(umode.color.dst),
			blendfactor_to_gl_blendfactor(umode.alpha.src),
			blendfactor_to_gl_blendfactor(umode.alpha.dst)
		);
	}
}

void gl33_bind_vbo(GLuint vbo) {
	R.vbo.pending = vbo;
}

void gl33_bind_vao(GLuint vao) {
	R.vao.pending = vao;
}

void gl33_bind_pbo(GLuint pbo) {
	if(!glext.pixel_buffer_object) {
		return;
	}

	if(pbo != R.pbo) {
		// r_flush_sprites();
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
		R.pbo = pbo;
	}
}

GLuint gl33_vbo_current(void) {
	return R.vbo.pending;
}

GLuint gl33_vao_current(void) {
	return R.vao.pending;
}

uint gl33_active_texunit(void) {
	return (uintptr_t)(R.texunits.active - R.texunits.indexed);
}

uint gl33_activate_texunit(uint unit) {
	assert(unit < R_MAX_TEXUNITS);
	uint prev_unit = gl33_active_texunit();

	if(prev_unit != unit) {
		glActiveTexture(GL_TEXTURE0 + unit);
		R.texunits.active = R.texunits.indexed + unit;
	}

	return prev_unit;
}

void gl33_texture_deleted(Texture *tex) {
	for(uint i = 0; i < R_MAX_TEXUNITS; ++i) {
		if(R.texunits.indexed[i].tex2d.pending == tex) {
			R.texunits.indexed[i].tex2d.pending = NULL;
		}

		if(R.texunits.indexed[i].tex2d.active == tex) {
			R.texunits.indexed[i].tex2d.active= NULL;
		}

		if(R.texunits.indexed[i].tex2d.gl_handle == tex->impl->gl_handle) {
			R.texunits.indexed[i].tex2d.gl_handle = 0;
		}
	}

	if(R.pbo == tex->impl->pbo) {
		R.pbo = 0;
	}
}

void gl33_framebuffer_deleted(Framebuffer *fb) {
	if(R.framebuffer.pending == fb) {
		R.framebuffer.pending = NULL;
	}

	if(R.framebuffer.active == fb) {
		R.framebuffer.active = NULL;
	}
}

void gl33_shader_deleted(ShaderProgram *prog) {
	if(R.progs.active == NULL) {
		R.progs.active = NULL;
	}

	if(R.progs.pending == prog) {
		R.progs.pending = NULL;
		r_shader_standard();
	}

	if(R.progs.gl_prog == prog->gl_handle) {
		R.progs.gl_prog = 0;
	}
}

void gl33_vertex_buffer_deleted(VertexBuffer *vbuf) {
	if(R.vbo.active == vbuf->impl->gl_handle) {
		R.vbo.active = 0;
	}

	if(R.vbo.pending == vbuf->impl->gl_handle) {
		R.vbo.pending = 0;
	}
}

void gl33_vertex_array_deleted(VertexArray *varr) {
	if(R.vertex_array.active == varr) {
		R.vertex_array.active = NULL;
	}

	if(R.vertex_array.pending == varr) {
		R.vertex_array.pending = NULL;
		r_vertex_array(r_vertex_array_static_models());
	}

	if(R.vao.active == varr->impl->gl_handle) {
		R.vao.active = 0;
	}

	if(R.vao.pending == varr->impl->gl_handle) {
		R.vao.pending = 0;
	}
}

/*
 * Renderer interface implementation
 */

static void gl33_init(void) {
	glcommon_load_library();

	SDL_GLcontextFlag ctxflags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;

	if(glcommon_debug_requested()) {
		ctxflags |= SDL_GL_CONTEXT_DEBUG_FLAG;
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, ctxflags);
}

static void gl33_post_init(void) {

}

static void gl33_shutdown(void) {
	glcommon_unload_library();
	SDL_GL_DeleteContext(R.gl_context);
}

static SDL_Window* gl33_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	SDL_Window *window = SDL_CreateWindow(title, x, y, w, h, flags | SDL_WINDOW_OPENGL);

	if(R.gl_context) {
		SDL_GL_MakeCurrent(window, R.gl_context);
	} else {
		GLVT.init_context(window);
	}

	return window;
}

static bool gl33_supports(RendererFeature feature) {
	return R.features & r_feature_bit(feature);
}

static void gl33_capabilities(r_capability_bits_t capbits) {
	R.capabilities.pending = capbits;
}

static r_capability_bits_t gl33_capabilities_current(void) {
	return R.capabilities.pending;
}

static void gl33_vsync(VsyncMode mode) {
	int interval = 0, result;

	switch(mode) {
		case VSYNC_NONE:     interval =  0; break;
		case VSYNC_NORMAL:   interval =  1; break;
		case VSYNC_ADAPTIVE: interval = -1; break;
		default:
			log_fatal("Unknown mode 0x%x", mode);
	}

set_interval:
	result = SDL_GL_SetSwapInterval(interval);

	if(result < 0) {
		log_warn("SDL_GL_SetSwapInterval(%i) failed: %s", interval, SDL_GetError());

		// if adaptive vsync failed, try normal vsync
		if(interval < 0) {
			interval = 1;
			goto set_interval;
		}
	}
}

static VsyncMode gl33_vsync_current(void) {
	int interval = SDL_GL_GetSwapInterval();

	if(interval == 0) {
		return VSYNC_NONE;
	}

	return interval > 0 ? VSYNC_NORMAL : VSYNC_ADAPTIVE;
}

static void gl33_color4(float r, float g, float b, float a) {
	glm_vec4_copy((vec4) { r, g, b, a }, R.color);
	// r_uniform_vec4("ctx.color", r, g, b, a);
}

static Color gl33_color_current(void) {
	return rgba(R.color[0], R.color[1], R.color[2], R.color[3]);
}

static void gl33_vertex_array(VertexArray *varr) {
	assert(varr->impl != NULL);

	R.vertex_array.pending = varr;
	gl33_bind_vao(varr->impl->gl_handle);
}

static VertexArray* gl33_vertex_array_current(void) {
	return R.vertex_array.pending;
}

static void gl33_draw(Primitive prim, uint first, uint count, uint32_t *indices, uint instances, uint base_instance) {
	assert(count > 0);
	assert((uint)prim < sizeof(prim_to_gl_prim)/sizeof(GLenum));

	r_flush_sprites();

	GLuint gl_prim = prim_to_gl_prim[prim];
	gl33_sync_state();

	gl33_stats_pre_draw();

	if(indices == NULL) {
		if(instances) {
			if(base_instance) {
				glDrawArraysInstancedBaseInstance(gl_prim, first, count, instances, base_instance);
			} else {
				glDrawArraysInstanced(gl_prim, first, count, instances);
			}
		} else {
			glDrawArrays(gl_prim, first, count);
		}
	} else {
		if(instances) {
			if(base_instance) {
				glDrawElementsInstancedBaseInstance(gl_prim, count, GL_UNSIGNED_INT, indices, instances, base_instance);
			} else {
				glDrawElementsInstanced(gl_prim, count, GL_UNSIGNED_INT, indices, instances);
			}
		} else {
			glDrawElements(gl_prim, count, GL_UNSIGNED_INT, indices);
		}
	}

	gl33_stats_post_draw();
}

static void gl33_texture(uint unit, Texture *tex) {
	assert(unit < R_MAX_TEXUNITS);

	if(tex != NULL) {
		assert(tex->impl != NULL);
		assert(tex->impl->gl_handle != 0);
	}

	R.texunits.indexed[unit].tex2d.pending = tex;
}

static Texture* gl33_texture_current(uint unit) {
	assert(unit < R_MAX_TEXUNITS);
	return R.texunits.indexed[unit].tex2d.pending;
}

static void gl33_framebuffer(Framebuffer *fb) {
	assert(fb == NULL || fb->impl != NULL);
	R.framebuffer.pending = fb;
}

static Framebuffer *gl33_framebuffer_current(void) {
	return R.framebuffer.pending;
}

static void gl33_framebuffer_viewport(Framebuffer *fb, IntRect vp) {
	memcpy(get_framebuffer_viewport(fb), &vp, sizeof(vp));
}

static void gl33_framebuffer_viewport_current(Framebuffer *fb, IntRect *out_rect) {
	*out_rect = *get_framebuffer_viewport(fb);
}

static void gl33_shader(ShaderProgram *prog) {
	assert(prog->gl_handle != 0);

	R.progs.pending = prog;
}

static ShaderProgram *gl33_shader_current(void) {
	return R.progs.pending;
}

static void gl33_clear(ClearBufferFlags flags) {
	GLbitfield mask = 0;

	if(flags & CLEAR_COLOR) {
		mask |= GL_COLOR_BUFFER_BIT;
	}

	if(flags & CLEAR_DEPTH) {
		mask |= GL_DEPTH_BUFFER_BIT;
	}

	r_flush_sprites();
	gl33_sync_framebuffer();
	glClear(mask);
}

static void gl33_clear_color4(float r, float g, float b, float a) {
	vec4 cc = { r, g, b, a };

	if(!memcmp(R.clear_color, cc, sizeof(cc))) {
		memcpy(R.clear_color, cc, sizeof(cc));
		glClearColor(r, g, b, a);
	}
}

static Color gl33_clear_color_current(void) {
	return rgba(R.clear_color[0], R.clear_color[1], R.clear_color[2], R.clear_color[3]);
}

static void gl33_swap(SDL_Window *window) {
	r_flush_sprites();
	gl33_sync_framebuffer();
	SDL_GL_SwapWindow(window);
	gl33_stats_post_frame();
}

static void gl33_blend(BlendMode mode) {
	R.blend.mode.pending = mode;
}

static BlendMode gl33_blend_current(void) {
	return R.blend.mode.pending;
}

static void gl33_cull(CullFaceMode mode) {
	R.cull_face.mode.pending = mode;
}

static CullFaceMode gl33_cull_current(void) {
	return R.cull_face.mode.pending;
}

static void gl33_depth_func(DepthTestFunc func) {
	R.depth_test.func.pending = func;
}

static DepthTestFunc gl33_depth_func_current(void) {
	return R.depth_test.func.pending;
}

static uint8_t* gl33_screenshot(uint *out_width, uint *out_height) {
	uint fbo = 0;
	IntRect *vp = &R.viewport.default_framebuffer;

	if(R.framebuffer.active != NULL) {
		fbo = R.framebuffer.active->impl->gl_fbo;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	uint8_t *pixels = malloc(vp->w * vp->h * 3);
	glReadBuffer(GL_FRONT);
	glReadPixels(vp->x, vp->y, vp->w, vp->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	*out_width = vp->w;
	*out_height = vp->h;

	if(fbo != 0) {
		// FIXME: Maybe we should only ever bind FBOs to GL_DRAW_FRAMEBUFFER?
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	}

	return pixels;
}

RendererBackend _r_backend_gl33 = {
	.name = "gl33",
	.funcs = {
		.init = gl33_init,
		.post_init = gl33_post_init,
		.shutdown = gl33_shutdown,
		.create_window = gl33_create_window,
		.supports = gl33_supports,
		.capabilities = gl33_capabilities,
		.capabilities_current = gl33_capabilities_current,
		.draw = gl33_draw,
		.color4 = gl33_color4,
		.color_current = gl33_color_current,
		.blend = gl33_blend,
		.blend_current = gl33_blend_current,
		.cull = gl33_cull,
		.cull_current = gl33_cull_current,
		.depth_func = gl33_depth_func,
		.depth_func_current = gl33_depth_func_current,
		.shader = gl33_shader,
		.shader_current = gl33_shader_current,
		.shader_uniform = gl33_shader_uniform,
		.uniform = gl33_uniform,
		.uniform_type = gl33_uniform_type,
		.texture_create = gl33_texture_create,
		.texture_get_params = gl33_texture_get_params,
		.texture_set_filter = gl33_texture_set_filter,
		.texture_set_wrap = gl33_texture_set_wrap,
		.texture_destroy = gl33_texture_destroy,
		.texture_invalidate = gl33_texture_invalidate,
		.texture_fill = gl33_texture_fill,
		.texture_fill_region = gl33_texture_fill_region,
		.texture = gl33_texture,
		.texture_current = gl33_texture_current,
		.framebuffer_create = gl33_framebuffer_create,
		.framebuffer_destroy = gl33_framebuffer_destroy,
		.framebuffer_attach = gl33_framebuffer_attach,
		.framebuffer_get_attachment = gl33_framebuffer_get_attachment,
		.framebuffer_get_attachment_mipmap = gl33_framebuffer_get_attachment_mipmap,
		.framebuffer_viewport = gl33_framebuffer_viewport,
		.framebuffer_viewport_current = gl33_framebuffer_viewport_current,
		.framebuffer = gl33_framebuffer,
		.framebuffer_current = gl33_framebuffer_current,
		.vertex_buffer_create = gl33_vertex_buffer_create,
		.vertex_buffer_destroy = gl33_vertex_buffer_destroy,
		.vertex_buffer_invalidate = gl33_vertex_buffer_invalidate,
		.vertex_buffer_write = gl33_vertex_buffer_write,
		.vertex_buffer_append = gl33_vertex_buffer_append,
		.vertex_array_create = gl33_vertex_array_create,
		.vertex_array_destroy = gl33_vertex_array_destroy,
		.vertex_array_layout = gl33_vertex_array_layout,
		.vertex_array_attach_buffer = gl33_vertex_array_attach_buffer,
		.vertex_array_get_attachment = gl33_vertex_array_get_attachment,
		.vertex_array = gl33_vertex_array,
		.vertex_array_current = gl33_vertex_array_current,
		.clear = gl33_clear,
		.clear_color4 = gl33_clear_color4,
		.clear_color_current = gl33_clear_color_current,
		.vsync = gl33_vsync,
		.vsync_current = gl33_vsync_current,
		.swap = gl33_swap,
		.screenshot = gl33_screenshot,
	},
	.res_handlers = {
		.shader_object = &gl33_shader_object_res_handler,
		.shader_program = &gl33_shader_program_res_handler,
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.texture_type_info = gl33_texture_type_info,
			.init_context = gl33_init_context,
		}
	},
};
