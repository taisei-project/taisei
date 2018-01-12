/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "color.h"
#include "resource/shader.h"

/*
 *  Utilities for using the recolor shader in rendering code efficiently
 */

typedef struct ColorTransform {
	union {
		struct {
			Color R[2];
			Color G[2];
			Color B[2];
			Color A[2];
		};

		struct {
			Color low;
			Color high;
		} pairs[4];
	};
} ColorTransform;

extern ColorTransform colortransform_identity;

typedef void (*ColorTransformFunc)(Color clr, ColorTransform *out, void *arg);

void recolor_init(void);
void recolor_reinit(void);
Shader* recolor_get_shader(void);
void recolor_apply_transform(ColorTransform *ct);
