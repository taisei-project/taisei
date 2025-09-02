/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource/font.h"

void set_ortho(float w, float h);
void colorfill(float r, float g, float b, float a);
void fade_out(float f);

typedef struct DrawFragmentsParams {
	Sprite *fill;

	struct {
		const Color *fill;
		const Color *back;
		const Color *frag;
	} color;

	struct {
		float x, y;
	} pos;

	struct {
		float x, y;
	} origin_offset;

	struct {
		int elements;
		int fragments;
	} limits;

	struct {
		int elements;
		int fragments;
	} filled;

	float alpha;
	float spacing;
} DrawFragmentsParams;

void draw_fragments(const DrawFragmentsParams *params);
double draw_fraction(double value, Alignment a, double pos_x, double pos_y, Font *f_int, Font *f_fract, const Color *c_int, const Color *c_fract, bool zero_pad);
void draw_framebuffer_tex(Framebuffer *fb, double width, double height);
void draw_framebuffer_attachment(Framebuffer *fb, double width, double height, FramebufferAttachment attachment);
