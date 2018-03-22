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

// FIXME: initialize this elsewhere
#include "recolor.h"

typedef struct UBOData {
	mat4 modelview;
	mat4 projection;
	mat4 texture;
	vec4 color;
} UBOData;

typedef struct TextureUnit {
	struct {
		GLuint gl_handle;
		Texture *ptr;
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
	} texunits;

	RenderTarget *render_target;
	vec4 color;
	// vec4 clear_color;
	GLuint ubo;
	UBOData ubodata;
	IntRect viewport;
	uint32_t capstates;
	bool blend_enabled;
	BlendMode blend_mode;
	CullFaceMode cull_mode;
	GLenum gl_cull_mode;
	DepthTestFunc depth_func;
	GLenum gl_vbo;
	GLenum gl_vao;
	VertexBuffer *vbuf;

	struct {
		ShaderProgram *active;
		ShaderProgram *pending;
		ShaderProgram *std;
		ShaderProgram *std_notex;
	} progs;

	SDL_GLContext *gl_context;

	VertexBuffer models_vbuf;

	struct {
		hrtime_t last_draw;
		hrtime_t draw_time;
		uint draw_calls;
	} stats;
} R;

static inline void gl33_stats_pre_draw(void) {
	R.stats.last_draw = time_get();
	R.stats.draw_calls++;
}

static inline void gl33_stats_post_draw(void) {
	R.stats.draw_time += (time_get() - R.stats.last_draw);
}

static inline void gl33_stats_post_frame(void) {
	// log_debug("%.20gs spent in %u draw calls", (double)R.stats.draw_time, R.stats.draw_calls);
	memset(&R.stats, 0, sizeof(R.stats));
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

	R.texunits.active = R.texunits.indexed;

	r_enable(RCAP_DEPTH_TEST);
	r_enable(RCAP_DEPTH_WRITE);
	r_depth_func(DEPTH_LEQUAL);
	r_cull(CULL_BACK);
	r_blend(BLEND_ALPHA);

	glGenBuffers(1, &R.ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, R.ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(R.ubodata), &R.ubodata, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 1, R.ubo, 0, sizeof(R.ubodata));
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetIntegerv(GL_VIEWPORT, &R.viewport.x);

	r_clear_color4(0, 0, 0, 1);
	r_clear(CLEAR_ALL);

	R.vbuf = &R.models_vbuf;

	r_vertex_buffer_create(&R.models_vbuf, 8192 * sizeof(StaticModelVertex), 3, (VertexAttribFormat[]) {
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // position
		{ 3, VA_FLOAT, VA_CONVERT_FLOAT }, // normal
		{ 2, VA_FLOAT, VA_CONVERT_FLOAT }, // texcoord
	});

	gl33_vertex_buffer_append_quad_model(&R.models_vbuf);
}

static inline attr_must_inline uint32_t cap_flag(RendererCapability cap) {
	uint32_t idx = cap;
	assert(idx < NUM_RCAPS);
	return (1 << idx);
}

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

uint gl33_active_texunit(void) {
	return (uintptr_t)(R.texunits.active - R.texunits.indexed);
}

