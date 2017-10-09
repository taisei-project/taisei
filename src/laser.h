/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "util.h"
#include "projectile.h"
#include "resource/shader.h"

typedef struct Laser Laser;

typedef complex (*LaserPosRule)(Laser* l, float time);
typedef void (*LaserLogicRule)(Laser* l, int time);

struct Laser {
	struct Laser *next;
	struct Laser *prev;

	complex pos;

	Color color;

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

	int in_background;

	complex args[4];

	bool unclearable;
	bool dead;
};

#define create_lasercurve1c(p, time, deathtime, clr, rule, a0) create_laser(p, time, deathtime, clr, rule, 0, a0, 0, 0, 0)
#define create_lasercurve2c(p, time, deathtime, clr, rule, a0, a1) create_laser(p, time, deathtime, clr, rule, 0, a0, a1, 0, 0)
#define create_lasercurve3c(p, time, deathtime, clr, rule, a0, a1, a2) create_laser(p, time, deathtime, clr, rule, 0, a0, a1, a2, 0)
#define create_lasercurve4c(p, time, deathtime, clr, rule, a0, a1, a2, a3) create_laser(p, time, deathtime, clr, rule, 0, a0, a1, a2, a3)


Laser *create_laserline(complex pos, complex dir, float charge, float dur, Color clr);
Laser *create_laserline_ab(complex a, complex b, float width, float charge, float dur, Color clr);

Laser *create_laser(complex pos, float time, float deathtime, Color color, LaserPosRule prule, LaserLogicRule lrule, complex a0, complex a1, complex a2, complex a3);
void draw_lasers(int);
void delete_lasers(void);
void process_lasers(void);

int collision_laser_line(Laser *l);
int collision_laser_curve(Laser *l);

complex las_linear(Laser *l, float t);
complex las_accel(Laser *l, float t);
complex las_sine(Laser *l, float t);
complex las_sine_expanding(Laser *l, float t);
complex las_turning(Laser *l, float t);
complex las_circle(Laser *l, float t);

float laser_charge(Laser *l, int t, float charge, float width);
void static_laser(Laser *l, int t);
