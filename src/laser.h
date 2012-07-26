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
#include "resource/shader.h"

typedef struct Laser Laser;

typedef complex (*LaserPosRule)(Laser* l, float time);
typedef void (*LaserLogicRule)(Laser* l, int time);

struct Laser {
	struct Laser *next;
	struct Laser *prev;
	
	complex pos;
	
	Color *color;
	
	int birthtime;
	
	float timespan;
	float deathtime;
	
	float timeshift;
	float speed;
	float width;
		
	Shader *shader;
	
	float collision_step;
	
	LaserPosRule prule;
	LaserLogicRule lrule;
	
	complex args[4];
};

#define create_lasercurve1c(p, time, deathtime, clr, rule, a0) create_laser(p, time, deathtime, clr, rule, 0, a0, 0, 0, 0)
#define create_lasercurve2c(p, time, deathtime, clr, rule, a0, a1) create_laser(p, time, deathtime, clr, rule, 0, a0, a1, 0, 0)
#define create_lasercurve3c(p, time, deathtime, clr, rule, a0, a1, a2) create_laser(p, time, deathtime, clr, rule, 0, a0, a1, a2, 0)
#define create_lasercurve4c(p, time, deathtime, clr, rule, a0, a1, a2, a3) create_laser(p, time, deathtime, clr, rule, 0, a0, a1, a2, a3)

#define create_laserline(pos, dir, charge, dur, clr) create_laserline_ab(pos, (pos)+(dir)*VIEWPORT_H*1.4/cabs(dir), cabs(dir), charge, dur, clr)

Laser *create_laserline_ab(complex a, complex b, float width, float charge, float dur, Color *clr);

Laser *create_laser(complex pos, float time, float deathtime, Color *color, LaserPosRule prule, LaserLogicRule lrule, complex a0, complex a1, complex a2, complex a3);
void draw_lasers();
void delete_lasers();
void process_lasers();

int collision_laser_line(Laser *l);
int collision_laser_curve(Laser *l);

complex las_linear(Laser *l, float t);
complex las_accel(Laser *l, float t);

float laser_charge(Laser *l, int t, float charge, float width);

void static_laser(Laser *l, int t);
#endif