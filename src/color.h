/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/miscmath.h"

typedef union Color3 {
	struct { float r, g, b; };
	vec3_noalign rgb;
} Color3;

typedef union Color {
	struct { float r, g, b, a; };
	vec4_noalign rgba;
	vec3_noalign rgb;
	Color3 color3;
	SDL_FColor sdl_fcolor;
} Color;

#define RGBA(r, g, b, a) ((Color) { { (r), (g), (b), (a) } })
#define RGBA_MUL_ALPHA(r, g, b, a) color_mul_alpha(RGBA((r), (g), (b), (a)))
#define RGB(r, g, b) RGBA((r), (g), (b), 1)

#define HSLA(h, s, l, a) color_hsla((h), (s), (l), (a))
#define HSLA_MUL_ALPHA(h, s, l, a) color_mul_alpha(HSLA((h), (s), (l), (a)))
#define HSL(h, s, l) HSLA((h), (s), (l), 1)

Color color_hsla(float h, float s, float l, float a)
	attr_const;

void color_get_hsl(const Color *c, float *out_h, float *out_s, float *out_l)
	attr_nonnull(1);

Color color_add(Color clr, Color clr2)
	attr_const;

Color color_sub(Color clr, Color clr2)
	attr_const;

Color color_mul(Color clr, Color clr2)
	attr_const;

Color color_mul_alpha(Color clr)
	attr_const;

Color color_mul_scalar(Color clr, float scalar)
	attr_const;

Color color_div(Color clr, Color clr2)
	attr_const;

Color color_div_alpha(Color clr)
	attr_const;

Color color_div_scalar(Color clr, float scalar)
	attr_const;

Color color_lerp(Color clr, Color clr2, float a)
	attr_const;;

Color color_approach(Color clr, Color clr2, float delta)
	attr_const;

Color color_set_opacity(Color clr, float opacity)
	attr_const;

bool color_equals(Color clr, Color clr2)
	attr_const;
