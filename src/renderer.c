/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "renderer.h"
#include "resource/shader_program.h"
#include "resource/resource.h"
#include "glm.h"

#include "recolor.h"

static struct {
	// MatrixMode mode;

	struct {
		union {
			struct {
				MatrixStack modelview;
				MatrixStack projection;
				MatrixStack texture;
			};

			MatrixStack indexed[MM_NUM_MODES];
		};

		MatrixStack *active;
	} mstacks;

	vec4 color;

	GLuint ubo;
	RendererUBOData ubodata;

	struct {
		ShaderProgram *active;
		ShaderProgram *pending;
		ShaderProgram *std;
		ShaderProgram *std_notex;
	} progs;
} R;

void matstack_init(MatrixStack *ms) {
	ms->head = ms->stack;
	glm_mat4_identity(ms->stack[0]);
}

void matstack_push(MatrixStack *ms) {
	if(ms->head >= ms->stack + RENDER_MATRIX_STACKSIZE) {
		log_fatal("matrix.stackoverflow.com/jobs: Increase RENDER_MATRIX_STACKSIZE");
	}

	glm_mat4_copy(*ms->head, *(ms->head + 1));
	ms->head++;
}

void matstack_pop(MatrixStack *ms) {
	assert(ms->head > ms->stack);
	ms->head--;
}

void r_init(void) {
	matstack_init(&R.mstacks.texture);
	matstack_init(&R.mstacks.modelview);
	matstack_init(&R.mstacks.projection);

	glGenBuffers(1, &R.ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, R.ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(R.ubodata), &R.ubodata, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 1, R.ubo, 0, sizeof(R.ubodata));
}

void r_post_init(void) {
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT | RESF_PERMANENT,
		"standard",
		"standardnotex",
	NULL);

	recolor_init();

	R.progs.std = get_shader_program("standard");
	R.progs.std_notex = get_shader_program("standardnotex");

	r_shader_standard();
}

void r_shutdown(void) {
	glDeleteBuffers(1, &R.ubo);
}

static inline vec4 *active_matrix(void) {
	return *R.mstacks.active->head;
}

void r_mat_mode(MatrixMode mode) {
	assert(mode >= 0);
	assert(mode < MM_NUM_MODES);
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

void r_mat_translate(float x, float y, float z) {
	r_mat_translate_v((vec3) { x, y, z });
}

void r_mat_rotate_v(float angle, vec3 v) {
	glm_rotate(active_matrix(), angle, v);
}

void r_mat_rotate(float angle, float x, float y, float z) {
	r_mat_rotate_v(angle, (vec3) { x, y, z });
}

void r_mat_rotate_deg_v(float angle_degrees, vec3 v) {
	r_mat_rotate_v(glm_rad(angle_degrees), v);
}

void r_mat_rotate_deg(float angle_degrees, float x, float y, float z) {
	r_mat_rotate_deg_v(angle_degrees, (vec3) { x, y, z });
}

void r_mat_scale_v(vec3 v) {
	glm_scale(active_matrix(), v);
}

void r_mat_scale(float x, float y, float z) {
	r_mat_scale_v((vec3) { x, y, z });
}

void r_mat_ortho(float left, float right, float bottom, float top, float near, float far) {
	glm_ortho(left, right, bottom, top, near, far, active_matrix());
}

void r_mat_perspective(float angle, float aspect, float near, float far) {
	glm_perspective(glm_rad(angle), aspect, near, far, active_matrix());
}

void r_color(Color c) {
	parse_color_array(c, R.color);
}

void r_color4(float r, float g, float b, float a) {
	glm_vec4_copy((vec4) { r, g, b, a }, R.color);
}

void r_color3(float r, float g, float b) {
	glm_vec4_copy((vec4) { r, g, b, 1.0 }, R.color);

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
	if(R.progs.active->renderctx_block_idx < 0) {
		return;
	}
	*/

	RendererUBOData ubo;
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

void r_shader(ShaderProgram *prog) {
	assert(prog != NULL);
	R.progs.pending = prog;
	// update_prog();
	glUseProgram(prog->gl_handle);
	R.progs.active = prog;
}

void r_shader_standard(void) {
	r_shader(R.progs.std);
}

void r_shader_standard_notex(void) {
	// log_fatal("fuck you");
	r_shader(R.progs.std_notex);
}

ShaderProgram *r_shader_current(void) {
	// update_prog();
	return R.progs.active;
}

void r_flush(void) {
	update_ubo();
}
