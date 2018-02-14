/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "renderer.h"
#include "resource/shader.h"
#include "glm.h"

Renderer _renderer;

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
	Renderer *r = &_renderer;
	matstack_init(&r->texture);
	matstack_init(&r->modelview);
	matstack_init(&r->projection);
	r->changed = true;

	glGenBuffers(1, &r->ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, r->ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(r->ubodata), &r->ubodata, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 1, r->ubo, 0, sizeof(r->ubodata));
}

void render_free(void) {
	glDeleteBuffers(1, &_renderer.ubo);
}


static MatrixStack *active_matrixstack(Renderer *r) {
	// return ((MatrixStack*)&r->mode)+r->mode (any sane c programmer would implement it like this)
	switch(r->mode) {
		case MM_MODELVIEW:
			return &r->modelview;
		case MM_PROJECTION:
			return &r->projection;
		case MM_TEXTURE:
			return &r->texture;
		default:
			log_fatal("Invalid MatrixMode: %d", r->mode);
			return 0;
	}
}

static vec4 *active_matrix(Renderer *r) {
	MatrixStack *ms = active_matrixstack(r);
	return *ms->head;
}

void render_matrix_mode(MatrixMode mode) {
	Renderer *r = &_renderer;
	r->mode=mode;
}

void render_push(void) {
	Renderer *r = &_renderer;
	matstack_push(active_matrixstack(r));
}
void render_pop(void) {
	Renderer *r = &_renderer;
	matstack_pop(active_matrixstack(r));
	r->changed=true;
}

void render_identity(void) {
	Renderer *r = &_renderer;
	glm_mat4_identity(active_matrix(r));
}

void render_translate(float x, float y, float z) {
	Renderer *r = &_renderer;
	glm_translate(active_matrix(r), (vec3){x, y, z});
	r->changed = true;
}

void render_rotate(float angle, float x, float y, float z) {
	Renderer *r = &_renderer;
	glm_rotate(active_matrix(r), angle, (vec3){x, y, z});
	r->changed = true;
}

void render_rotate_deg(float angle_degrees, float x, float y, float z) {
	Renderer *r = &_renderer;
	glm_rotate(active_matrix(r), glm_rad(angle_degrees), (vec3){x, y, z});
	r->changed = true;
}

void render_scale(float x, float y, float z) {
	Renderer *r = &_renderer;
	glm_scale(active_matrix(r), (vec3){x, y, z});
	r->changed = true;
}

void render_ortho(float left, float right, float bottom, float top, float near, float far) {
	Renderer *r = &_renderer;
	glm_ortho(left, right, bottom, top, near, far, active_matrix(r));
	r->changed = true;
}

void render_perspective(float angle, float aspect, float near, float far) {
	Renderer *r = &_renderer;
	glm_perspective(angle, aspect, near, far, active_matrix(r));
	r->changed = true;
}

void render_color(Color c) {
	Renderer *r = &_renderer;
	parse_color_array(c, r->color);

	r->changed = true;
}

void render_color4(float r, float g, float b, float a) {
	Renderer *ren = &_renderer;
	glm_vec4_copy((vec4){r, g, b, a}, ren->color);
	ren->changed = true;
}

void render_color3(float r, float g, float b) {
	Renderer *ren = &_renderer;
	glm_vec4_copy((vec4){r, g, b, 1.0}, ren->color);
	ren->changed = true;
}

void render_draw_quad(void) {
	Renderer *r = &_renderer;
	if(r->changed) {
		glm_mat4_copy(*r->modelview.head, r->ubodata.modelview);
		glm_mat4_copy(*r->projection.head, r->ubodata.projection);
		glm_mat4_copy(*r->texture.head, r->ubodata.texture);
		glm_vec4_copy(r->color, r->ubodata.color);

		//glm_mat4_print(r->ubodata.projection, stdout);

		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(r->ubodata), &r->ubodata);

		r->changed = false;
	}
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_shader_standard(void) {
	Shader *standard = get_shader("standard");
	glUseProgram(standard->prog);
}

void render_shader_standard_notex(void) {
	Shader *standard = get_shader("standardnotex");
	glUseProgram(standard->prog);
}
