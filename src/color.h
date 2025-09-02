/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
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

/*
 * These macros return a pointer to a new Color instance, which is *block-scoped*,
 * and has automatic storage-class.
 */

#define RGBA(r, g, b, a) (&(Color) { { (r), (g), (b), (a) } })
#define RGBA_MUL_ALPHA(r, g, b, a) color_mul_alpha(RGBA((r), (g), (b), (a)))
#define RGB(r, g, b) RGBA((r), (g), (b), 1)

#define HSLA(h, s, l, a) color_hsla((&(Color) {}), (h), (s), (l), (a))
#define HSLA_MUL_ALPHA(h, s, l, a) color_mul_alpha(HSLA((h), (s), (l), (a)))
#define HSL(h, s, l) HSLA((h), (s), (l), 1)

#define COLOR_COPY(c) color_copy((&(Color) {}), (c))

/*
 * All of these modify the first argument in-place, and return it for convenience.
 */

Color* color_copy(Color *dst, const Color *src)
	attr_nonnull(1) attr_returns_nonnull;

Color* color_hsla(Color *clr, float h, float s, float l, float a)
	attr_nonnull(1) attr_returns_nonnull;

void color_get_hsl(const Color *c, float *out_h, float *out_s, float *out_l)
	attr_nonnull(1);

Color *color_add(Color *clr, const Color *clr2)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_sub(Color *clr, const Color *clr2)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_mul(Color *clr, const Color *clr2)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_mul_alpha(Color *clr)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_mul_scalar(Color *clr, float scalar)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_div(Color *clr, const Color *clr2)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_div_alpha(Color *clr)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_div_scalar(Color *clr, float scalar)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_lerp(Color *clr, const Color *clr2, float a)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_approach(Color *clr, const Color *clr2, float delta)
	attr_nonnull(1) attr_returns_nonnull;

Color *color_set_opacity(Color *clr, float opacity)
	attr_nonnull(1) attr_returns_nonnull;

/*
 * End of color manipulation routines.
 */

bool color_equals(const Color *clr, const Color *clr2)
	attr_nonnull(1, 2);

char *color_str(const Color *clr)
	attr_nonnull(1) attr_returns_allocated;
