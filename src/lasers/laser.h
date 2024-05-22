/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "color.h"
#include "coroutine/taskdsl.h"
#include "entity.h"

typedef cmplx LaserRuleFunc(Laser *p, real t, void *ruledata);

typedef struct LaserRule {
	LaserRuleFunc *func;
	alignas(cmplx) char data[sizeof(cmplx) * 5];
} LaserRule;

typedef LIST_ANCHOR(Laser) LaserList;

typedef struct LaserBBox {
	FloatOffset top_left, bottom_right;
} LaserBBox;

DEFINE_ENTITY_TYPE(Laser, {
	cmplx pos;
	LaserRule rule;

	struct {
		int segments_ofs;
		int num_segments;
		LaserBBox bbox;
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

void lasers_init(void);
void lasers_shutdown(void);

void delete_lasers(void);
void process_lasers(void);

Laser *create_laserline(cmplx pos, cmplx dir, float charge, float dur, const Color *clr);
Laser *create_laserline_ab(cmplx a, cmplx b, float width, float charge, float dur, const Color *clr);
void laserline_set_ab(Laser *l, cmplx a, cmplx b);
void laserline_set_posdir(Laser *l, cmplx pos, cmplx dir);

Laser *create_laser(cmplx pos, float time, float deathtime, const Color *color, LaserRule rule);

bool laser_is_active(Laser *l);
bool laser_is_clearable(Laser *l);
bool clear_laser(Laser *l, uint flags);

void laser_make_static(Laser *l);
void laser_charge(Laser *l, int t, float charge, float width);

bool laser_intersects_circle(Laser *l, Circle circle);
bool laser_intersects_ellipse(Laser *l, Ellipse ellipse);

typedef struct LaserSegment {
	union {
		struct { cmplxf a, b; } pos;
		float attr0[4];
	};

	union {
		struct {
			struct { float a, b; } width;
			struct { float a, b; } time;
		};
		float attr1[4];
	};
} LaserSegment;

typedef struct LaserTraceSample {
	const LaserSegment *segment;
	cmplx pos;
	float segment_param;
	bool discontinuous;
} LaserTraceSample;

typedef void *(*LaserTraceFunc)(Laser *l, const LaserTraceSample *sample, void *userdata);
void *laser_trace(Laser *l, real step, LaserTraceFunc trace, void *userdata);

INLINE cmplx laser_pos_at(Laser *l, real t) {
	return l->rule.func(l, t, &l->rule.data);
}

DECLARE_EXTERN_TASK(laser_charge, {
	BoxedLaser laser;
	float charge_delay;
	float target_width;
});

#include "rules.h"  // IWYU pragma: export
