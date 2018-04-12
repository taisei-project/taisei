/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "../api.h"
#include "matstack.h"
#include "glm.h"

void matstack_reset(MatrixStack *ms) {
	ms->head = ms->stack;
	glm_mat4_identity(ms->stack[0]);
}

void matstack_push(MatrixStack *ms) {
	assert(ms->head < ms->stack + MATSTACK_LIMIT);
	glm_mat4_copy(*ms->head, *(ms->head + 1));
	ms->head++;
}

void matstack_pop(MatrixStack *ms) {
	assert(ms->head > ms->stack);
	ms->head--;
}

MatrixStates _r_matrices;

static inline vec4* active_matrix(void) {
	return *_r_matrices.active->head;
}

void r_mat_mode(MatrixMode mode) {
	assert((uint)mode < sizeof(_r_matrices.indexed) / sizeof(MatrixStack));
	_r_matrices.active = &_r_matrices.indexed[mode];
}

void r_mat_push(void) {
	matstack_push(_r_matrices.active);
}

void r_mat_pop(void) {
	matstack_pop(_r_matrices.active);
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
	assert((uint)mode < sizeof(_r_matrices.indexed) / sizeof(MatrixStack));
	glm_mat4_copy(*_r_matrices.indexed[mode].head, out_mat);
}

void _r_mat_init(void) {
	matstack_reset(&_r_matrices.texture);
	matstack_reset(&_r_matrices.modelview);
	matstack_reset(&_r_matrices.projection);
	r_mat_mode(MM_MODELVIEW);
}
