/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "gl33.h"
#include "../api.h"
#include "../common/matstack.h"
#include "../common/backend.h"
#include "../common/sprite_batch.h"
#include "texture.h"
#include "shader_object.h"
#include "shader_program.h"
#include "framebuffer.h"
#include "framebuffer_async_read.h"
#include "common_buffer.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "vertex_array.h"
#include "../glcommon/debug.h"
#include "../glcommon/vtable.h"
#include "resource/resource.h"
#include "resource/model.h"
#include "util/glm.h"
#include "util/env.h"

// #define GL33_DEBUG_TEXUNITS
// #define GL33_DRAW_STATS

typedef struct TextureUnit {
	LIST_INTERFACE(struct TextureUnit);

	Texture *active;
	Texture *pending;
	GLuint gl_handle;
	GLenum bind_target;
	GLenum locked_for_target;
	int old_prio;
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
		GLuint active;
		GLuint pending;
	} buffer_objects[GL33_NUM_BUFFER_BINDINGS];

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
		FloatRect active;
		FloatRect default_framebuffer;
	} viewport;

	struct {
		IntRect pending;
		IntRect active;
		bool active_enabled;
	} scissor;

	Color color;
	Color clear_color;
	float clear_depth;
	r_feature_bits_t features;

	SDL_GLContext *gl_context;
	SDL_Window *window;

#ifndef STATIC_GLES3
	bool fb_sRGB_state;
#endif

	#ifdef GL33_DRAW_STATS
	struct {
		hrtime_t last_draw;
		hrtime_t draw_time;
		uint draw_calls;
		uint texture_rebinds;
	} stats;
	#endif
} R;

/*
 * Internal functions
 */

static void gl33_set_framebuffer_sRGB_write(bool enable) {
#ifndef STATIC_GLES3
	if(glext.version.is_es) {
		return;
	}

	if(enable) {
		if(!R.fb_sRGB_state) {
			glEnable(GL_FRAMEBUFFER_SRGB);
			R.fb_sRGB_state = true;
		}
	} else {
		if(R.fb_sRGB_state) {
			glDisable(GL_FRAMEBUFFER_SRGB);
			R.fb_sRGB_state = false;
		}
	}
#endif
}

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
	log_debug("%.1fµs spent in %u draw calls", R.stats.draw_time / (HRTIME_RESOLUTION / 1000000.0) , R.stats.draw_calls);
	log_debug("%u texture rebinds", R.stats.texture_rebinds);
	memset(&R.stats, 0, sizeof(R.stats));
	#endif
}

static void gl33_init_texunits(void) {
	GLint texunits_available, texunits_capped, texunits_max, texunits_min = 8;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &texunits_available);

	if(texunits_available < texunits_min) {
		log_fatal("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is %i; at least %i is expected.", texunits_available, texunits_min);
	}

	texunits_max = env_get_int("GL33_MAX_NUM_TEXUNITS", 32);

	if(texunits_max == 0) {
		texunits_max = texunits_available;
	} else {
		texunits_max = clamp(texunits_max, texunits_min, texunits_available);
	}

	texunits_capped = min(texunits_max, texunits_available);
	R.texunits.limit = env_get_int("GL33_NUM_TEXUNITS", texunits_capped);

	if(R.texunits.limit == 0) {
		R.texunits.limit = texunits_available;
	} else {
		R.texunits.limit = clamp(R.texunits.limit, texunits_min, texunits_available);
	}

	R.texunits.array = ALLOC_ARRAY(R.texunits.limit, typeof(*R.texunits.array));
	R.texunits.active = R.texunits.array;

	for(int i = 0; i < R.texunits.limit; ++i) {
		TextureUnit *u = R.texunits.array + i;
		alist_append(&R.texunits.list, u);
	}

	log_info("Using %i texturing units (%i available)", R.texunits.limit, texunits_available);
}

static void gl33_get_viewport(FloatRect *vp) {
	IntRect vp_int;
	glGetIntegerv(GL_VIEWPORT, &vp_int.x);
	vp->x = vp_int.x;
	vp->y = vp_int.y;
	vp->w = vp_int.w;
	vp->h = vp_int.h;
}

static void gl33_set_viewport(const FloatRect *vp) {
	glViewport(vp->x, vp->y, vp->w, vp->h);
}

#ifndef STATIC_GLES3
static void gl41_get_viewport(FloatRect *vp) {
	glGetFloati_v(GL_VIEWPORT, 0, &vp->x);
}

static void gl41_set_viewport(const FloatRect *vp) {
	glViewportIndexedfv(0, &vp->x);
}
#endif

