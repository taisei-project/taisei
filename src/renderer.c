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

struct {
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

	bool changed;

	GLuint ubo;
	RendererUBOData ubodata;

	struct {
		ShaderProgram *current;
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

void render_init(void) {
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"standard",
		"standardnotex",
	NULL);

	matstack_init(&R.mstacks.texture);
	matstack_init(&R.mstacks.modelview);
	matstack_init(&R.mstacks.projection);
	R.changed = true;

	glGenBuffers(1, &R.ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, R.ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(R.ubodata), &R.ubodata, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 1, R.ubo, 0, sizeof(R.ubodata));

	R.progs.std = get_shader_program("standard");
	R.progs.std_notex = get_shader_program("standardnotex");
}

void render_free(void) {
	glDeleteBuffers(1, &R.ubo);
}

static inline vec4 *active_matrix(void) {
	return *R.mstacks.active->head;
}

void render_matrix_mode(MatrixMode mode) {
	assert(mode > 0);
	assert(mode < MM_NUM_MODES);
	R.mstacks.active = &R.mstacks.indexed[mode];
}

void render_push(void) {
	matstack_push(R.mstacks.active);
}
void render_pop(void) {
	matstack_pop(R.mstacks.active);
	R.changed = true;
}

void render_identity(void) {
	glm_mat4_identity(active_matrix());
}

void render_translate(float x, float y, float z) {
	glm_translate(active_matrix(), (vec3){x, y, z});
	R.changed = true;
}

void render_rotate(float angle, float x, float y, float z) {
	glm_rotate(active_matrix(), angle, (vec3){x, y, z});
	R.changed = true;
}

void render_rotate_deg(float angle_degrees, float x, float y, float z) {
	glm_rotate(active_matrix(), glm_rad(angle_degrees), (vec3){x, y, z});
	R.changed = true;
}

void render_scale(float x, float y, float z) {
	glm_scale(active_matrix(), (vec3){x, y, z});
	R.changed = true;
}

void render_ortho(float left, float right, float bottom, float top, float near, float far) {
	glm_ortho(left, right, bottom, top, near, far, active_matrix());
	R.changed = true;
}

void render_perspective(float angle, float aspect, float near, float far) {
	glm_perspective(angle, aspect, near, far, active_matrix());
	R.changed = true;
}

void render_color(Color c) {
	parse_color_array(c, R.color);
	R.changed = true;
}

void render_color4(float r, float g, float b, float a) {
	glm_vec4_copy((vec4){r, g, b, a}, R.color);
	R.changed = true;
}

void render_color3(float r, float g, float b) {
	glm_vec4_copy((vec4){r, g, b, 1.0}, R.color);
	R.changed = true;
}

void render_draw_quad(void) {
	if(R.changed && R.progs.current->renderctx_block_idx >= 0) {
		glm_mat4_copy(*R.mstacks.modelview.head, R.ubodata.modelview);
		glm_mat4_copy(*R.mstacks.projection.head, R.ubodata.projection);
		glm_mat4_copy(*R.mstacks.texture.head, R.ubodata.texture);
		glm_vec4_copy(R.color, R.ubodata.color);

		// glm_mat4_print(R.ubodata.projection, stdout);

		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(R.ubodata), &R.ubodata);

		R.changed = false;
	}

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_shader(ShaderProgram *prog) {
	assert(prog != NULL);

	if(R.progs.current != prog) {
		glUseProgram(prog->gl_handle);
		R.progs.current = prog;
	}
}

void render_shader_standard(void) {
	glUseProgram(R.progs.std->gl_handle);
}

void render_shader_standard_notex(void) {
	glUseProgram(R.progs.std_notex->gl_handle);
}

ShaderProgram* render_current_shader(void) {
	return R.progs.current;
}
