/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "renderer.h"

Renderer render;

void matstack_init(MatrixStack *ms) {
	ms->head = 0;
	glm_mat4_identity(ms->stack[0]);
}	

void matstack_push(MatrixStack *ms) {
	if(ms->head >= RENDER_MATRIX_STACKSIZE) {
		log_fatal("matrix.stackoverflow.com/jobs: Increase RENDER_MATRIX_STACKSIZE");
	}
	glm_copy(ms->stack[head],ms->stack[head+1]);
	ms->head++;
}

void matstack_pop(MatrixStack *ms) {
	assert(ms->head > 0);
	ms->head--;
}

void render_init(Renderer *r) {
	matstack_init(&r->texture);
	matstack_init(&r->modelview);
	matstack_init(&r->projection);
	r->changed = true;
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
			log_fatal("Invalid MatrixMode: %d",r->mode);
	}
}

static mat4 active_matrix(Renderer *r) {
	MatrixStack *ms = active_matrixstack(r);
	return ms->stack[ms->head];
}

void render_matrix_mode(Renderer *r, MatrixMode mode) {
	r->mode=mode;
}

void render_push(Renderer *r) {
	matstack_push(active_matrixstack(r));
}
void render_pop(Renderer *r) {
	matstack_pop(active_matrixstack(r));
	r->changed=true;
}

void render_identity(Renderer *r) {
	glm_mat4_identity(active_matrix(r));
}

void render_translate(Renderer *r, vec3 offset) {
	glm_translate(active_matrix(r),offset);
	r->changed = true;
}

void render_rotate(Renderer *r, float angle, vec3 axis) {
	glm_rotate(active_matrix(r), angle, axis);
	r->changed = true;
}

void render_rotate_deg(Renderer *r, float angle_degrees, vec3 axis) {
	glm_rotate(active_matrix(r), glm_rad(angle_degrees), axis);
	r->changed = true;
}

void render_scale(Renderer *r, vec3 factors) {
	glm_scale(active_matrix, factors);
	r->changed = true;
}

void render_color(Renderer *r, Color c) {
	r->color = c;
	r->changed = true;
}
