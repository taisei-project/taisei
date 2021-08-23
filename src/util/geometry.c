/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "geometry.h"
#include "miscmath.h"

static inline void ellipse_bbox(const Ellipse *e, Rect *r) {
	float largest_radius = fmax(creal(e->axes), cimag(e->axes)) * 0.5;
	r->top_left     = e->origin - largest_radius - I * largest_radius;
	r->bottom_right = e->origin + largest_radius + I * largest_radius;
}

bool point_in_ellipse(cmplx p, Ellipse e) {
	double Xp = creal(p);
	double Yp = cimag(p);
	double Xe = creal(e.origin);
	double Ye = cimag(e.origin);
	double a = e.angle;

	Rect e_bbox;
	ellipse_bbox(&e, &e_bbox);

	return point_in_rect(p, e_bbox) && (
		pow(cos(a) * (Xp - Xe) + sin(a) * (Yp - Ye), 2) / pow(creal(e.axes)/2, 2) +
		pow(sin(a) * (Xp - Xe) - cos(a) * (Yp - Ye), 2) / pow(cimag(e.axes)/2, 2)
	) <= 1;
}

// If segment_ellipse_nonintersection_heuristic returns true, then the
// segment and ellipse do not intersect. However, **the converse is not true**.
// Used for quick returning false in real intersection functions.
static bool segment_ellipse_nonintersection_heuristic(LineSegment seg, Ellipse e) {
	Rect seg_bbox = {
		.top_left = fmin(creal(seg.a), creal(seg.b)) + I * fmin(cimag(seg.a), cimag(seg.b)),
		.bottom_right = fmax(creal(seg.a), creal(seg.b)) + I * fmax(cimag(seg.a), cimag(seg.b))
	};

	Rect e_bbox;
	ellipse_bbox(&e, &e_bbox);

	return !rect_rect_intersect(seg_bbox, e_bbox, true, true);
}

static double lineseg_closest_factor_impl(cmplx m, cmplx v) {
	// m == vector from A to B
	// v == vector from point of interest to A

	double lm2 = cabs2(m);
	assume(lm2 > 0);

	double f = -creal(v * conj(m)) / lm2; // project v onto the line
	f = clamp(f, 0, 1);                   // restrict it to segment

	return f;
}

// Return f such that a + f * (b - a) is the closest point on segment to p
double lineseg_closest_factor(LineSegment seg, cmplx p) {
	if(UNLIKELY(seg.a == seg.b)) {
		return 0;
	}

	return lineseg_closest_factor_impl(seg.b - seg.a, seg.a - p);
}

cmplx lineseg_closest_point(LineSegment seg, cmplx p) {
	if(UNLIKELY(seg.a == seg.b)) {
		return seg.a;
	}

	return clerp(seg.a, seg.b, lineseg_closest_factor_impl(seg.b - seg.a, seg.a - p));
}

// Is the point of shortest distance between the line through a and b
// and a point c between a and b and closer than r?
// if yes, return f so that a+f*(b-a) is that point.
// otherwise return -1.
static double lineseg_circle_intersect_fallback(LineSegment seg, Circle c) {
	double rad2 = c.radius * c.radius;

	if(UNLIKELY(seg.a == seg.b)) {
		if(cabs2(seg.a - c.origin) <= rad2) {
			return 0;
		}

		return -1;
	}

	double f = lineseg_closest_factor_impl(seg.b - seg.a, seg.a - c.origin);
	cmplx p = clerp(seg.a, seg.b, f);

	if(cabs2(p - c.origin) <= rad2) {
		return f;
	}

	return -1;
}

bool lineseg_ellipse_intersect(LineSegment seg, Ellipse e) {
	if(segment_ellipse_nonintersection_heuristic(seg, e)) {
		return false;
	}

	// Transform the coordinate system so that the ellipse becomes a circle
	// with origin at (0, 0) and diameter equal to its X axis. Then we can
	// calculate the segment-circle intersection.

	seg.a -= e.origin;
	seg.b -= e.origin;

	double ratio = creal(e.axes) / cimag(e.axes);
	cmplx rotation = cexp(I * -e.angle);
	seg.a *= rotation;
	seg.b *= rotation;
	seg.a = creal(seg.a) + I * ratio * cimag(seg.a);
	seg.b = creal(seg.b) + I * ratio * cimag(seg.b);

	Circle c = { .radius = creal(e.axes) / 2 };
	return lineseg_circle_intersect_fallback(seg, c) >= 0;
}

