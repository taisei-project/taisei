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
#include "shader_program.h"
#include "texture.h"
#include "render_target.h"
#include "resource/resource.h"
#include "resource/model.h"
#include "glm.h"

// FIXME: initialize this elsewhere
#include "recolor.h"

// FIXME: move this to gl33
#include "vbo.h"

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
	GLuint ubo;
	UBOData ubodata;

	struct {
		ShaderProgram *active;
		ShaderProgram *pending;
		ShaderProgram *std;
		ShaderProgram *std_notex;
	} progs;

	SDL_GLContext *gl_context;
} R;

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
	char *strtype = "unknown";
	char *strsev = "unknown";
	LogLevel lvl = LOG_DEBUG;

	switch(type) {
		case GL_DEBUG_TYPE_ERROR:               strtype = "error";       lvl = LOG_FATAL; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: strtype = "deprecated";  lvl = LOG_WARN;  break;
		case GL_DEBUG_TYPE_PORTABILITY:         strtype = "portability";                  break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  strtype = "undefined";                    break;
		case GL_DEBUG_TYPE_PERFORMANCE:         strtype = "performance";                  break;
		case GL_DEBUG_TYPE_OTHER:               strtype = "other";                        break;
	}

	switch(severity) {
		case GL_DEBUG_SEVERITY_LOW:             strsev  = "low";                          break;
		case GL_DEBUG_SEVERITY_MEDIUM:          strsev  = "medium";                       break;
		case GL_DEBUG_SEVERITY_HIGH:            strsev  = "high";                         break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:    strsev  = "notify";                       break;
	}

	if(type == GL_DEBUG_TYPE_OTHER && severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
		// too verbose, spits a message every time some text is drawn
		return;
	}

	log_custom(lvl, "[OpenGL debug, %s, %s] %i: %s", strtype, strsev, id, message);
}

static void APIENTRY gl33_debug_enable(void) {
	GLuint unused = 0;
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(gl33_debug, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, true);
	log_info("Enabled OpenGL debugging");
}
#endif

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

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glClear(GL_COLOR_BUFFER_BIT);

	glGenBuffers(1, &R.ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, R.ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(R.ubodata), &R.ubodata, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 1, R.ubo, 0, sizeof(R.ubodata));

	// FIXME: move this to gl33
	init_quadvbo();
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

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	#ifdef DEBUG_GL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	#endif

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
	unload_gl_library();
	SDL_GL_DeleteContext(R.gl_context);
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

void r_draw_quad(void) {
	update_ubo();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void r_draw_model(const char *name) {
	Model *model = get_model(name);
	r_mat_mode(MM_TEXTURE);
	// TODO: get rid of this -1?
	r_mat_scale(1, -1, 1); // every texture in taisei is actually read vertically mirrored. and I noticed that just now.

	update_ubo();
	glDrawElements(GL_TRIANGLES, model->icount, GL_UNSIGNED_INT, model->indices);

	r_mat_identity();
	r_mat_mode(MM_MODELVIEW);
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
