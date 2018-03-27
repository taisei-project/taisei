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
#include "texture.h"
#include "shader_program.h"
#include "render_target.h"
#include "vertex_buffer.h"
#include "resource/resource.h"
#include "resource/model.h"
#include "glm.h"

#include <stdalign.h>

typedef struct TextureUnit {
	struct {
		GLuint gl_handle;
		Texture *active;
		Texture *pending;
	} tex2d;
} TextureUnit;

static struct {
	struct {
		union {
			struct {
				MatrixStack modelview;
				MatrixStack projection;
				MatrixStack texture;
			};

			MatrixStack indexed[3];
		};

		MatrixStack *active;
	} mstacks;

	struct {
		TextureUnit indexed[GL33_MAX_TEXUNITS];
		TextureUnit *active;
		TextureUnit *pending;
	} texunits;

	struct {
		RenderTarget *active;
		RenderTarget *pending;
	} render_target;

	struct {
		GLenum gl_vbo;
		GLenum gl_vao;
		VertexBuffer *active;
		VertexBuffer *pending;
	} vertex_buffer;

	struct {
		GLuint gl_prog;
		ShaderProgram *active;
		ShaderProgram *pending;
		ShaderProgram *std;
		ShaderProgram *std_notex;
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
		uint32_t active;
		uint32_t pending;
	} capabilities;

	vec4 color;
	vec4 clear_color;
	IntRect viewport;
	GLuint pbo;

	SDL_GLContext *gl_context;

#ifdef GL33_DRAW_STATS
	struct {
		hrtime_t last_draw;
		hrtime_t draw_time;
		uint draw_calls;
	} stats;
#endif
} R;

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

#ifdef DEBUG_GL
static void APIENTRY gl33_debug(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *arg
) {
	char *strsrc = "unknown";
	char *strtype = "unknown";
	char *strsev = "unknown";
	LogLevel lvl0 = LOG_DEBUG;
	LogLevel lvl1 = LOG_DEBUG;
	LogLevel lvl;

	switch(source) {
		case GL_DEBUG_SOURCE_API:               strsrc  = "api";                           break;
		case GL_DEBUG_SOURCE_APPLICATION:       strsrc  = "app";                           break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:   strsrc  = "shaderc";                       break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:       strsrc  = "extern";                        break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     strsrc  = "winsys";                        break;
		case GL_DEBUG_SOURCE_OTHER:             strsrc  = "other";                         break;
	}

	switch(type) {
		case GL_DEBUG_TYPE_ERROR:               strtype = "error";       lvl0 = LOG_FATAL; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: strtype = "deprecated";  lvl0 = LOG_WARN;  break;
		case GL_DEBUG_TYPE_PORTABILITY:         strtype = "portability"; lvl0 = LOG_WARN;  break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  strtype = "undefined";   lvl0 = LOG_WARN;  break;
		case GL_DEBUG_TYPE_PERFORMANCE:         strtype = "performance"; lvl0 = LOG_WARN;  break;
		case GL_DEBUG_TYPE_OTHER:               strtype = "other";                         break;
	}

	switch(severity) {
		case GL_DEBUG_SEVERITY_LOW:             strsev  = "low";         lvl1 = LOG_INFO;  break;
		case GL_DEBUG_SEVERITY_MEDIUM:          strsev  = "medium";      lvl1 = LOG_WARN;  break;
		case GL_DEBUG_SEVERITY_HIGH:            strsev  = "high";        lvl1 = LOG_FATAL; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:    strsev  = "notify";                        break;
	}

	lvl = lvl1 > lvl0 ? lvl1 : lvl0;

	if(source == GL_DEBUG_SOURCE_SHADER_COMPILER && type == GL_DEBUG_TYPE_ERROR) {
		lvl = LOG_WARN;
	}

	log_custom(lvl, "[%s, %s, %s, id:%i] %s", strsrc, strtype, strsev, id, message);
}

