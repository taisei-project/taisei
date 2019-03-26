/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_util_geometry_h
#define IGUARD_util_geometry_h

#include "taisei.h"

typedef struct FloatRect {
	float x;
	float y;
	float w;
	float h;
} FloatRect;

typedef struct IntRect {
	int x;
	int y;
	int w;
	int h;
} IntRect;

typedef struct Ellipse {
	complex origin;
	complex axes; // NOT half-axes!
	double angle;
} Ellipse;

typedef struct LineSegment {
	complex a;
	complex b;
} LineSegment;

typedef struct Circle {
	complex origin;
	double radius;
} Circle;

typedef struct Rect {
	complex top_left;
	complex bottom_right;
} Rect;

bool point_in_ellipse(complex p, Ellipse e) attr_const;
double lineseg_circle_intersect(LineSegment seg, Circle c) attr_const;
bool lineseg_ellipse_intersect(LineSegment seg, Ellipse e) attr_const;

static inline attr_must_inline attr_const
double rect_x(Rect r) {
	return creal(r.top_left);
}

static inline attr_must_inline attr_const
double rect_y(Rect r) {
	return cimag(r.top_left);
}

static inline attr_must_inline attr_const
double rect_width(Rect r) {
	return creal(r.bottom_right) - creal(r.top_left);
}

static inline attr_must_inline attr_const
double rect_height(Rect r) {
	return cimag(r.bottom_right) - cimag(r.top_left);
}

static inline attr_must_inline attr_const
double rect_top(Rect r) {
	return cimag(r.top_left);
}

static inline attr_must_inline attr_const
double rect_bottom(Rect r) {
	return cimag(r.bottom_right);
}

static inline attr_must_inline attr_const
double rect_left(Rect r) {
	return creal(r.top_left);
}

static inline attr_must_inline attr_const
double rect_right(Rect r) {
	return creal(r.bottom_right);
}

static inline attr_must_inline attr_const
double rect_area(Rect r) {
	return rect_width(r) * rect_height(r);
}

static inline attr_must_inline
void rect_move(Rect *r, complex pos) {
	complex vector = pos - r->top_left;
	r->top_left += vector;
	r->bottom_right += vector;
}

bool point_in_rect(complex p, Rect r);
bool rect_in_rect(Rect inner, Rect outer) attr_const;
bool rect_rect_intersect(Rect r1, Rect r2, bool edges, bool corners) attr_const;
bool rect_rect_intersection(Rect r1, Rect r2, bool edges, bool corners, Rect *out) attr_pure attr_nonnull(5);
bool rect_join(Rect *r1, Rect r2) attr_pure attr_nonnull(1);
void rect_set_xywh(Rect *rect, double x, double y, double w, double h) attr_nonnull(1);

#endif // IGUARD_util_geometry_h
