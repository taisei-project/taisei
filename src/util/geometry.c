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

Rect ellipse_bbox(Ellipse e) {
	double largest_radius = fmax(re(e.axes), im(e.axes)) * 0.5;
	cmplx d = CMPLX(largest_radius, largest_radius);
	return (Rect) {
		.top_left     = e.origin - d,
		.bottom_right = e.origin + d,
	};
}

bool point_in_ellipse(cmplx p, Ellipse e) {
	double Xp = re(p);
	double Yp = im(p);
	double Xe = re(e.origin);
	double Ye = im(e.origin);
	double a = e.angle;

	Rect e_bbox = ellipse_bbox(e);

	return point_in_rect(p, e_bbox) && (
		pow(cos(a) * (Xp - Xe) + sin(a) * (Yp - Ye), 2) / pow(re(e.axes)/2, 2) +
		pow(sin(a) * (Xp - Xe) - cos(a) * (Yp - Ye), 2) / pow(im(e.axes)/2, 2)
	) <= 1;
}

Rect lineseg_bbox(LineSegment seg) {
	return (Rect) {
		.top_left     = fmin(re(seg.a), re(seg.b)) + I * fmin(im(seg.a), im(seg.b)),
		.bottom_right = fmax(re(seg.a), re(seg.b)) + I * fmax(im(seg.a), im(seg.b))
	};
}

// If segment_ellipse_nonintersection_heuristic returns true, then the
// segment and ellipse do not intersect. However, **the converse is not true**.
// Used for quick returning false in real intersection functions.
static bool segment_ellipse_nonintersection_heuristic(LineSegment seg, Ellipse e) {
	Rect seg_bbox = lineseg_bbox(seg);
	Rect e_bbox = ellipse_bbox(e);
	return !rect_rect_intersect(seg_bbox, e_bbox, true, true);
}

static double lineseg_closest_factor_impl(cmplx m, cmplx v) {
	// m == vector from A to B
	// v == vector from point of interest to A

	double lm2 = cabs2(m);

	if(UNLIKELY(lm2 == 0)) {
		return 0;
	}

	double f = -re(v * conj(m)) / lm2; // project v onto the line
	f = clamp(f, 0, 1);                   // restrict it to segment

	return f;
}

// Return f such that a + f * (b - a) is the closest point on segment to p
double lineseg_closest_factor(LineSegment seg, cmplx p) {
	return lineseg_closest_factor_impl(seg.b - seg.a, seg.a - p);
}

cmplx lineseg_closest_point(LineSegment seg, cmplx p) {
	return clerp(seg.a, seg.b, lineseg_closest_factor_impl(seg.b - seg.a, seg.a - p));
}

// Is the point of shortest distance between the line through a and b
// and a point c between a and b and closer than r?
// if yes, return f so that a+f*(b-a) is that point.
// otherwise return -1.
static double lineseg_circle_intersect_fallback(LineSegment seg, Circle c) {
	double rad2 = c.radius * c.radius;

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

	double ratio = re(e.axes) / im(e.axes);
	cmplx rotation = cexp(I * -e.angle);
	seg.a *= rotation;
	seg.b *= rotation;
	seg.a = re(seg.a) + I * ratio * im(seg.a);
	seg.b = re(seg.b) + I * ratio * im(seg.b);

	Circle c = { .radius = re(e.axes) / 2 };
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
		re(p) >= rect_left(r)  &&
		re(p) <= rect_right(r) &&
		im(p) >= rect_top(r)   &&
		im(p) <= rect_bottom(r);
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

		r1->top_left = CMPLX(re(r1->top_left), y_min);
		r1->bottom_right = CMPLX(re(r1->bottom_right), y_max);

		return true;
	}

	if(rect_top(*r1) == rect_top(r2) && rect_bottom(*r1) == rect_bottom(r2)) {
		// r2 is directly left/right to r1
		double x_min = fmin(rect_left(*r1), rect_left(r2));
		double x_max = fmax(rect_right(*r1), rect_right(r2));

		r1->top_left = CMPLX(x_min, im(r1->top_left));
		r1->bottom_right = CMPLX(x_max, im(r1->bottom_right));

		return true;
	}

	return false;
}

void rect_set_xywh(Rect *rect, double x, double y, double w, double h) {
	rect->top_left = CMPLX(x, y);
	rect->bottom_right = CMPLX(w, h) + rect->top_left;
}

double ucapsule_dist_from_point(cmplx p, UnevenCapsule ucap) {
	assert(ucap.radius.b >= ucap.radius.a);

	p -= ucap.pos.a;
	ucap.pos.b -= ucap.pos.a;
	double h = cabs2(ucap.pos.b);
	cmplx q = CMPLX(cdot(p, conj(cswap(ucap.pos.b))), cdot(p, ucap.pos.b)) / h;

	q = CMPLX(fabs(re(q)), im(q));

	double b = ucap.radius.a - ucap.radius.b;
	cmplx c = CMPLX(sqrt(h - b * b), b);

	double k = ccross(c, q);
	double m = cdot(c, q);
	double n = cabs2(q);

	if(k < 0) {
		return sqrt(h * n) - ucap.radius.a;
	}

	if(k > re(c)) {
		return sqrt(h * (n + 1 - 2 * im(q))) - ucap.radius.b;
	}

	return m - ucap.radius.a;
}

bool lineseg_lineseg_intersection(LineSegment seg0, LineSegment seg1, cmplx *out) {
	// Based on an answer from https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect

	double p0_x = re(seg0.a);
	double p0_y = im(seg0.a);

	double p1_x = re(seg0.b);
	double p1_y = im(seg0.b);

	double p2_x = re(seg1.a);
	double p2_y = im(seg1.a);

	double p3_x = re(seg1.b);
	double p3_y = im(seg1.b);

	double s1_x = p1_x - p0_x;
	double s1_y = p1_y - p0_y;

	double s2_x = p3_x - p2_x;
	double s2_y = p3_y - p2_y;

	double d = -s2_x * s1_y + s1_x * s2_y;

	if(d == 0) {
		// NOTE: parallel or colinear.
		// In the colinear case, the intersection may be another line segment.
		// For our purposes, ignoring it is probably fine.
		return false;
	}

	double s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / d;

	if(s < 0 || s > 1) {
		return false;
	}

	double t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / d;

	if(t < 0 || t > 1) {
		return false;
	}

	if(out) {
		*out = CMPLX(p0_x + (t * s1_x), p0_y + (t * s1_y));
	}

	return true;
}