static void gl33_debug_enable(void) {
	GLuint unused = 0;
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(gl33_debug, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, true);
	log_info("Enabled OpenGL debugging");
}
#endif

void gl33_debug_object_label(GLenum identifier, GLuint name, const char *label) {
#ifdef DEBUG_GL
	if(glext.KHR_debug) {
		glObjectLabel(identifier, name, strlen(label), label);
	}
#endif
}

static void gl33_init_context(SDL_Window *window) {
	R.gl_context = SDL_GL_CreateContext(window);

	if(!R.gl_context) {
		log_fatal("Could not create the OpenGL context: %s", SDL_GetError());
	}

	load_gl_functions();
	check_gl_extensions();

#ifdef DEBUG_GL
	if(glext.debug_output) {
		gl33_debug_enable();
	} else {
		log_warn("OpenGL debugging is not supported by the implementation");
	}
#endif

	matstack_reset(&R.mstacks.texture);
	matstack_reset(&R.mstacks.modelview);
	matstack_reset(&R.mstacks.projection);
	r_mat_mode(MM_MODELVIEW);

	R.texunits.active = R.texunits.indexed;

	r_enable(RCAP_DEPTH_TEST);
	r_enable(RCAP_DEPTH_WRITE);
	r_enable(RCAP_CULL_FACE);
	r_depth_func(DEPTH_LEQUAL);
	r_cull(CULL_BACK);
	r_blend(BLEND_ALPHA);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetIntegerv(GL_VIEWPORT, &R.viewport.x);

	r_clear_color4(0, 0, 0, 1);
	r_clear(CLEAR_ALL);
}

static inline attr_must_inline uint32_t cap_flag(RendererCapability cap) {
	uint32_t idx = cap;
	assert(idx < NUM_RCAPS);
	return (1 << idx);
}
/*
void r_capability(RendererCapability cap, bool value) {
	uint32_t flag = cap_flag(cap);

	if((bool)(R.capstates & flag) == value) {
		return;
	}

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

	if(value) {
		R.capstates |= flag;
	} else {
		R.capstates &= ~flag;
	}
}

bool r_capability_current(RendererCapability cap) {
	return R.capstates & cap_flag(cap);
}
*/

void gl33_apply_capability(RendererCapability cap, bool value) {
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

void gl33_sync_capabilities(void) {
	if(R.capabilities.active == R.capabilities.pending) {
		return;
	}

	for(RendererCapability cap = 0; cap < NUM_RCAPS; ++cap) {
		uint32_t flag = cap_flag(cap);
		bool pending = R.capabilities.pending & flag;
		bool active = R.capabilities.active & flag;

		if(pending != active) {
			gl33_apply_capability(cap, pending);
		}
	}

	R.capabilities.active = R.capabilities.pending;
}

void r_capability(RendererCapability cap, bool value) {
	uint32_t flag = cap_flag(cap);

	if(value) {
		R.capabilities.pending |=  flag;
	} else {
		R.capabilities.pending &= ~flag;
	}
}

bool r_capability_current(RendererCapability cap) {
	return R.capabilities.pending & cap_flag(cap);
}

uint gl33_active_texunit(void) {
	return (uintptr_t)(R.texunits.active - R.texunits.indexed);
}

uint gl33_activate_texunit(uint unit) {
	assert(unit < GL33_MAX_TEXUNITS);
	uint prev_unit = gl33_active_texunit();

	if(prev_unit != unit) {
		glActiveTexture(GL_TEXTURE0 + unit);
		R.texunits.active = R.texunits.indexed + unit;
	}

	return prev_unit;
}

SDL_Window* r_create_window(const char *title, int x, int y, int w, int h, uint32_t flags) {
	static bool libgl_loaded;

	if(!libgl_loaded) {
		load_gl_library();
		libgl_loaded = true;
	}

	SDL_GLcontextFlag ctxflags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;

#ifdef DEBUG_GL
	ctxflags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, ctxflags);

	SDL_Window *window = SDL_CreateWindow(title, x, y, w, h, flags | SDL_WINDOW_OPENGL);

	if(R.gl_context) {
		SDL_GL_MakeCurrent(window, R.gl_context);
	} else {
		gl33_init_context(window);
	}

	return window;
}

