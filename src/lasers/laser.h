/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "draw.h"

#include "util.h"
#include "projectile.h"
#include "resource/shader_program.h"
#include "entity.h"

typedef LIST_ANCHOR(Laser) LaserList;

typedef cmplx (*LaserPosRule)(Laser* l, float time);

DEFINE_ENTITY_TYPE(Laser, {
	cmplx pos;
	cmplx args[4];

	LaserPosRule prule;

	struct {
		int segments_ofs;
		int num_segments;
		struct {
			FloatOffset top_left, bottom_right;
		} bbox;
	} _internal;

	Color color;

	int birthtime;
	int next_graze;
	float timespan;
	float deathtime;
	float timeshift;
	float speed;
	float width;
	float width_exponent;
	uint clear_flags;

	uchar unclearable : 1;
	uchar collision_active : 1;
});

#define create_lasercurve1c(p, time, deathtime, clr, rule, a0) create_laser(p, time, deathtime, clr, rule, a0, 0, 0, 0)
#define create_lasercurve2c(p, time, deathtime, clr, rule, a0, a1) create_laser(p, time, deathtime, clr, rule, a0, a1, 0, 0)
#define create_lasercurve3c(p, time, deathtime, clr, rule, a0, a1, a2) create_laser(p, time, deathtime, clr, rule, a0, a1, a2, 0)
#define create_lasercurve4c(p, time, deathtime, clr, rule, a0, a1, a2, a3) create_laser(p, time, deathtime, clr, rule, a0, a1, a2, a3)

void lasers_init(void);
void lasers_shutdown(void);

void delete_lasers(void);
void process_lasers(void);

Laser *create_laserline(cmplx pos, cmplx dir, float charge, float dur, const Color *clr);
Laser *create_laserline_ab(cmplx a, cmplx b, float width, float charge, float dur, const Color *clr);
void laserline_set_ab(Laser *l, cmplx a, cmplx b);
void laserline_set_posdir(Laser *l, cmplx pos, cmplx dir);

Laser *create_laser(cmplx pos, float time, float deathtime, const Color *color, LaserPosRule prule, cmplx a0, cmplx a1, cmplx a2, cmplx a3);

bool laser_is_active(Laser *l);
bool laser_is_clearable(Laser *l);
bool clear_laser(Laser *l, uint flags);

cmplx las_linear(Laser *l, float t);
cmplx las_accel(Laser *l, float t);
cmplx las_sine(Laser *l, float t);
cmplx las_weird_sine(Laser *l, float t);
cmplx las_sine_expanding(Laser *l, float t);
cmplx las_turning(Laser *l, float t);
cmplx las_circle(Laser *l, float t);

void laser_make_static(Laser *l);
void laser_charge(Laser *l, int t, float charge, float width);

bool laser_intersects_circle(Laser *l, Circle circle);
bool laser_intersects_ellipse(Laser *l, Ellipse ellipse);

DECLARE_EXTERN_TASK(laser_charge, {
	BoxedLaser laser;
	float charge_delay;
	float target_width;
});