uint gl33_activate_texunit(uint unit) {
	assert(unit < GL33_MAX_TEXUNITS);
	uint prev_unit = gl33_active_texunit();

	if(prev_unit != unit) {
		// TODO: defer until needed
		r_flush();
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

void r_init(void) {
	preload_resources(RES_SHADER_PROGRAM, RESF_PERMANENT,
		"standard",
		"standardnotex",
	NULL);

	R.progs.std = get_resource_data(RES_SHADER_PROGRAM, "standard", RESF_PERMANENT);
	R.progs.std_notex = get_resource_data(RES_SHADER_PROGRAM, "standardnotex", RESF_PERMANENT);

	r_shader_standard();

	// FIXME: initialize this elsewhere
	recolor_init();
}

void r_shutdown(void) {
	glDeleteBuffers(1, &R.ubo);
	r_vertex_buffer_destroy(&R.models_vbuf);
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
	assert(mode >= 0);
	assert(mode < sizeof(R.mstacks.indexed) / sizeof(MatrixStack));
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

void r_color4(float r, float g, float b, float a) {
	glm_vec4_copy((vec4) { r, g, b, a }, R.color);
}

static void update_prog(void) {
	if(R.progs.pending != R.progs.active) {
		log_debug("switch program %i", R.progs.pending->gl_handle);
		glUseProgram(R.progs.pending->gl_handle);
		R.progs.active = R.progs.pending;
	}
}

static void update_ubo(void) {
	// update_prog();

	/*
	 *	if(R.progs.active->renderctx_block_idx < 0) {
	 *		return;
	}
*/

	UBOData ubo;
	memset(&ubo, 0, sizeof(ubo));

	glm_mat4_copy(*R.mstacks.modelview.head, ubo.modelview);
	glm_mat4_copy(*R.mstacks.projection.head, ubo.projection);
	glm_mat4_copy(*R.mstacks.texture.head, ubo.texture);
	glm_vec4_copy(R.color, ubo.color);

	if(!memcmp(&R.ubodata, &ubo, sizeof(ubo))) {
		return;
	}

	memcpy(&R.ubodata, &ubo, sizeof(ubo));
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(R.ubodata), &R.ubodata);
}

void r_vertex_buffer(VertexBuffer *vbuf) {
	assert(vbuf->impl != NULL);

	if(vbuf != R.vbuf) {
		R.vbuf = vbuf;
	}

	if(R.gl_vao != vbuf->impl->gl_vao) {
		R.gl_vao = vbuf->impl->gl_vao;
		glBindVertexArray(R.gl_vao);
	}

	if(R.gl_vbo != vbuf->impl->gl_vbo) {
		R.gl_vbo = vbuf->impl->gl_vbo;
		glBindBuffer(GL_ARRAY_BUFFER, R.gl_vbo);
	}
}

VertexBuffer* r_vertex_buffer_current(void) {
	return R.vbuf;
}

VertexBuffer* r_vertex_buffer_static_models(void) {
	return &R.models_vbuf;
}

void r_draw_quad(void) {
	update_ubo();
	VertexBuffer *vbuf_saved = R.vbuf;
	r_vertex_buffer(&R.models_vbuf);
	gl33_stats_pre_draw();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	gl33_stats_post_draw();
	r_vertex_buffer(vbuf_saved);
}

void r_draw_quad_instanced(uint instances) {
	update_ubo();
	VertexBuffer *vbuf_saved = R.vbuf;
	r_vertex_buffer(&R.models_vbuf);
	gl33_stats_pre_draw();
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, instances);
	gl33_stats_post_draw();
	r_vertex_buffer(vbuf_saved);
}

void r_draw_model(const char *name) {
	VertexBuffer *vbuf_saved = R.vbuf;
	r_vertex_buffer(&R.models_vbuf);

	Model *model = get_model(name);
	r_mat_mode(MM_TEXTURE);
	// TODO: get rid of this -1?
	r_mat_scale(1, -1, 1); // every texture in taisei is actually read vertically mirrored. and I noticed that just now.

	update_ubo();
	gl33_stats_pre_draw();
	glDrawElements(GL_TRIANGLES, model->icount, GL_UNSIGNED_INT, model->indices);
	gl33_stats_post_draw();

	r_mat_identity();
	r_mat_mode(MM_MODELVIEW);

	r_vertex_buffer(vbuf_saved);
}

void r_texture_ptr(uint unit, Texture *tex) {
	assert(unit < GL33_MAX_TEXUNITS);

	if(tex == NULL) {
		if(R.texunits.indexed[unit].tex2d.gl_handle != 0) {
			glBindTexture(GL_TEXTURE_2D, 0);
			R.texunits.indexed[unit].tex2d.gl_handle = 0;
			R.texunits.indexed[unit].tex2d.ptr = NULL;
		}

		return;
	}

	assert(tex->impl != NULL);
	assert(tex->impl->gl_handle != 0);

	if(R.texunits.indexed[unit].tex2d.gl_handle != tex->impl->gl_handle) {
		// TODO: defer until needed
		gl33_activate_texunit(unit);
		r_flush();
		glBindTexture(GL_TEXTURE_2D, tex->impl->gl_handle);
		R.texunits.indexed[unit].tex2d.gl_handle = tex->impl->gl_handle;
		R.texunits.indexed[unit].tex2d.ptr = tex;
	}
}

Texture* r_texture_current(uint unit) {
	assert(unit < GL33_MAX_TEXUNITS);
	return R.texunits.indexed[unit].tex2d.ptr;
}

static inline GLuint fbo_num(RenderTarget *target) {
	if(target == NULL) {
		return 0;
	}

	assert(target->impl != NULL);
	assert(target->impl->gl_fbo != 0);

	return target->impl->gl_fbo;
}

void r_target(RenderTarget *target) {
	// TODO: defer until needed

	if(fbo_num(R.render_target) != fbo_num(target)) {
		r_flush();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_num(target));
		R.render_target = target;
	}
}