void _r_init(void) {
	preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
		"standard",
		"standardnotex",
	NULL);

	R.progs.std = get_resource_data(RES_SHADER_PROGRAM, "standard", RESF_PERMANENT);
	R.progs.std_notex = get_resource_data(RES_SHADER_PROGRAM, "standardnotex", RESF_PERMANENT);

	r_shader_standard();
}

void _r_shutdown(void) {
	unload_gl_library();
	SDL_GL_DeleteContext(R.gl_context);
}

void r_vsync(VsyncMode mode) {
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

VsyncMode r_vsync_current(void) {
	int interval = SDL_GL_GetSwapInterval();

	if(interval == 0) {
		return VSYNC_NONE;
	}

	return interval > 0 ? VSYNC_NORMAL : VSYNC_ADAPTIVE;
}

static inline vec4* active_matrix(void) {
	return *R.mstacks.active->head;
}

void r_mat_mode(MatrixMode mode) {
	assert((uint)mode < sizeof(R.mstacks.indexed) / sizeof(MatrixStack));
	R.mstacks.active = &R.mstacks.indexed[mode];
}

void r_mat_push(void) {
	matstack_push(R.mstacks.active);
}

void r_mat_pop(void) {
	matstack_pop(R.mstacks.active);
}

void r_mat_identity(void) {
	glm_mat4_identity(active_matrix());
}

void r_mat_translate_v(vec3 v) {
	glm_translate(active_matrix(), v);
}

void r_mat_rotate_v(float angle, vec3 v) {
	glm_rotate(active_matrix(), angle, v);
}

void r_mat_scale_v(vec3 v) {
	glm_scale(active_matrix(), v);
}

void r_mat_ortho(float left, float right, float bottom, float top, float near, float far) {
	glm_ortho(left, right, bottom, top, near, far, active_matrix());
}

void r_mat_perspective(float angle, float aspect, float near, float far) {
	glm_perspective(angle, aspect, near, far, active_matrix());
}

void r_mat_current(MatrixMode mode, mat4 out_mat) {
	assert((uint)mode < sizeof(R.mstacks.indexed) / sizeof(MatrixStack));
	glm_mat4_copy(*R.mstacks.indexed[mode].head, out_mat);
}

void r_color4(float r, float g, float b, float a) {
	glm_vec4_copy((vec4) { r, g, b, a }, R.color);
}

Color r_color_current(void) {
	return rgba(R.color[0], R.color[1], R.color[2], R.color[3]);
}

void gl33_sync_vertex_buffer(void) {
	R.vertex_buffer.active = R.vertex_buffer.pending;

	if(R.vertex_buffer.gl_vao != R.vertex_buffer.active->impl->gl_vao) {
		R.vertex_buffer.gl_vao = R.vertex_buffer.active->impl->gl_vao;
		glBindVertexArray(R.vertex_buffer.gl_vao);
	}

	if(R.vertex_buffer.gl_vbo != R.vertex_buffer.active->impl->gl_vbo) {
		R.vertex_buffer.gl_vbo = R.vertex_buffer.active->impl->gl_vbo;
		glBindBuffer(GL_ARRAY_BUFFER, R.vertex_buffer.active->impl->gl_vbo);
	}
}

void r_vertex_buffer(VertexBuffer *vbuf) {
	assert(vbuf->impl != NULL);

	R.vertex_buffer.pending = vbuf;
}

VertexBuffer* r_vertex_buffer_current(void) {
	return R.vertex_buffer.pending;
}

static void gl33_sync_state(void) {
	gl33_sync_capabilities();
	gl33_sync_shader();
	r_uniform("ctx.modelViewMatrix", 1, R.mstacks.modelview.head);
	r_uniform("ctx.projectionMatrix", 1, R.mstacks.projection.head);
	r_uniform("ctx.textureMatrix", 1, R.mstacks.texture.head);
	r_uniform("ctx.color", 1, R.color);
	gl33_sync_uniforms(R.progs.active);
	gl33_sync_texunits();
	gl33_sync_render_target();
	gl33_sync_vertex_buffer();
	gl33_sync_blend_mode();

	if(R.capabilities.active & cap_flag(RCAP_CULL_FACE)) {
		gl33_sync_cull_face_mode();
	}

	if(R.capabilities.active & cap_flag(RCAP_DEPTH_TEST)) {
		gl33_sync_depth_test_func();
	}
}

static GLenum prim_to_gl_prim[] = {
	[PRIM_POINTS]         = GL_POINTS,
	[PRIM_LINE_STRIP]     = GL_LINE_STRIP,
	[PRIM_LINE_LOOP]      = GL_LINE_LOOP,
	[PRIM_LINES]          = GL_LINES,
	[PRIM_TRIANGLE_STRIP] = GL_TRIANGLE_STRIP,
	[PRIM_TRIANGLE_FAN]   = GL_TRIANGLE_FAN,
	[PRIM_TRIANGLES]      = GL_TRIANGLES,
};

void r_draw(Primitive prim, uint first, uint count, uint32_t *indices, uint instances) {
	assert(count > 0);
	assert((uint)prim < sizeof(prim_to_gl_prim)/sizeof(GLenum));

	r_flush_sprites();

	GLuint gl_prim = prim_to_gl_prim[prim];
	gl33_sync_state();

	if(indices == NULL) {
		if(instances) {
			gl33_stats_pre_draw();
			glDrawArraysInstanced(gl_prim, first, count, instances);
			gl33_stats_post_draw();
		} else {
			gl33_stats_pre_draw();
			glDrawArrays(gl_prim, first, count);
			gl33_stats_post_draw();
		}
	} else {
		if(instances) {
			gl33_stats_pre_draw();
			glDrawElementsInstanced(gl_prim, count, GL_UNSIGNED_INT, indices, instances);
			gl33_stats_post_draw();
		} else {
			gl33_stats_pre_draw();
			glDrawElements(gl_prim, count, GL_UNSIGNED_INT, indices);
			gl33_stats_post_draw();
		}
	}
}

void gl33_sync_texunit(uint unit) {
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
}

void gl33_sync_texunits(void) {
	for(uint i = 0; i < GL33_MAX_TEXUNITS; ++i) {
		gl33_sync_texunit(i);
	}
}

void gl33_texture_deleted(Texture *tex) {
	for(uint i = 0; i < GL33_MAX_TEXUNITS; ++i) {
		if(R.texunits.indexed[i].tex2d.pending == tex) {
			R.texunits.indexed[i].tex2d.pending = NULL;
		}
	}
}

void r_texture_ptr(uint unit, Texture *tex) {
	assert(unit < GL33_MAX_TEXUNITS);

	if(tex != NULL) {
		assert(tex->impl != NULL);
		assert(tex->impl->gl_handle != 0);
	}

	R.texunits.indexed[unit].tex2d.pending = tex;
}

Texture* r_texture_current(uint unit) {
	assert(unit < GL33_MAX_TEXUNITS);
	return R.texunits.indexed[unit].tex2d.pending;
}

static inline GLuint fbo_num(RenderTarget *target) {
	if(target == NULL) {
		return 0;
	}

	assert(target->impl != NULL);
	assert(target->impl->gl_fbo != 0);

	return target->impl->gl_fbo;
}

void gl33_sync_render_target(void) {
	if(fbo_num(R.render_target.active) != fbo_num(R.render_target.pending)) {
		r_flush_sprites();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_num(R.render_target.pending));
		R.render_target.active = R.render_target.pending;
	}

	if(R.render_target.active) {
		gl33_target_initialize(R.render_target.active);
	}
}