static SDL_GLContext gl33_create_context(SDL_Window *window) {
	SDL_GLContext ctx = SDL_GL_CreateContext(window);

	int gl_profile;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &gl_profile);

	if(!ctx && gl_profile != SDL_GL_CONTEXT_PROFILE_ES) {
		log_error("Failed to create OpenGL context: %s", SDL_GetError());
		log_warn("Attempting to create a fallback context");
		SDL_GL_ResetAttributes();
		ctx = SDL_GL_CreateContext(window);
	}

	if(!ctx) {
		log_fatal("Failed to create OpenGL context: %s", SDL_GetError());
	}

	return ctx;
}

static void gl33_init_context(SDL_Window *window) {
	R.gl_context = GLVT.create_context(window);

	glcommon_load_functions();
	glcommon_check_capabilities();

	if(glcommon_debug_requested()) {
		glcommon_debug_enable();
	}

	gl33_init_texunits();
	gl33_set_clear_depth(1);
	gl33_set_clear_color(RGBA(0, 0, 0, 0));

#ifdef STATIC_GLES3
	GLVT.get_viewport = gl33_get_viewport;
	GLVT.set_viewport = gl33_set_viewport;
#else
	if(glext.viewport_array) {
		GLVT.get_viewport = gl41_get_viewport;
		GLVT.set_viewport = gl41_set_viewport;
	} else {
		GLVT.get_viewport = gl33_get_viewport;
		GLVT.set_viewport = gl33_set_viewport;
	}
#endif

	glFrontFace(GL_CW);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	GLVT.get_viewport(&R.viewport.default_framebuffer);

	if(HAVE_GL_FUNC(glReadBuffer)) {
		glReadBuffer(GL_BACK);
	}

#ifndef STATIC_GLES3
	if(glext.seamless_cubemap) {
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}
#endif

	R.viewport.active = R.viewport.default_framebuffer;

	if(glext.instanced_arrays) {
		R.features |= r_feature_bit(RFEAT_DRAW_INSTANCED);
	}

	if(glext.depth_texture) {
		R.features |= r_feature_bit(RFEAT_DEPTH_TEXTURE);
	}

	if(glext.draw_buffers) {
		R.features |= r_feature_bit(RFEAT_FRAMEBUFFER_MULTIPLE_OUTPUTS);
	}

	if(glext.texture_swizzle) {
		R.features |= r_feature_bit(RFEAT_TEXTURE_SWIZZLE);
	}

	if(!glext.version.is_es || GLES_ATLEAST(3, 0)) {
		R.features |= r_feature_bit(RFEAT_PARTIAL_MIPMAPS);
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

static int get_framebuffer_height(Framebuffer *fb) {
	int fb_height = 0;

	if(fb == NULL) {
		SDL_Window *win = SDL_GL_GetCurrentWindow();
		assume(win != NULL);
		SDL_GL_GetDrawableSize(win, NULL, &fb_height);
	} else {
		for(FramebufferAttachment a = FRAMEBUFFER_ATTACH_COLOR0; a < FRAMEBUFFER_MAX_ATTACHMENTS; ++a) {
			if(fb->attachments[a] != NULL) {
				fb_height = fb->attachments[a]->params.height;
				break;
			}
		}
	}

	assert(fb_height > 0);
	return fb_height;
}

static void transform_viewport_origin(Framebuffer *fb, FloatRect *vp) {
	int fb_height = get_framebuffer_height(fb);
	vp->y = fb_height - vp->y - vp->h;
}

static inline FloatRect* get_framebuffer_viewport(Framebuffer *fb) {
	if(fb == NULL) {
		return &R.viewport.default_framebuffer;
	}

	return &fb->viewport;
}

static void gl33_sync_viewport(void) {
	FloatRect *vp = get_framebuffer_viewport(R.framebuffer.pending);

	if(memcmp(&R.viewport.active, vp, sizeof(R.viewport.active))) {
		R.viewport.active = *vp;
		GLVT.set_viewport(vp);
	}
}

void gl33_sync_scissor(void) {
	bool pending_enabled =
		R.scissor.pending.w &&
		R.scissor.pending.h;

	if(!pending_enabled) {
		if(R.scissor.active_enabled) {
			glDisable(GL_SCISSOR_TEST);
			R.scissor.active_enabled = false;
		}

		return;
	}

	if(!R.scissor.active_enabled) {
		glEnable(GL_SCISSOR_TEST);
		R.scissor.active_enabled = true;
	}

	IntRect scissor = R.scissor.pending;
	int fb_height = get_framebuffer_height(R.framebuffer.pending);
	scissor.y = fb_height - scissor.y - scissor.h;

	if(memcmp(&R.scissor.active, &scissor, sizeof(R.scissor.active))) {
		glScissor(scissor.x, scissor.y, scissor.w, scissor.h);
		R.scissor.active = scissor;
	}
}

static IntExtent gl33_get_default_framebuffer_size(void) {
	IntExtent s;
	// TODO: cache this at window creation time and refresh on resize events?
	SDL_GL_GetDrawableSize(R.window, &s.w, &s.h);
	return s;
}

static void gl33_get_output_size(Framebuffer *fb, FramebufferAttachment a, vec2_noalign out) {
	if(a == FRAMEBUFFER_ATTACH_NONE) {
		out[0] = 0;
		out[1] = 0;
		return;
	}

	FramebufferAttachmentQueryResult q = gl33_framebuffer_query_attachment(fb, a);

	if(q.texture == NULL) {
		out[0] = 0;
		out[1] = 0;
		return;
	}

	uint w, h;
	gl33_texture_get_size(q.texture, q.miplevel, &w, &h);
	out[0] = w;
	out[1] = h;
}

static void gl33_sync_magic_uniforms(void) {
	ShaderProgram *shader = NOT_NULL(R.progs.active);
	Framebuffer *fb = R.framebuffer.active;
	Uniform **u = shader->magic_uniforms;

	r_uniform_mat4(u[UMAGIC_MATRIX_MV], *_r_matrices.modelview.head);
	r_uniform_mat4(u[UMAGIC_MATRIX_PROJ], *_r_matrices.projection.head);
	r_uniform_mat4(u[UMAGIC_MATRIX_TEX], *_r_matrices.texture.head);
	r_uniform_vec4_rgba(u[UMAGIC_COLOR], &R.color);
	r_uniform_vec4_vec(u[UMAGIC_VIEWPORT], (float*)&R.viewport.active);

	int num_color_out;

	if(u[UMAGIC_COLOR_OUT_SIZES]) {
		num_color_out = clamp(u[UMAGIC_COLOR_OUT_SIZES]->array_size, 0, FRAMEBUFFER_MAX_OUTPUTS);
	} else {
		num_color_out = 0;
	}

	if(fb) {
		if(num_color_out > 0) {
			vec2_noalign out[num_color_out];

			for(int i = 0; i < num_color_out; ++i) {
				gl33_get_output_size(fb, fb->output_mapping[i], out[i]);
			}

			r_uniform_vec2_array(u[UMAGIC_COLOR_OUT_SIZES], 0, num_color_out, out);
		}

		if(u[UMAGIC_DEPTH_OUT_SIZE]) {
			vec2_noalign out;
			gl33_get_output_size(fb, FRAMEBUFFER_ATTACH_DEPTH, out);
			r_uniform_vec2_vec(u[UMAGIC_DEPTH_OUT_SIZE], out);
		}
	} else {   // default framebuffer
		// TODO: Maybe figure out whether we have the depth buffer?
		// We don't request one for the default framebuffer, so pretend it's not there.
		r_uniform_vec2(u[UMAGIC_DEPTH_OUT_SIZE], 0, 0);

		if(num_color_out > 0) {
			IntExtent fb_size = gl33_get_default_framebuffer_size();
			vec2 out[num_color_out];
			out[0][0] = fb_size.w;
			out[0][1] = fb_size.h;
			memset(out + 1, 0, sizeof(out) - sizeof(out[0]));
			r_uniform_vec2_array(u[UMAGIC_COLOR_OUT_SIZES], 0, num_color_out, out);
		}
	}
}

static void gl33_sync_state(void) {
	gl33_sync_capabilities();
	gl33_sync_framebuffer();
	gl33_sync_shader();
	gl33_sync_viewport();
	gl33_sync_scissor();
	gl33_sync_magic_uniforms();
	gl33_sync_uniforms(R.progs.active);
	gl33_sync_texunits(true);
	gl33_sync_vao();
	gl33_sync_blend_mode();

	if(R.capabilities.active & r_capability_bit(RCAP_CULL_FACE)) {
		gl33_sync_cull_face_mode();
	}

	if(R.capabilities.active & r_capability_bit(RCAP_DEPTH_TEST)) {
		gl33_sync_depth_test_func();
	}

	gl33_set_framebuffer_sRGB_write(R.framebuffer.active != NULL);
}

/*
 * Exported functions
 */

GLenum gl33_prim_to_gl_prim(Primitive prim) {
	static GLenum map[] = {
		[PRIM_POINTS]         = GL_POINTS,
		[PRIM_LINE_STRIP]     = GL_LINE_STRIP,
		[PRIM_LINE_LOOP]      = GL_LINE_LOOP,
		[PRIM_LINES]          = GL_LINES,
		[PRIM_TRIANGLE_STRIP] = GL_TRIANGLE_STRIP,
		[PRIM_TRIANGLES]      = GL_TRIANGLES,
	};

	assert((uint)prim < sizeof(map)/sizeof(*map));
	return map[prim];
}

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
	if(u->locked_for_target) {
		return 3;
	}

	if(u->pending) {
		return 2;
	}

	if(u->active) {
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

attr_unused static void gl33_dump_texunit(TextureUnit *u) {
	char buf1[128], buf2[128];
	texture_str(u->active, buf1, sizeof(buf1));
	texture_str(u->pending, buf2, sizeof(buf2));
	log_debug("[Unit %u | prio=%i | target=0x%x | lock=0x%x] bound: %s; pending: %s",
		(uint)TU_INDEX(u),
		gl33_texunit_priority(u),
		u->bind_target,
		u->locked_for_target,
		buf1,
		buf2
	);
}

attr_unused static void gl33_dump_texunits(void) {
	log_debug("=== BEGIN DUMP ===");

	for(TextureUnit *u = R.texunits.list.first; u; u = u->next) {
		gl33_dump_texunit(u);
	}

	log_debug("=== END DUMP ===");
}

static void gl33_relocate_texuint(TextureUnit *unit) {
	int prio = gl33_texunit_priority(unit);

	if(unit->old_prio == prio) {
		return;
	}

	// TODO: optimize with a heap-based priority queue?

	alist_unlink(&R.texunits.list, unit);

	if(prio > 1) {
		alist_insert_at_priority_tail(&R.texunits.list, unit, prio, gl33_texunit_priority_callback);
	} else {
		alist_insert_at_priority_head(&R.texunits.list, unit, prio, gl33_texunit_priority_callback);
	}

	unit->old_prio = prio;

#ifdef GL33_DEBUG_TEXUNITS
	// gl33_dump_texunits();
	// log_debug("Relocated unit %u", (uint)TU_INDEX(unit));
#endif
}

attr_nonnull(1)
static void gl33_set_texunit_binding(TextureUnit *unit, Texture *tex, GLuint lock_target) {
	assert(!unit->locked_for_target);

	if(tex && lock_target) {
		assert(tex->bind_target == lock_target);
	}

	if(unit->pending == tex) {
		if(tex) {
			tex->binding_unit = unit;
		}

		if(lock_target) {
			unit->locked_for_target = lock_target;
		}

		gl33_relocate_texuint(unit);
		return;
	}

	if(unit->pending != NULL) {
		// assert(unit->tex2d.pending->binding_unit == unit);

		if(unit->pending->binding_unit == unit) {
			// FIXME: should we search through the units for a matching binding,
			// just in case another unit had the same texture bound?
			unit->pending->binding_unit = NULL;
		}
	}

	unit->pending = tex;

	if(tex) {
		tex->binding_unit = unit;
	}

	if(lock_target) {
		unit->locked_for_target = lock_target;
	}

	gl33_relocate_texuint(unit);
}

static void gl33_bind_handle_to_texunit(TextureUnit *u, GLenum target, GLuint handle) {
	/*
	 * For hysterical raisins, OpenGL texturing units can technically have multiple textures bound
	 * to them — one for each target. However, a single texturing unit can not be accessed by two
	 * samplers of a distinct type from a shader, making this "feature" virtually useless.
	 *
	 * Therefore we maintain only 1 kind of texture per unit here.
	 */

#ifdef GL33_DEBUG_TEXUNITS
	if(target != u->bind_target) {
		log_debug("[%u] TYPE CHANGE: 0x%x -> 0x%x", (uint)TU_INDEX(u), u->bind_target, target);
		gl33_dump_texunit(u);
	}
#endif

	if(target != u->bind_target && u->bind_target != 0) {
		// If changing target, clear the old binding.
		glBindTexture(u->bind_target, 0);

#ifdef GL33_DRAW_STATS
		++R.stats.texture_rebinds;
#endif
	}

	glBindTexture(target, handle);

#ifdef GL33_DRAW_STATS
	++R.stats.texture_rebinds;
#endif

	u->bind_target = target;
	u->gl_handle = handle;
}

void gl33_sync_texunit(TextureUnit *unit, bool prepare_rendering, bool ensure_active) {
	Texture *tex = unit->pending;

#ifdef GL33_DEBUG_TEXUNITS
	if(unit->pending != unit->active) {
		attr_unused char buf1[128], buf2[128];
		texture_str(unit->active, buf1, sizeof(buf1));
		texture_str(unit->pending, buf2, sizeof(buf2));
		log_debug("[Unit %u] %s ===> %s (render: %i)", (uint)TU_INDEX(unit), buf1, buf2, prepare_rendering);
	}
#endif

	if(tex == NULL) {
		if(unit->gl_handle != 0) {
			gl33_activate_texunit(unit);
			gl33_bind_handle_to_texunit(unit, unit->bind_target, 0);
			unit->active = NULL;
			gl33_relocate_texuint(unit);
		}
	} else if(unit->gl_handle != tex->gl_handle) {
		gl33_activate_texunit(unit);
		gl33_bind_handle_to_texunit(unit, tex->bind_target, tex->gl_handle);

		bool need_relocate = unit->active == NULL;
		unit->active = tex;

		if(need_relocate) {
			gl33_relocate_texuint(unit);
		}
	} else if(ensure_active) {
		gl33_activate_texunit(unit);
	}

	if(prepare_rendering) {
		if(unit->active != NULL && unit->locked_for_target) {
			gl33_texture_prepare(unit->active);
		}

		unit->locked_for_target = 0;
		gl33_relocate_texuint(unit);
	}
}

void gl33_sync_texunits(bool prepare_rendering) {
// 	gl33_dump_texunits();
	for(TextureUnit *u = R.texunits.array; u < R.texunits.array + R.texunits.limit; ++u) {
		gl33_sync_texunit(u, prepare_rendering, false);
	}
}

void gl33_sync_vao(void) {
	if(R.vao.active != R.vao.pending) {
		R.vao.active = R.vao.pending;
		glBindVertexArray(R.vao.active);
	}
}

GLenum gl33_bindidx_to_glenum(BufferBindingIndex bindidx) {
	if(bindidx == GL33_BUFFER_BINDING_ELEMENT_ARRAY) {
		return GL_ELEMENT_ARRAY_BUFFER;
	}

	static GLenum map[] = {
		[GL33_BUFFER_BINDING_ARRAY] = GL_ARRAY_BUFFER,
		[GL33_BUFFER_BINDING_COPY_WRITE] = GL_COPY_WRITE_BUFFER,
		[GL33_BUFFER_BINDING_PIXEL_UNPACK] = GL_PIXEL_UNPACK_BUFFER,
		[GL33_BUFFER_BINDING_PIXEL_PACK] = GL_PIXEL_PACK_BUFFER,
	};

	static_assert(sizeof(map) == sizeof(GLenum) * GL33_NUM_BUFFER_BINDINGS, "Fix the lookup table");
	assert((uint)bindidx < GL33_NUM_BUFFER_BINDINGS);
	return map[bindidx];
}

void gl33_sync_buffer(BufferBindingIndex bindidx) {
	assert((uint)bindidx < GL33_NUM_BUFFER_BINDINGS);

	if(R.buffer_objects[bindidx].active != R.buffer_objects[bindidx].pending) {
		R.buffer_objects[bindidx].active = R.buffer_objects[bindidx].pending;
		glBindBuffer(gl33_bindidx_to_glenum(bindidx), R.buffer_objects[bindidx].active);
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

static TextureUnit *gl33_get_free_texunit(void) {
	TextureUnit *u = R.texunits.list.first;

	if(
		u->pending &&
		u->pending != u->active
	) {
		log_warn("Ran out of texturing units, expect rendering errors!");
	}

	assert(!u->locked_for_target);
	return u;
}

uint gl33_bind_texture(Texture *texture, GLuint lock_target, int preferred_unit) {
	if(texture == NULL) {
		TextureUnit *unit = NULL;

		if(preferred_unit >= 0) {
			assert(preferred_unit < R.texunits.limit);
			unit = &R.texunits.array[preferred_unit];

			if(
				unit->locked_for_target &&
				(unit->pending != NULL || unit->locked_for_target != lock_target)
			) {
				// someone else already took this unit for rendering
				unit = NULL;
			}
		}

		if(unit == NULL) {
			assert(!glext.issues.avoid_sampler_uniform_updates);
			unit = gl33_get_free_texunit();
		}

		if(unit->locked_for_target) {
			gl33_relocate_texuint(unit);
		} else {
			gl33_set_texunit_binding(unit, NULL, lock_target);
		}

		assert(unit->pending == NULL);
		return TU_INDEX(unit);
	}

	assert(!lock_target || lock_target == texture->bind_target);

	if(glext.issues.avoid_sampler_uniform_updates && preferred_unit >= 0) {
		assert(preferred_unit < R.texunits.limit);
		TextureUnit *u = &R.texunits.array[preferred_unit];

		if(u->pending == texture) {
			if(!u->locked_for_target) {
				u->locked_for_target = lock_target;
			} else {
				assert(u->locked_for_target == lock_target);
			}
		} else {
			gl33_set_texunit_binding(u, texture, lock_target);
		}

		// In this case the texture may be bound to more than one unit.
		// This is fine though, and we just always update binding_unit to the
		// most recent binding.
		texture->binding_unit = u;
	} else if(!texture->binding_unit) {
		gl33_set_texunit_binding(gl33_get_free_texunit(), texture, lock_target);
	} else /* if(for_rendering) */ {
		if(!texture->binding_unit->locked_for_target) {
			texture->binding_unit->locked_for_target = lock_target;
		}

		gl33_relocate_texuint(texture->binding_unit);
	}

	assert(texture->binding_unit->pending == texture);
	return TU_INDEX(texture->binding_unit);
}

void gl33_bind_buffer(BufferBindingIndex bindidx, GLuint gl_handle) {
	R.buffer_objects[bindidx].pending = gl_handle;
}

void gl33_bind_vao(GLuint vao) {
	R.vao.pending = vao;
}

GLuint gl33_buffer_current(BufferBindingIndex bindidx) {
	return R.buffer_objects[bindidx].pending;
}

GLuint gl33_vao_current(void) {
	return R.vao.pending;
}

void gl33_texture_deleted(Texture *tex) {
	_r_sprite_batch_texture_deleted(tex);
	gl33_unref_texture_from_samplers(tex);

	for(TextureUnit *unit = R.texunits.array; unit < R.texunits.array + R.texunits.limit; ++unit) {
		bool bump = false;

		if(unit->pending == tex) {
			unit->pending = NULL;
			unit->locked_for_target = 0;
			bump = true;
		}

		if(unit->active == tex) {
			assert(unit->gl_handle == tex->gl_handle);
			unit->active = NULL;
			unit->gl_handle = 0;
			bump = true;
		} else {
			assert(unit->gl_handle != tex->gl_handle);
		}

		if(bump) {
			gl33_relocate_texuint(unit);
		}
	}

	if(R.buffer_objects[GL33_BUFFER_BINDING_PIXEL_UNPACK].pending == tex->pbo) {
		R.buffer_objects[GL33_BUFFER_BINDING_PIXEL_UNPACK].pending = 0;
	}

	if(R.buffer_objects[GL33_BUFFER_BINDING_PIXEL_UNPACK].active == tex->pbo) {
		R.buffer_objects[GL33_BUFFER_BINDING_PIXEL_UNPACK].active = 0;
	}
}

void gl33_texture_pointer_renamed(Texture *pold, Texture *pnew) {
	_r_sprite_batch_texture_deleted(pold);
	gl33_uniforms_handle_texture_pointer_renamed(pold, pnew);

	for(TextureUnit *unit = R.texunits.array; unit < R.texunits.array + R.texunits.limit; ++unit) {
		if(unit->pending == pold) {
			unit->pending = pnew;
		}

		if(unit->active == pold) {
			unit->active = pnew;
		}
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

void gl33_buffer_deleted(CommonBuffer *cbuf) {
	if(cbuf->bindidx == GL33_BUFFER_BINDING_ELEMENT_ARRAY) {
		// TODO: unreference from VAO
		return;
	}

	if(R.buffer_objects[cbuf->bindidx].active == cbuf->gl_handle) {
		R.buffer_objects[cbuf->bindidx].active = 0;
	}

	if(R.buffer_objects[cbuf->bindidx].pending == cbuf->gl_handle) {
		R.buffer_objects[cbuf->bindidx].pending = 0;
	}
}

void gl33_vertex_buffer_deleted(VertexBuffer *vbuf) {
	if(R.buffer_objects[GL33_BUFFER_BINDING_ARRAY].active == vbuf->cbuf.gl_handle) {
		R.buffer_objects[GL33_BUFFER_BINDING_ARRAY].active = 0;
	}

	if(R.buffer_objects[GL33_BUFFER_BINDING_ARRAY].pending == vbuf->cbuf.gl_handle) {
		R.buffer_objects[GL33_BUFFER_BINDING_ARRAY].pending = 0;
	}
}

void gl33_vertex_array_deleted(VertexArray *varr) {
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
	SDL_GLprofile profile;
	SDL_GLcontextFlag flags = 0;

	if(env_get("TAISEI_GL33_CORE_PROFILE", true)) {
		profile = SDL_GL_CONTEXT_PROFILE_CORE;
	} else {
		profile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
	}

	if(env_get("TAISEI_GL33_FORWARD_COMPATIBLE", true)) {
		flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
	}

	int major = env_get("TAISEI_GL33_VERSION_MAJOR", 3);
	int minor = env_get("TAISEI_GL33_VERSION_MINOR", 3);

	glcommon_setup_attributes(profile, major, minor, flags);
	glcommon_load_library();
}

static void gl33_post_init(void) {

}

static void gl33_shutdown(void) {
	gl33_framebuffer_finalize_read_requests();
	glcommon_unload_library();
	SDL_GL_DeleteContext(R.gl_context);
}

static SDL_Window *gl33_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	SDL_Window *window = SDL_CreateWindow(title, x, y, w, h, flags | SDL_WINDOW_OPENGL);

	if(!window) {
		log_sdl_error(LOG_FATAL, "SDL_CreateWindow");
		return NULL;
	}

	if(R.gl_context) {
		SDL_GL_MakeCurrent(window, R.gl_context);
	} else {
		GLVT.init_context(window);
	}

	R.window = window;
	return window;
}

static r_feature_bits_t gl33_features(void) {
	return R.features;
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

void gl33_begin_draw(VertexArray *varr, void **state) {
	gl33_stats_pre_draw();
	r_flush_sprites();
	GLuint prev_vao = gl33_vao_current();
	gl33_bind_vao(varr->gl_handle);
	gl33_sync_state();
	gl33_vertex_array_flush_buffers(varr);
	*state = (void*)(uintptr_t)prev_vao;
}

void gl33_end_draw(void *state) {
	if(R.framebuffer.active) {
		gl33_framebuffer_taint(R.framebuffer.active);
	}

	gl33_bind_vao((uintptr_t)state);
	gl33_stats_post_draw();
}

static void gl33_draw(VertexArray *varr, Primitive prim, uint firstvert, uint count, uint instances, uint base_instance) {
	assert(count > 0);
	assert(base_instance == 0);
	GLuint gl_prim = gl33_prim_to_gl_prim(prim);

	void *state;
	gl33_begin_draw(varr, &state);

	if(instances) {
		glDrawArraysInstanced(gl_prim, firstvert, count, instances);
	} else {
		glDrawArrays(gl_prim, firstvert, count);
	}

	gl33_end_draw(state);
}

static void gl33_draw_indexed(VertexArray *varr, Primitive prim, uint firstidx, uint count, uint instances, uint base_instance) {
	assert(count > 0);
	assert(base_instance == 0);

	IndexBuffer *ibuf = NOT_NULL(varr->index_attachment);
	GLuint gl_prim = gl33_prim_to_gl_prim(prim);

	void *state;
	gl33_begin_draw(varr, &state);

	uintptr_t iofs = firstidx * ibuf->index_size;

	if(instances) {
		glDrawElementsInstanced(gl_prim, count, ibuf->index_datatype, (void*)iofs, instances);
	} else {
		glDrawElements(gl_prim, count, ibuf->index_datatype, (void*)iofs);
	}

	gl33_end_draw(state);
}

static void gl33_framebuffer(Framebuffer *fb) {
	R.framebuffer.pending = fb;
}

static Framebuffer *gl33_framebuffer_current(void) {
	return R.framebuffer.pending;
}

static void gl33_framebuffer_viewport(Framebuffer *fb, FloatRect vp) {
	transform_viewport_origin(fb, &vp);
	memcpy(get_framebuffer_viewport(fb), &vp, sizeof(vp));
}

static void gl33_framebuffer_viewport_current(Framebuffer *fb, FloatRect *out_rect) {
	*out_rect = *get_framebuffer_viewport(fb);
	transform_viewport_origin(fb, out_rect);
}

static IntExtent gl33_framebuffer_get_size(Framebuffer *fb) {
	IntExtent fb_size;

	if(fb == NULL) {
		fb_size = gl33_get_default_framebuffer_size();
	} else {
		fb_size = gl33_framebuffer_get_effective_size(fb);
	}

	return fb_size;
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

	Framebuffer *prev_fb = r_framebuffer_current();
	r_framebuffer(NULL);
	gl33_sync_framebuffer();
#ifndef __EMSCRIPTEN__
	SDL_GL_SwapWindow(window);
#endif
	r_framebuffer(prev_fb);

	gl33_framebuffer_process_read_requests();
	gl33_stats_post_frame();

	// We can't rely on viewport being preserved across frames,
	// so force the next frame to set one on the first draw call.
	// The viewport might get updated externally when e.g. going
	// fullscreen, and we can't catch that in the resize event.
	memset(&R.viewport.active, 0, sizeof(R.viewport.active));
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
	FloatRect *vp = &R.viewport.default_framebuffer;
	out->width = vp->w;
	out->height = vp->h;
	out->format = PIXMAP_FORMAT_RGB8;
	out->origin = PIXMAP_ORIGIN_BOTTOMLEFT;
	out->data.untyped = pixmap_alloc_buffer_for_copy(out, &out->data_size);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadPixels(vp->x, vp->y, vp->w, vp->h, GL_RGB, GL_UNSIGNED_BYTE, out->data.untyped);
	return true;
}

static void gl33_scissor(IntRect scissor) {
	R.scissor.pending = scissor;
}

static void gl33_scissor_current(IntRect *scissor) {
	*scissor = R.scissor.pending;
}

RendererBackend _r_backend_gl33 = {
	.name = "gl33",
	.funcs = {
		.init = gl33_init,
		.post_init = gl33_post_init,
		.shutdown = gl33_shutdown,
		.create_window = gl33_create_window,
		.features = gl33_features,
		.capabilities = gl33_capabilities,
		.capabilities_current = gl33_capabilities_current,
		.draw = gl33_draw,
		.draw_indexed = gl33_draw_indexed,
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
		.shader_object_transfer = gl33_shader_object_transfer,
		.shader_program_link = gl33_shader_program_link,
		.shader_program_destroy = gl33_shader_program_destroy,
		.shader_program_set_debug_label = gl33_shader_program_set_debug_label,
		.shader_program_get_debug_label = gl33_shader_program_get_debug_label,
		.shader_program_transfer = gl33_shader_program_transfer,
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
		.texture_type_query = gl33_texture_type_query,
		.texture_dump = gl33_texture_dump,
		.texture_transfer = gl33_texture_transfer,
		.framebuffer_create = gl33_framebuffer_create,
		.framebuffer_destroy = gl33_framebuffer_destroy,
		.framebuffer_attach = gl33_framebuffer_attach,
		.framebuffer_get_debug_label = gl33_framebuffer_get_debug_label,
		.framebuffer_set_debug_label = gl33_framebuffer_set_debug_label,
		.framebuffer_query_attachment = gl33_framebuffer_query_attachment,
		.framebuffer_outputs = gl33_framebuffer_outputs,
		.framebuffer_viewport = gl33_framebuffer_viewport,
		.framebuffer_viewport_current = gl33_framebuffer_viewport_current,
		.framebuffer = gl33_framebuffer,
		.framebuffer_current = gl33_framebuffer_current,
		.framebuffer_clear = gl33_framebuffer_clear,
		.framebuffer_copy = gl33_framebuffer_copy,
		.framebuffer_get_size = gl33_framebuffer_get_size,
		.framebuffer_read_async = gl33_framebuffer_read_async,
		.vertex_buffer_create = gl33_vertex_buffer_create,
		.vertex_buffer_set_debug_label = gl33_vertex_buffer_set_debug_label,
		.vertex_buffer_get_debug_label = gl33_vertex_buffer_get_debug_label,
		.vertex_buffer_destroy = gl33_vertex_buffer_destroy,
		.vertex_buffer_invalidate = gl33_vertex_buffer_invalidate,
		.vertex_buffer_get_stream = gl33_vertex_buffer_get_stream,
		.index_buffer_create = gl33_index_buffer_create,
		.index_buffer_get_capacity = gl33_index_buffer_get_capacity,
		.index_buffer_get_index_size = gl33_index_buffer_get_index_size,
		.index_buffer_get_debug_label = gl33_index_buffer_get_debug_label,
		.index_buffer_set_debug_label = gl33_index_buffer_set_debug_label,
		.index_buffer_set_offset = gl33_index_buffer_set_offset,
		.index_buffer_get_offset = gl33_index_buffer_get_offset,
		.index_buffer_add_indices = gl33_index_buffer_add_indices,
		.index_buffer_invalidate = gl33_index_buffer_invalidate,
		.index_buffer_destroy = gl33_index_buffer_destroy,
		.vertex_array_destroy = gl33_vertex_array_destroy,
		.vertex_array_create = gl33_vertex_array_create,
		.vertex_array_set_debug_label = gl33_vertex_array_set_debug_label,
		.vertex_array_get_debug_label = gl33_vertex_array_get_debug_label,
		.vertex_array_layout = gl33_vertex_array_layout,
		.vertex_array_attach_vertex_buffer = gl33_vertex_array_attach_vertex_buffer,
		.vertex_array_get_vertex_attachment = gl33_vertex_array_get_vertex_attachment,
		.vertex_array_attach_index_buffer = gl33_vertex_array_attach_index_buffer,
		.vertex_array_get_index_attachment = gl33_vertex_array_get_index_attachment,
		.scissor = gl33_scissor,
		.scissor_current = gl33_scissor_current,
		.vsync = gl33_vsync,
		.vsync_current = gl33_vsync_current,
		.swap = gl33_swap,
		.screenshot = gl33_screenshot,
	},
	.custom = &(GLBackendData) {
		.vtable = {
			.create_context = gl33_create_context,
			.init_context = gl33_init_context,
		}
	},
};
