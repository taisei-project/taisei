/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "matstack.h"

#include "../api.h"
#include "util/glm.h"

void matstack_reset(MatrixStack *ms) {
	ms->head = ms->stack;
	glm_mat4_identity(ms->stack[0]);
}

static void matstack_push_raw(MatrixStack *ms) {
	assert(ms->head < ms->stack + MATSTACK_LIMIT);
	ms->head++;
}

void matstack_push(MatrixStack *ms) {
	matstack_push_raw(ms);
	glm_mat4_copy(*(ms->head - 1), *ms->head);
}

void matstack_push_premade(MatrixStack *ms, mat4 mat) {
	matstack_push_raw(ms);
	glm_mat4_copy(mat, *ms->head);
}

void matstack_push_identity(MatrixStack *ms) {
	matstack_push_raw(ms);
	glm_mat4_identity(*ms->head);
}

void matstack_pop(MatrixStack *ms) {
	assert(ms->head > ms->stack);
	ms->head--;
}

MatrixStates _r_matrices;

// BEGIN modelview

void r_mat_mv_push(void) {
	matstack_push(&_r_matrices.modelview);
}

void r_mat_mv_push_premade(mat4 mat) {
	matstack_push_premade(&_r_matrices.modelview, mat);
}

void r_mat_mv_push_identity(void) {
	matstack_push_identity(&_r_matrices.modelview);
}

void r_mat_mv_pop(void) {
	matstack_pop(&_r_matrices.modelview);
}

void r_mat_mv(mat4 mat) {
	glm_mat4_copy(mat, *_r_matrices.modelview.head);
}

void r_mat_mv_current(mat4 out_mat) {
	glm_mat4_copy(*_r_matrices.modelview.head, out_mat);
}

mat4 *r_mat_mv_current_ptr(void) {
	return _r_matrices.modelview.head;
}

void r_mat_mv_identity(void) {
	glm_mat4_identity(*_r_matrices.modelview.head);
}

void r_mat_mv_translate_v(vec3 v) {
	glm_translate(*_r_matrices.modelview.head, v);
}

void r_mat_mv_rotate_v(float angle, vec3 v) {
	glm_rotate(*_r_matrices.modelview.head, angle, v);
}

void r_mat_mv_scale_v(vec3 v) {
	glm_scale(*_r_matrices.modelview.head, v);
}

// END modelview

// BEGIN projection

void r_mat_proj_push(void) {
	matstack_push(&_r_matrices.projection);
}

void r_mat_proj_push_premade(mat4 mat) {
	matstack_push_premade(&_r_matrices.projection, mat);
}

void r_mat_proj_push_identity(void) {
	matstack_push_identity(&_r_matrices.projection);
}

void r_mat_proj_push_ortho_ex(float left, float right, float bottom, float top, float near, float far) {
	matstack_push_raw(&_r_matrices.projection);
	r_mat_proj_ortho(left, right, bottom, top, near, far);
}

void r_mat_proj_push_ortho(float width, float height) {
	r_mat_proj_push_ortho_ex(0, width, height, 0, -1, 1);
}

void r_mat_proj_push_perspective(float angle, float aspect, float near, float far) {
	matstack_push_raw(&_r_matrices.projection);
	r_mat_proj_perspective(angle, aspect, near, far);
}

void r_mat_proj_pop(void) {
	matstack_pop(&_r_matrices.projection);
}

void r_mat_proj(mat4 mat) {
	glm_mat4_copy(mat, *_r_matrices.projection.head);
}

void r_mat_proj_current(mat4 out_mat) {
	glm_mat4_copy(*_r_matrices.projection.head, out_mat);
}

mat4 *r_mat_proj_current_ptr(void) {
	return _r_matrices.projection.head;
}

void r_mat_proj_identity(void) {
	glm_mat4_identity(*_r_matrices.projection.head);
}

void r_mat_proj_translate_v(vec3 v) {
	glm_translate(*_r_matrices.projection.head, v);
}

void r_mat_proj_rotate_v(float angle, vec3 v) {
	glm_rotate(*_r_matrices.projection.head, angle, v);
}

void r_mat_proj_scale_v(vec3 v) {
	glm_scale(*_r_matrices.projection.head, v);
}

void r_mat_proj_ortho(float left, float right, float bottom, float top, float near, float far) {
	glm_ortho(left, right, bottom, top, near, far, *_r_matrices.projection.head);
}

void r_mat_proj_perspective(float angle, float aspect, float near, float far) {
	glm_perspective(angle, aspect, near, far, *_r_matrices.projection.head);
}

// END projection

// BEGIN texture

void r_mat_tex_push(void) {
	matstack_push(&_r_matrices.texture);
}

void r_mat_tex_push_premade(mat4 mat) {
	matstack_push_premade(&_r_matrices.texture, mat);
}

void r_mat_tex_push_identity(void) {
	matstack_push_identity(&_r_matrices.texture);
}

void r_mat_tex_pop(void) {
	matstack_pop(&_r_matrices.texture);
}

void r_mat_tex(mat4 mat) {
	glm_mat4_copy(mat, *_r_matrices.texture.head);
}

void r_mat_tex_current(mat4 out_mat) {
	glm_mat4_copy(*_r_matrices.texture.head, out_mat);
}

mat4 *r_mat_tex_current_ptr(void) {
	return _r_matrices.texture.head;
}

void r_mat_tex_identity(void) {
	glm_mat4_identity(*_r_matrices.texture.head);
}

void r_mat_tex_translate_v(vec3 v) {
	glm_translate(*_r_matrices.texture.head, v);
}

void r_mat_tex_rotate_v(float angle, vec3 v) {
	glm_rotate(*_r_matrices.texture.head, angle, v);
}

void r_mat_tex_scale_v(vec3 v) {
	glm_scale(*_r_matrices.texture.head, v);
}

// END texture

void _r_mat_init(void) {
	matstack_reset(&_r_matrices.texture);
	matstack_reset(&_r_matrices.modelview);
	matstack_reset(&_r_matrices.projection);
}