void r_target(RenderTarget *target) {
	assert(target == NULL || target->impl != NULL);
	R.render_target.pending = target;

	// TODO: figure out why this is necessary, fix it, and remove this.
	gl33_sync_render_target();
}

RenderTarget *r_target_current() {
	return R.render_target.pending;
}

void gl33_sync_shader(void) {
	if(R.progs.pending && R.progs.gl_prog != R.progs.pending->gl_handle) {
		glUseProgram(R.progs.pending->gl_handle);
		R.progs.gl_prog = R.progs.pending->gl_handle;
		R.progs.active = R.progs.pending;
	}
}

void r_shader_ptr(ShaderProgram *prog) {
	assert(prog != NULL);
	assert(prog->gl_handle != 0);

	R.progs.pending = prog;
}

void r_shader_standard(void) {
	r_shader_ptr(R.progs.std);
}

void r_shader_standard_notex(void) {
	r_shader_ptr(R.progs.std_notex);
}

ShaderProgram *r_shader_current() {
	return R.progs.pending;
}

void r_clear(ClearBufferFlags flags) {
	GLbitfield mask = 0;

	if(flags & CLEAR_COLOR) {
		mask |= GL_COLOR_BUFFER_BIT;
	}

	if(flags & CLEAR_DEPTH) {
		mask |= GL_DEPTH_BUFFER_BIT;
	}

	r_flush_sprites();
	gl33_sync_render_target();
	glClear(mask);
}

