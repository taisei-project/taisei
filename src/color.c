/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <stdio.h>
#include "color.h"

static const float conv = 1.0f / CLR_ONEVALUE;

#ifndef COLOR_INLINE

Color rgba(float r, float g, float b, float a) {
	assert(!r || isnormal(r));
	assert(!g || isnormal(g));
	assert(!b || isnormal(b));
	assert(!a || isnormal(a));

	return RGBA(r, g, b, a);
}

Color rgb(float r, float g, float b) {
	assert(!r || isnormal(r));
	assert(!g || isnormal(g));
	assert(!b || isnormal(b));

	return RGB(r, g, b);
}

#endif

void parse_color(Color clr, float *r, float *g, float *b, float *a) {
	*r = (ColorComponent)((clr >> CLR_R) & CLR_CMASK) * conv;
	*g = (ColorComponent)((clr >> CLR_G) & CLR_CMASK) * conv;
	*b = (ColorComponent)((clr >> CLR_B) & CLR_CMASK) * conv;
	*a = (ColorComponent)((clr >> CLR_A) & CLR_CMASK) * conv;
}

void parse_color_array(Color clr, float array[4]) {
	parse_color(clr, array, array+1, array+2, array+3);
}

Color derive_color(Color src, Color mask, Color mod) {
	return (src & ~mask) | (mod & mask);
}

float color_component(Color clr, uint ofs) {
	return (ColorComponent)((clr >> ofs) & CLR_CMASK) * conv;
}

Color multiply_colors(Color c1, Color c2) {
	float c1a[4], c2a[4];
	parse_color_array(c1, c1a);
	parse_color_array(c2, c2a);
	return rgba(c1a[0]*c2a[0], c1a[1]*c2a[1], c1a[2]*c2a[2], c1a[3]*c2a[3]);
}

Color add_colors(Color c1, Color c2) {
	float c1a[4], c2a[4];
	parse_color_array(c1, c1a);
	parse_color_array(c2, c2a);
	return rgba(c1a[0]+c2a[0], c1a[1]+c2a[1], c1a[2]+c2a[2], c1a[3]+c2a[3]);
}

Color subtract_colors(Color c1, Color c2) {
	float c1a[4], c2a[4];
	parse_color_array(c1, c1a);
	parse_color_array(c2, c2a);
	return rgba(c1a[0]-c2a[0], c1a[1]-c2a[1], c1a[2]-c2a[2], c1a[3]-c2a[3]);
}

Color divide_colors(Color c1, Color c2) {
	float c1a[4], c2a[4];
	parse_color_array(c1, c1a);
	parse_color_array(c2, c2a);
	return rgba(c1a[0]/c2a[0], c1a[1]/c2a[1], c1a[2]/c2a[2], c1a[3]/c2a[3]);
}

Color mix_colors(Color c1, Color c2, double a) {
	float c1a[4], c2a[4];
	double f1 = a;
	double f2 = 1 - f1;
	parse_color_array(c1, c1a);
	parse_color_array(c2, c2a);
	return rgba(f1*c1a[0]+f2*c2a[0], f1*c1a[1]+f2*c2a[1], f1*c1a[2]+f2*c2a[2], f1*c1a[3]+f2*c2a[3]);
}

Color approach_color(Color src, Color dst, double delta) {
	float c1a[4], c2a[4];
	parse_color_array(src, c1a);
	parse_color_array(dst, c2a);
	return rgba(
		c1a[0] + (c2a[0] - c1a[0]) * delta,
		c1a[1] + (c2a[1] - c1a[1]) * delta,
		c1a[2] + (c2a[2] - c1a[2]) * delta,
		c1a[3] + (c2a[3] - c1a[3]) * delta
	);
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

Color hsla(float h, float s, float l, float a) {
	float r, g, b;

	if(s == 0) {
		r = g = b = l;
	} else {
		float v1, v2;
		h = fmod(h, 1.0);

		if(l < 0.5) {
			v2 = l * (1.0 + s);
		} else {
			v2 = l + s - s * l;
		}

		v1 = 2.0 * l - v2;

		r = hue_to_rgb(v1, v2, h + (1.0/3.0));
		g = hue_to_rgb(v1, v2, h);
		b = hue_to_rgb(v1, v2, h - (1.0/3.0));
	}

	return rgba(r, g, b, a);
}

Color hsl(float h, float s, float l) {
	return hsla(h, s, l, 1.0);
}

char* color_str(Color c) {
	float r, g, b, a;
	parse_color(c, &r, &g, &b, &a);
	return strfmt("rgba(%f, %f, %f, %f) 0x%016"PRIxMAX, r, g, b, a, (uintmax_t)c);
}
