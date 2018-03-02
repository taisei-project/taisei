/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
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
// It's initialized to a copy of theprevious head.
void matstack_push(MatrixStack *ms)
	attr_nonnull(1);

// Pops a matrix from [ms], updates [ms]->head.
// It's an error to pop the last element.
void matstack_pop(MatrixStack *ms)
	attr_nonnull(1);
