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
#include "../common/sprite_batch.h"
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
#include "util/env.h"

typedef struct TextureUnit {
	LIST_INTERFACE(struct TextureUnit);

	struct {
		GLuint gl_handle;
		Texture *active;
		Texture *pending;
		bool locked;
	} tex2d;
} TextureUnit;

#define TU_INDEX(unit) ((ptrdiff_t)((unit) - R.texunits.array))

static struct {
	struct {
		TextureUnit *array;
		LIST_ANCHOR(TextureUnit) list;
		TextureUnit *active;
		TextureUnit *pending;
		GLint limit;
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

	Color color;
	Color clear_color;
	float clear_depth;
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

static void gl33_init_texunits(void) {
	GLint texunits_available, texunits_capped, texunits_max, texunits_min = 4;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &texunits_available);

	if(texunits_available < texunits_min) {
		log_fatal("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is %i; at least %i is expected.", texunits_available, texunits_min);
	}

	texunits_max = env_get_int("GL33_MAX_NUM_TEXUNITS", 32);

	if(texunits_max == 0) {
		texunits_max = texunits_available;
	} else {
		texunits_max = iclamp(texunits_max, texunits_min, texunits_available);
	}

	texunits_capped = imin(texunits_max, texunits_available);
	R.texunits.limit = env_get_int("GL33_NUM_TEXUNITS", texunits_capped);

	if(R.texunits.limit == 0) {
		R.texunits.limit = texunits_available;
	} else {
		R.texunits.limit = iclamp(R.texunits.limit, texunits_min, texunits_available);
	}

	R.texunits.array = calloc(R.texunits.limit, sizeof(TextureUnit));
	R.texunits.active = R.texunits.array;

	for(int i = 0; i < R.texunits.limit; ++i) {
		TextureUnit *u = R.texunits.array + i;
		alist_append(&R.texunits.list, u);
	}

	log_info("Using %i texturing units (%i available)", R.texunits.limit, texunits_available);
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

	gl33_init_texunits();
	gl33_set_clear_depth(1);
	gl33_set_clear_color(RGBA(0, 0, 0, 0));

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetIntegerv(GL_VIEWPORT, &R.viewport.default_framebuffer.x);

	if(glReadBuffer != NULL) {
		glReadBuffer(GL_BACK);
	}

	R.viewport.active = R.viewport.default_framebuffer;

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

	R.features |= r_feature_bit(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN);

