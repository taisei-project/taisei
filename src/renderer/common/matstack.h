/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util.h"

enum {
	MATSTACK_LIMIT = 32,
};

typedef struct MatrixStack {
	mat4 *head;

	// the alignment is required for the SSE codepath in CGLM
	mat4 stack[MATSTACK_LIMIT] CGLM_ALIGN(32);
} MatrixStack;

// Resets [ms] to its initial state, with a single indentity matrix.
// Must be called at least once before the stack can be used.
void matstack_reset(MatrixStack *ms)
	attr_nonnull(1);

// Pushes a new matrix onto [ms], updates [ms]->head.
// It's initialized to a copy of the previous head.
void matstack_push(MatrixStack *ms)
	attr_nonnull(1);

// Pushes a new matrix onto [ms], updates [ms]->head.
// [mat] is copied into the new [ms]->head.
void matstack_push_premade(MatrixStack *ms, mat4 mat)
	attr_nonnull(1, 2);

// Pushes a new matrix onto [ms], updates [ms]->head.
// It's initialized to the identity matrix.
void matstack_push_identity(MatrixStack *ms)
	attr_nonnull(1);

// Pops a matrix from [ms], updates [ms]->head.
// It's an error to pop the last element.
void matstack_pop(MatrixStack *ms)
	attr_nonnull(1);

typedef struct MatrixStates {
	union {
		struct {
			MatrixStack modelview;
			MatrixStack projection;
			MatrixStack texture;
		};

		MatrixStack indexed[3];
	};
} MatrixStates;

extern MatrixStates _r_matrices;

void _r_mat_init(void);
