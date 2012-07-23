/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef LASER_H
#define LASER_H

#include <complex.h>
#include "projectile.h"

typedef enum {
	LT_Line,
	LT_Curve
} LaserType;

struct Laser;

typedef complex (*LaserRule)(struct Laser*, float time);

typedef struct Laser {
	struct Laser *next;
	struct Laser *prev;
	
	LaserType type;
	
	complex pos;
	complex dir; // if type == LaserLine, carg(pos0) is orientation and cabs(pos0) width
	
	Color *color;
	
	int birthtime;
	int time; // line: startup time; curve: length
	int deathtime;
	
	LaserRule rule;
	complex args[4];
} Laser;

#define create_lasercurve1c(p, time, deathtime, clr, rule, a0) create_laser_p(LT_Curve, p, 0, time, deathtime, clr, rule, a0, 0, 0, 0)
#define create_lasercurve2c(p, time, deathtime, clr, rule, a0, a1) create_laser_p(LT_Curve, p, 0, time, deathtime, clr, rule, a0, a1, 0, 0)
#define create_lasercurve3c(p, time, deathtime, clr, rule, a0, a1, a2) create_laser_p(LT_Curve, p, 0, time, deathtime, clr, rule, a0, a1, a2, 0)
#define create_lasercurve4c(p, time, deathtime, clr, rule, a0, a1, a2, a3) create_laser_p(LT_Curve, p, 0, time, deathtime, clr, rule, a0, a1, a2, a3)

#define create_laserline1c(p, prop, time, deathtime, clr, rule, a0) create_laser_p(LT_Line, p, prop, time, deathtime, clr, rule, a0, 0, 0, 0)
#define create_laserline2c(p, prop, time, deathtime, clr, rule, a0, a1) create_laser_p(LT_Line, p, prop, time, deathtime, clr, rule, a0, a1, 0, 0)
#define create_laserline3c(p, prop, time, deathtime, clr, rule, a0, a1, a2) create_laser_p(LT_Line, p, prop, time, deathtime, clr, rule, a0, a1, a2, 0)
#define create_laserline4c(p, prop, time, deathtime, clr, rule, a0, a1, a2, a3) create_laser_p(LT_Line, p, prop, time, deathtime, clr, rule, a0, a1, a2, a3)

Laser *create_laser_p(LaserType type, complex pos, complex dir, int time, int deathtime, Color *color, LaserRule rule, complex a0, complex a1, complex a2, complex a3);
void draw_lasers();
void delete_lasers();
void process_lasers();

int collision_laser_line(Laser *l);
int collision_laser_curve(Laser *l);

complex las_linear(Laser *l, float t);
complex las_accel(Laser *l, float t);

#endif