	if(glext.clear_texture) {
		_r_backend.funcs.texture_clear = gl44_texture_clear;
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

	return &fb->viewport;
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
	r_uniform_mat4("r_modelViewMatrix", *_r_matrices.modelview.head);
	r_uniform_mat4("r_projectionMatrix", *_r_matrices.projection.head);
	r_uniform_mat4("r_textureMatrix", *_r_matrices.texture.head);
	r_uniform_vec4_rgba("r_color", &R.color);
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

attr_nonnull(1)
static void gl33_activate_texunit(TextureUnit *unit) {
	assert(unit >= R.texunits.array && unit < R.texunits.array + R.texunits.limit);

	if(R.texunits.active != unit) {
		glActiveTexture(GL_TEXTURE0 + TU_INDEX(unit));
		R.texunits.active = unit;
#ifdef GL33_DEBUG_TEXUNITS
		log_debug("Activated unit %i", (uint)TU_INDEX(unit));
#endif
	}
}

static int gl33_texunit_priority(TextureUnit *u) {
	if(u->tex2d.locked) {
		assert(u->tex2d.pending);
		return 3;
	}

	if(u->tex2d.pending) {
		return 2;
	}

	if(u->tex2d.active) {
		return 1;
	}

	return 0;
}

static int gl33_texunit_priority_callback(List *elem) {
	TextureUnit *u = (TextureUnit*)elem;
	return gl33_texunit_priority(u);
}

attr_unused static void texture_str(Texture *tex, char *buf, size_t bufsize) {
	if(tex == NULL) {
		snprintf(buf, bufsize, "None");
	} else {
		snprintf(buf, bufsize, "\"%s\" (#%i; at %p)", tex->debug_label, tex->gl_handle, (void*)tex);
	}
}

attr_unused static void gl33_dump_texunits(void) {
	log_debug("=== BEGIN DUMP ===");

	for(TextureUnit *u = R.texunits.list.first; u; u = u->next) {
		char buf1[128], buf2[128];
		texture_str(u->tex2d.active, buf1, sizeof(buf1));
		texture_str(u->tex2d.pending, buf2, sizeof(buf2));
		log_debug("[Unit %u | %i] bound: %s; pending: %s", (uint)TU_INDEX(u), gl33_texunit_priority(u), buf1, buf2);
	}

	log_debug("=== END DUMP ===");
}

static void gl33_relocate_texuint(TextureUnit *unit) {
	int prio = gl33_texunit_priority(unit);

	alist_unlink(&R.texunits.list, unit);

	if(prio > 1) {
		alist_insert_at_priority_tail(&R.texunits.list, unit, prio, gl33_texunit_priority_callback);
	} else {
		alist_insert_at_priority_head(&R.texunits.list, unit, prio, gl33_texunit_priority_callback);
	}

#ifdef GL33_DEBUG_TEXUNITS
	// gl33_dump_texunits();
	// log_debug("Relocated unit %u", (uint)TU_INDEX(unit));
#endif
}

attr_nonnull(1)
static void gl33_set_texunit_binding(TextureUnit *unit, Texture *tex, bool lock) {
	assert(!unit->tex2d.locked);

	if(unit->tex2d.pending == tex) {
		return;
	}

	if(unit->tex2d.pending != NULL) {
		assert(unit->tex2d.pending->binding_unit == unit);
		unit->tex2d.pending->binding_unit = NULL;
	}

	unit->tex2d.pending = tex;

	if(tex) {
		tex->binding_unit = unit;
	}

	if(lock) {
		unit->tex2d.locked = true;
	}

	gl33_relocate_texuint(unit);
}

void gl33_sync_texunit(TextureUnit *unit, bool prepare_rendering, bool ensure_active) {
	Texture *tex = unit->tex2d.pending;

#ifdef GL33_DEBUG_TEXUNITS
	if(unit->tex2d.pending != unit->tex2d.active) {
		attr_unused char buf1[128], buf2[128];
		texture_str(unit->tex2d.active, buf1, sizeof(buf1));
		texture_str(unit->tex2d.pending, buf2, sizeof(buf2));
		log_debug("[Unit %u] %s ===> %s", (uint)TU_INDEX(unit), buf1, buf2);
	}
#endif

	if(tex == NULL) {
		if(unit->tex2d.gl_handle != 0) {
			gl33_activate_texunit(unit);
			glBindTexture(GL_TEXTURE_2D, 0);
			unit->tex2d.gl_handle = 0;
			unit->tex2d.active = NULL;
			gl33_relocate_texuint(unit);
		}
	} else if(unit->tex2d.gl_handle != tex->gl_handle) {
		gl33_activate_texunit(unit);
		glBindTexture(GL_TEXTURE_2D, tex->gl_handle);
		unit->tex2d.gl_handle = tex->gl_handle;

		if(unit->tex2d.active == NULL) {
			unit->tex2d.active = tex;
			gl33_relocate_texuint(unit);
		} else {
			unit->tex2d.active = tex;
		}
	} else if(ensure_active) {
		gl33_activate_texunit(unit);
	}

	if(prepare_rendering && unit->tex2d.active != NULL) {
		gl33_texture_prepare(unit->tex2d.active);
		unit->tex2d.locked = false;
	}
}

void gl33_sync_texunits(bool prepare_rendering) {
	for(TextureUnit *u = R.texunits.array; u < R.texunits.array + R.texunits.limit; ++u) {
		gl33_sync_texunit(u, prepare_rendering, false);
	}
}

void gl33_sync_vertex_array(void) {
	R.vertex_array.active = R.vertex_array.pending;

	if(R.vertex_array.active != NULL) {
		gl33_vertex_array_flush_buffers(R.vertex_array.active);
	}

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

	assert(fb->gl_fbo != 0);
	return fb->gl_fbo;
}

void gl33_sync_framebuffer(void) {
	if(fbo_num(R.framebuffer.active) != fbo_num(R.framebuffer.pending)) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_num(R.framebuffer.pending));
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

uint gl33_bind_texture(Texture *texture, bool for_rendering) {
	if(!texture->binding_unit) {
		assert(R.texunits.list.first->tex2d.pending != texture);

		if(
			R.texunits.list.first->tex2d.pending &&
			R.texunits.list.first->tex2d.pending != R.texunits.list.first->tex2d.active
		) {
			log_warn("Ran out of texturing units, expect rendering errors!");
		}

		gl33_set_texunit_binding(R.texunits.list.first, texture, for_rendering);
	} else /* if(for_rendering) */ {
		texture->binding_unit->tex2d.locked |= for_rendering;
		gl33_relocate_texuint(texture->binding_unit);
	}

	assert(texture->binding_unit->tex2d.pending == texture);
	return TU_INDEX(texture->binding_unit);
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

void gl33_texture_deleted(Texture *tex) {
	_r_sprite_batch_texture_deleted(tex);
	gl33_unref_texture_from_samplers(tex);

	for(TextureUnit *unit = R.texunits.array; unit < R.texunits.array + R.texunits.limit; ++unit) {
		bool bump = false;

		if(unit->tex2d.pending == tex) {
			unit->tex2d.pending = NULL;
			unit->tex2d.locked = false;
			bump = true;
		}

		if(unit->tex2d.active == tex) {
			assert(unit->tex2d.gl_handle == tex->gl_handle);
			unit->tex2d.active = NULL;
			unit->tex2d.gl_handle = 0;
			bump = true;
		} else {
			assert(unit->tex2d.gl_handle != tex->gl_handle);
		}

		if(bump) {
			gl33_relocate_texuint(unit);
		}
	}

	if(R.pbo == tex->pbo) {
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
	if(R.vbo.active == vbuf->gl_handle) {
		R.vbo.active = 0;
	}

	if(R.vbo.pending == vbuf->gl_handle) {
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

	if(R.vao.active == varr->gl_handle) {
		R.vao.active = 0;
	}

	if(R.vao.pending == varr->gl_handle) {
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
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
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
	R.color.r = r;
	R.color.g = g;
	R.color.b = b;
	R.color.a = a;
}

static const Color* gl33_color_current(void) {
	return &R.color;
}

static void gl33_vertex_array(VertexArray *varr) {
	R.vertex_array.pending = varr;
	gl33_bind_vao(varr->gl_handle);
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

	if(R.framebuffer.active) {
		gl33_framebuffer_taint(R.framebuffer.active);
	}
}

static void gl33_framebuffer(Framebuffer *fb) {
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

void gl33_set_clear_color(const Color *color) {
	if(memcmp(&R.clear_color, color, sizeof(*color))) {
		memcpy(&R.clear_color, color, sizeof(*color));
		glClearColor(color->r, color->g, color->b, color->a);
	}
}

void gl33_set_clear_depth(float depth) {
	if(R.clear_depth != depth) {
		R.clear_depth = depth;
		glClearDepth(depth);
	}
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

static bool gl33_screenshot(Pixmap *out) {
	IntRect *vp = &R.viewport.default_framebuffer;
	out->width = vp->w;
	out->height = vp->h;
	out->format = PIXMAP_FORMAT_RGB8;
	out->origin = PIXMAP_ORIGIN_BOTTOMLEFT;
	out->data.untyped = pixmap_alloc_buffer_for_copy(out);
	glReadPixels(vp->x, vp->y, vp->w, vp->h, GL_RGB, GL_UNSIGNED_BYTE, out->data.untyped);
	return true;
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
		.shader_language_supported = gl33_shader_language_supported,
		.shader_object_compile = gl33_shader_object_compile,
		.shader_object_destroy = gl33_shader_object_destroy,
		.shader_object_set_debug_label = gl33_shader_object_set_debug_label,
		.shader_object_get_debug_label = gl33_shader_object_get_debug_label,
		.shader_program_link = gl33_shader_program_link,
		.shader_program_destroy = gl33_shader_program_destroy,
		.shader_program_set_debug_label = gl33_shader_program_set_debug_label,
		.shader_program_get_debug_label = gl33_shader_program_get_debug_label,
		.shader = gl33_shader,
		.shader_current = gl33_shader_current,
		.shader_uniform = gl33_shader_uniform,
		.uniform = gl33_uniform,
		.uniform_type = gl33_uniform_type,
		.texture_create = gl33_texture_create,
		.texture_get_size = gl33_texture_get_size,
		.texture_get_params = gl33_texture_get_params,
		.texture_get_debug_label = gl33_texture_get_debug_label,
		.texture_set_debug_label = gl33_texture_set_debug_label,
		.texture_set_filter = gl33_texture_set_filter,
		.texture_set_wrap = gl33_texture_set_wrap,
		.texture_destroy = gl33_texture_destroy,
		.texture_invalidate = gl33_texture_invalidate,
		.texture_fill = gl33_texture_fill,
		.texture_fill_region = gl33_texture_fill_region,
		.texture_clear = gl33_texture_clear,
		.framebuffer_create = gl33_framebuffer_create,
		.framebuffer_destroy = gl33_framebuffer_destroy,
		.framebuffer_attach = gl33_framebuffer_attach,
		.framebuffer_get_debug_label = gl33_framebuffer_get_debug_label,
		.framebuffer_set_debug_label = gl33_framebuffer_set_debug_label,
		.framebuffer_get_attachment = gl33_framebuffer_get_attachment,
		.framebuffer_get_attachment_mipmap = gl33_framebuffer_get_attachment_mipmap,
		.framebuffer_viewport = gl33_framebuffer_viewport,
		.framebuffer_viewport_current = gl33_framebuffer_viewport_current,
		.framebuffer = gl33_framebuffer,
		.framebuffer_current = gl33_framebuffer_current,
		.framebuffer_clear = gl33_framebuffer_clear,
		.vertex_buffer_create = gl33_vertex_buffer_create,
		.vertex_buffer_set_debug_label = gl33_vertex_buffer_set_debug_label,
		.vertex_buffer_get_debug_label = gl33_vertex_buffer_get_debug_label,
		.vertex_buffer_destroy = gl33_vertex_buffer_destroy,
		.vertex_buffer_invalidate = gl33_vertex_buffer_invalidate,
		.vertex_buffer_get_stream = gl33_vertex_buffer_get_stream,
		.vertex_array_destroy = gl33_vertex_array_destroy,
		.vertex_array_create = gl33_vertex_array_create,
		.vertex_array_set_debug_label = gl33_vertex_array_set_debug_label,
		.vertex_array_get_debug_label = gl33_vertex_array_get_debug_label,
		.vertex_array_layout = gl33_vertex_array_layout,
		.vertex_array_attach_buffer = gl33_vertex_array_attach_buffer,
		.vertex_array_get_attachment = gl33_vertex_array_get_attachment,
		.vertex_array = gl33_vertex_array,
		.vertex_array_current = gl33_vertex_array_current,
		.vsync = gl33_vsync,
		.vsync_current = gl33_vsync_current,
		.swap = gl33_swap,
		.screenshot = gl33_screenshot,
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.texture_type_info = gl33_texture_type_info,
			.init_context = gl33_init_context,
		}
	},
};
