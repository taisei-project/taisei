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
	LaserLine,
	LaserCurve
} LaserType;

struct Laser;

typedef complex (*LaserRule)(struct Laser*, float time);

typedef struct Laser {
	struct Laser *next;
	struct Laser *prev;
	
	LaserType type;
	
	complex pos;
	complex pos0; // if type == LaserLine, carg(pos0) is orientation and cabs(pos0) width
	
	Color *color;
	
	int birthtime;
	int time; // line: startup time; curve: length
	int deathtime;
	
	LaserRule rule;
	complex args[4];
} Laser;

Laser *create_laser(LaserType type, complex pos, complex pos0, int time, int deathtime, Color *color, LaserRule rule, complex args, ...);
void draw_lasers();
void free_lasers();
void process_lasers();

int collision_laser_line(Laser *l);
int collision_laser_curve(Laser *l);

#endif