RenderTarget *r_target_current() {
	return R.render_target;
}

void r_shader_ptr(ShaderProgram *prog) {
	// TODO: defer until needed

	assert(prog != NULL);
	assert(prog->gl_handle != 0);

	R.progs.pending = prog;
	// update_prog();
	glUseProgram(prog->gl_handle);
	R.progs.active = prog;
}

void r_shader_standard(void) {
	r_shader_ptr(R.progs.std);
}

void r_shader_standard_notex(void) {
	r_shader_ptr(R.progs.std_notex);
}

ShaderProgram *r_shader_current() {
	// update_prog();
	return R.progs.active;
}

void r_flush(void) {
	update_ubo();
}

void r_clear(ClearBufferFlags flags) {
	GLbitfield mask = 0;

	if(flags & CLEAR_COLOR) {
		mask |= GL_COLOR_BUFFER_BIT;
	}

	if(flags & CLEAR_DEPTH) {
		mask |= GL_DEPTH_BUFFER_BIT;
	}

	glClear(mask);
}

void r_clear_color4(float r, float g, float b, float a) {
	glClearColor(r, g, b, a);
}

void r_viewport_rect(IntRect rect) {
	if(memcmp(&R.viewport, &rect, sizeof(IntRect))) {
		memcpy(&R.viewport, &rect, sizeof(IntRect));
		glViewport(rect.x, rect.y, rect.w, rect.h);
	}
}

void r_viewport_current(IntRect *out_rect) {
	memcpy(out_rect, &R.viewport, sizeof(R.viewport));
}

void r_swap(SDL_Window *window) {
	r_target(NULL);
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

void r_blend(BlendMode mode) {
	if(mode == BLEND_NONE) {
		if(R.blend_enabled) {
			glDisable(GL_BLEND);
		}

		return;
	}

	if(!R.blend_enabled) {
		glEnable(GL_BLEND);
	}

	if(mode != R.blend_mode) {
		static UnpackedBlendMode umode;
		r_blend_unpack(mode, &umode);
		R.blend_mode = mode;

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

BlendMode r_blend_current(void) {
	return R.blend_mode;
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

void r_cull(CullFaceMode mode) {
	if(mode != R.cull_mode) {
		if(mode == CULL_NONE) {
			glDisable(GL_CULL_FACE);
		} else {
			if(R.cull_mode == CULL_NONE) {
				glEnable(GL_CULL_FACE);
			}

			GLenum glcull = r_cull_to_gl_cull(mode);

			if(glcull != R.gl_cull_mode) {
				glCullFace(glcull);
				R.gl_cull_mode = glcull;
			}
		}

		R.cull_mode = mode;
	}
}

CullFaceMode r_cull_current(void) {
	return R.cull_mode;
}

void r_depth_func(DepthTestFunc func) {
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

	if(R.depth_func != func) {
		glDepthFunc(func_to_glfunc[idx]);
		R.depth_func = func;
	}
}

DepthTestFunc r_depth_func_current(void) {
	return R.depth_func;
}

uint8_t* r_screenshot(uint *out_width, uint *out_height) {
	uint8_t *pixels = malloc(R.viewport.w * R.viewport.h * 3);
	glReadBuffer(GL_FRONT);
	glReadPixels(R.viewport.x, R.viewport.y, R.viewport.w, R.viewport.h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	*out_width = R.viewport.w;
	*out_height = R.viewport.h;
	return pixels;
}
