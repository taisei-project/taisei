/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "color.h"
#include "util/stringops.h"

#define COLOR_OP(c1, op, c2) do { \
	(c1)->r = (c1)->r op (c2)->r; \
	(c1)->g = (c1)->g op (c2)->g; \
	(c1)->b = (c1)->b op (c2)->b; \
	(c1)->a = (c1)->a op (c2)->a; \
} while(0);

Color* color_copy(Color *dst, const Color *src) {
	*dst = *src;
	return dst;
}

Color* color_add(Color *clr, const Color *clr2) {
	COLOR_OP(clr, +, clr2);
	return clr;
}

Color* color_sub(Color *clr, const Color *clr2) {
	COLOR_OP(clr, -, clr2);
	return clr;
}

Color* color_mul(Color *clr, const Color *clr2) {
	COLOR_OP(clr, *, clr2);
	return clr;
}

Color* color_mul_alpha(Color *clr) {
	clr->r *= clr->a;
	clr->g *= clr->a;
	clr->b *= clr->a;
	return clr;
}

Color* color_mul_scalar(Color *clr, float scalar) {
	clr->r *= scalar;
	clr->g *= scalar;
	clr->b *= scalar;
	clr->a *= scalar;
	return clr;
}

Color* color_div(Color *clr, const Color *clr2) {
	COLOR_OP(clr, /, clr2);
	return clr;
}

Color* color_div_alpha(Color *clr) {
	if(clr->a != 0) {
		clr->r /= clr->a;
		clr->g /= clr->a;
		clr->b /= clr->a;
	}

	return clr;
}

Color* color_div_scalar(Color *clr, float scalar) {
	clr->r /= scalar;
	clr->g /= scalar;
	clr->b /= scalar;
	clr->a /= scalar;
	return clr;
}

Color* color_lerp(Color *clr, const Color *clr2, float a) {
	float ia = 1 - a;
	clr->r = clr->r * ia + clr2->r * a;
	clr->g = clr->g * ia + clr2->g * a;
	clr->b = clr->b * ia + clr2->b * a;
	clr->a = clr->a * ia + clr2->a * a;
	return clr;
}

Color* color_approach(Color *clr, const Color *clr2, float delta) {
	clr->r += (clr2->r - clr->r) * delta;
	clr->g += (clr2->g - clr->g) * delta;
	clr->b += (clr2->b - clr->b) * delta;
	clr->a += (clr2->a - clr->a) * delta;
	return clr;
}

static float hue_to_rgb(float v1, float v2, float vH) {
	if(vH < 0) {
		vH += 1;
	}

	if(vH > 1) {
		vH -= 1;
	}

	if((6 * vH) < 1) {
		return (v1 + (v2 - v1) * 6 * vH);
	}

	if((2 * vH) < 1) {
		return v2;
	}

	if((3 * vH) < 2) {
		return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);
	}

	return v1;
}

Color* color_hsla(Color *clr, float h, float s, float l, float a) {
	if(s == 0) {
		clr->r = clr->g = clr->b = l;
	} else {
		float v1, v2;
		h = fmod(h, 1.0);

		if(l < 0.5) {
			v2 = l * (1.0 + s);
		} else {
			v2 = l + s - s * l;
		}

		v1 = 2.0 * l - v2;

		clr->r = hue_to_rgb(v1, v2, h + (1.0/3.0));
		clr->g = hue_to_rgb(v1, v2, h);
		clr->b = hue_to_rgb(v1, v2, h - (1.0/3.0));
	}

	clr->a = a;
	return clr;
}

void color_get_hsl(const Color *c, float *out_h, float *out_s, float *out_l) {
	float r = clamp(c->r, 0, 1);
	float g = clamp(c->g, 0, 1);
	float b = clamp(c->b, 0, 1);

	float maxv = max(max(r, g), b);
	float minv = min(min(r, g), b);
	float h = 0, s = 0, d = maxv - minv, l = (maxv + minv) / 2;

	if(maxv != minv) {
		s = l > 0.5 ? d / (2 - maxv - minv) : d / (maxv + minv);
		if(maxv == r) {
			h = (g - b) / d + (g < b ? 6 : 0);
		} else if(maxv == g) {
			h = (b - r) / d + 2;
		} else if(maxv == b) {
			h = (r - g) / d + 4;
		}
		h /= 6;
	}

	if(out_h) *out_h = h;
	if(out_s) *out_s = s;
	if(out_l) *out_l = l;
}

Color* color_set_opacity(Color *clr, float opacity) {
	// FIXME: is this correct?

	if(clr->a != 0) {
		opacity /= clr->a;
	}

	color_mul_scalar(clr, opacity);
	return clr;
}

bool color_equals(const Color *clr, const Color *clr2) {
	return (
		clr->r == clr2->r &&
		clr->g == clr2->g &&
		clr->b == clr2->b &&
		clr->a == clr2->a
	);
}

char *color_str(const Color *clr) {
	return strfmt(
		"RGBA(%f, %f, %f, %f) at %p",
		clr->r,
		clr->g,
		clr->b,
		clr->a,
		(void*)clr
	);
}