double lineseg_circle_intersect(LineSegment seg, Circle c) {
	Ellipse e = { .origin = c.origin, .axes = 2*c.radius + I*2*c.radius };
	if(segment_ellipse_nonintersection_heuristic(seg, e)) {
		return -1;
	}
	return lineseg_circle_intersect_fallback(seg, c);
}

bool point_in_rect(cmplx p, Rect r) {
	return
		creal(p) >= rect_left(r)  &&
		creal(p) <= rect_right(r) &&
		cimag(p) >= rect_top(r)   &&
		cimag(p) <= rect_bottom(r);
}

bool rect_in_rect(Rect inner, Rect outer) {
	return
		rect_left(inner)   >= rect_left(outer)  &&
		rect_right(inner)  <= rect_right(outer) &&
		rect_top(inner)    >= rect_top(outer)   &&
		rect_bottom(inner) <= rect_bottom(outer);
}

bool rect_rect_intersect(Rect r1, Rect r2, bool edges, bool corners) {
	if(
		rect_bottom(r1) < rect_top(r2)    ||
		rect_top(r1)    > rect_bottom(r2) ||
		rect_left(r1)   > rect_right(r2)  ||
		rect_right(r1)  < rect_left(r2)
	) {
		// Not even touching
		return false;
	}

	if(!edges && (
		rect_bottom(r1) == rect_top(r2)    ||
		rect_top(r1)    == rect_bottom(r2) ||
		rect_left(r1)   == rect_right(r2)  ||
		rect_right(r1)  == rect_left(r2)
	)) {
		// Discard edge intersects
		return false;
	}

	if(!corners && (
		(rect_left(r1) == rect_right(r2) && rect_bottom(r1) == rect_top(r2)) ||
		(rect_left(r1) == rect_right(r2) && rect_bottom(r2) == rect_top(r1)) ||
		(rect_left(r2) == rect_right(r1) && rect_bottom(r1) == rect_top(r2)) ||
		(rect_left(r2) == rect_right(r1) && rect_bottom(r2) == rect_top(r1))
	)) {
		// Discard corner intersects
		return false;
	}

	return true;
}

bool rect_rect_intersection(Rect r1, Rect r2, bool edges, bool corners, Rect *out) {
	if(!rect_rect_intersect(r1, r2, edges, corners)) {
		return false;
	}

	out->top_left = CMPLX(
		fmax(rect_left(r1), rect_left(r2)),
		fmax(rect_top(r1), rect_top(r2))
	);

	out->bottom_right = CMPLX(
		fmin(rect_right(r1), rect_right(r2)),
		fmin(rect_bottom(r1), rect_bottom(r2))
	);

	return true;
}

bool rect_join(Rect *r1, Rect r2) {
	if(rect_in_rect(r2, *r1)) {
		return true;
	}

	if(rect_in_rect(*r1, r2)) {
		memcpy(r1, &r2, sizeof(r2));
		return true;
	}

	if(!rect_rect_intersect(*r1, r2, true, false)) {
		return false;
	}

	if(rect_left(*r1) == rect_left(r2) && rect_right(*r1) == rect_right(r2)) {
		// r2 is directly above/below r1
		double y_min = fmin(rect_top(*r1), rect_top(r2));
		double y_max = fmax(rect_bottom(*r1), rect_bottom(r2));

		r1->top_left = CMPLX(creal(r1->top_left), y_min);
		r1->bottom_right = CMPLX(creal(r1->bottom_right), y_max);

		return true;
	}

	if(rect_top(*r1) == rect_top(r2) && rect_bottom(*r1) == rect_bottom(r2)) {
		// r2 is directly left/right to r1
		double x_min = fmin(rect_left(*r1), rect_left(r2));
		double x_max = fmax(rect_right(*r1), rect_right(r2));

		r1->top_left = CMPLX(x_min, cimag(r1->top_left));
		r1->bottom_right = CMPLX(x_max, cimag(r1->bottom_right));

		return true;
	}

	return false;
}

void rect_set_xywh(Rect *rect, double x, double y, double w, double h) {
	rect->top_left = CMPLX(x, y);
	rect->bottom_right = CMPLX(w, h) + rect->top_left;
}
