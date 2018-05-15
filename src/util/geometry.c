/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "geometry.h"

bool point_in_ellipse(complex p, Ellipse e) {
	double Xp = creal(p);
	double Yp = cimag(p);
	double Xe = creal(e.origin);
	double Ye = cimag(e.origin);
	double a = e.angle;

	return (
		pow(cos(a) * (Xp - Xe) + sin(a) * (Yp - Ye), 2) / pow(creal(e.axes)/2, 2) +
		pow(sin(a) * (Xp - Xe) - cos(a) * (Yp - Ye), 2) / pow(cimag(e.axes)/2, 2)
	) <= 1;
}

// Is the point of shortest distance between the line through a and b
// and a point c between a and b and closer than r?
// if yes, return f so that a+f*(b-a) is that point.
// otherwise return -1.
double lineseg_circle_intersect(LineSegment seg, Circle c) {
	complex m, v;
	double projection, lv, lm, distance;

	m = seg.b - seg.a; // vector pointing along the line
	v = seg.a - c.origin; // vector from circle to point A

	lv = cabs(v);
	lm = cabs(m);

	if(lv < c.radius) {
		return 0;
	}

	if(lm == 0) {
		return -1;
	}

	projection = -creal(v*conj(m)) / lm; // project v onto the line

	// now the distance can be calculated by Pythagoras
	distance = sqrt(pow(lv, 2) - pow(projection, 2));

	if(distance <= c.radius) {
		double f = projection/lm;

		if(f >= 0 && f <= 1) { // it’s on the line!
			return f;
		}
	}

	return -1;
}

bool lineseg_ellipse_intersect(LineSegment seg, Ellipse e) {
	// Transform the coordinate system so that the ellipse becomes a circle
	// with origin at (0, 0) and diameter equal to its X axis. Then we can
	// calculate the segment-circle intersection.

	double ratio = creal(e.axes) / cimag(e.axes);
	complex rotation = cexp(I * -e.angle);
	seg.a *= rotation;
	seg.b *= rotation;
	seg.a = creal(seg.a) + I * ratio * cimag(seg.a);
	seg.b = creal(seg.b) + I * ratio * cimag(seg.b);

	Circle c = { .radius = creal(e.axes) / 2 };
	return lineseg_circle_intersect(seg, c) >= 0;
}