void r_clear_color4(float r, float g, float b, float a) {
	vec4 cc = { r, g, b, a };

	if(!memcmp(R.clear_color, cc, sizeof(cc))) {
		memcpy(R.clear_color, cc, sizeof(cc));
		glClearColor(r, g, b, a);
	}
}

Color r_clear_color_current(void) {
	return rgba(R.clear_color[0], R.clear_color[1], R.clear_color[2], R.clear_color[3]);
}

void r_viewport_rect(IntRect rect) {
	if(memcmp(&R.viewport, &rect, sizeof(IntRect))) {
		r_flush_sprites();
		memcpy(&R.viewport, &rect, sizeof(IntRect));
		glViewport(rect.x, rect.y, rect.w, rect.h);
	}
}

void r_viewport_current(IntRect *out_rect) {
	memcpy(out_rect, &R.viewport, sizeof(R.viewport));
}

void r_swap(SDL_Window *window) {
	r_flush_sprites();
	gl33_sync_render_target();
	SDL_GL_SwapWindow(window);
	gl33_stats_post_frame();
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

void r_blend(BlendMode mode) {
	// TODO: validate it here?
	R.blend.mode.pending = mode;
}

BlendMode r_blend_current(void) {
	return R.blend.mode.pending;
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

void gl33_sync_cull_face_mode(void) {
	if(R.cull_face.mode.pending != R.cull_face.mode.active) {
		GLenum glcull = r_cull_to_gl_cull(R.cull_face.mode.pending);
		glCullFace(glcull);
		R.cull_face.mode.active = R.cull_face.mode.pending;
	}
}

void r_cull(CullFaceMode mode) {
	// TODO: validate it here?
	R.cull_face.mode.pending = mode;
}

CullFaceMode r_cull_current(void) {
	return R.cull_face.mode.pending;
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

void r_depth_func(DepthTestFunc func) {
	// TODO: validate it here?
	R.depth_test.func.pending = func;
}

DepthTestFunc r_depth_func_current(void) {
	return R.depth_test.func.pending;
}

uint8_t* r_screenshot(uint *out_width, uint *out_height) {
	uint8_t *pixels = malloc(R.viewport.w * R.viewport.h * 3);
	glReadBuffer(GL_FRONT);
	glReadPixels(R.viewport.x, R.viewport.y, R.viewport.w, R.viewport.h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	*out_width = R.viewport.w;
	*out_height = R.viewport.h;
	return pixels;
}

void gl33_bind_pbo(GLuint pbo) {
	if(pbo != R.pbo) {
		// r_flush_sprites();
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
		R.pbo = pbo;
	}
}
