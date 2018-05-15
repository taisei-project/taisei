/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
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
