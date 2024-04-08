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

typedef cmplx LaserRuleFunc(Laser *p, real t, void *ruledata);

typedef struct LaserRule {
	LaserRuleFunc *func;
	alignas(cmplx) char data[sizeof(cmplx) * 5];
} LaserRule;

typedef cmplx (*LegacyLaserPosRule)(Laser *l, float time);
typedef LegacyLaserPosRule LaserPosRule
	attr_deprecated("For compatibility with legacy rules");

typedef LIST_ANCHOR(Laser) LaserList;

DEFINE_ENTITY_TYPE(Laser, {
	cmplx pos;

	union {
		LaserRule rule;
		struct {
			char _padding[offsetof(LaserRule, data)];
			cmplx args[(sizeof(LaserRule) - offsetof(LaserRule, data)) / sizeof(cmplx)]
				attr_deprecated("For compatibility with legacy rules");
		};
	};

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

#define create_lasercurve1c(p, time, deathtime, clr, rule, a0) \
	create_laser(p, time, deathtime, clr, laser_rule_compat(rule, a0, 0, 0, 0))
#define create_lasercurve2c(p, time, deathtime, clr, rule, a0, a1) \
	create_laser(p, time, deathtime, clr, laser_rule_compat(rule, a0, a1, 0, 0))
#define create_lasercurve3c(p, time, deathtime, clr, rule, a0, a1, a2) \
	create_laser(p, time, deathtime, clr, laser_rule_compat(rule, a0, a1, a2, 0))
#define create_lasercurve4c(p, time, deathtime, clr, rule, a0, a1, a2, a3) \
	create_laser(p, time, deathtime, clr, laser_rule_compat(rule, a0, a1, a2, a3))

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

#include "rules.h"
