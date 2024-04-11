/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "laser.h"
#include "ringbuf.h"

cmplx las_linear(Laser *l, float t) attr_deprecated("Use laser_rule_linear");
cmplx las_accel(Laser *l, float t) attr_deprecated("Use laser_rule_accelerated");
cmplx las_sine(Laser *l, float t) attr_deprecated("Use laser_rule_sine");
cmplx las_sine_expanding(Laser *l, float t) attr_deprecated("Use laser_rule_sine_expanding");
cmplx las_turning(Laser *l, float t) attr_deprecated("No replacement");
cmplx las_circle(Laser *l, float t) attr_deprecated("Use laser_rule_arc");

extern LaserRuleFunc laser_rule_compat_adapter;

typedef struct LaserRuleLinearData {
	cmplx velocity;
} LaserRuleLinearData;

LaserRule laser_rule_linear(cmplx velocity);
LaserRuleLinearData *laser_get_ruledata_linear(Laser *l);

typedef struct LaserRuleAcceleratedData {
	cmplx velocity;
	cmplx half_accel;
} LaserRuleAcceleratedData;

LaserRule laser_rule_accelerated(cmplx velocity, cmplx accel);
LaserRuleAcceleratedData *laser_get_ruledata_accelerated(Laser *l);

typedef struct LaserRuleSineData {
	cmplx velocity;
	cmplx amplitude;
	real frequency;
	real phase;
} LaserRuleSineData;

LaserRule laser_rule_sine(cmplx velocity, cmplx amplitude, real frequency, real phase);
LaserRuleSineData *laser_get_ruledata_sine(Laser *l);

typedef struct LaserRuleSineExpandingData {
	cmplx velocity;
	real amplitude;
	real frequency;
	real phase;
} LaserRuleSineExpandingData;

LaserRule laser_rule_sine_expanding(cmplx velocity, real amplitude, real frequency, real phase);
LaserRuleSineExpandingData *laser_get_ruledata_sine_expanding(Laser *l);

typedef struct LaserRuleArcData {
	cmplx radius;
	real turn_speed;
	real time_ofs;
} LaserRuleArcData;

LaserRule laser_rule_arc(cmplx radius, real turnspeed, real timeofs);
LaserRuleArcData *laser_get_ruledata_arc(Laser *l);

typedef struct LaserRuleCompatData {
	cmplx args[4];
	LegacyLaserPosRule oldrule;
} LaserRuleCompatData;

LaserRule laser_rule_compat(LegacyLaserPosRule oldrule, cmplx a0, cmplx a1, cmplx a2, cmplx a3)
	attr_deprecated("Use the new rule format (see laser/rules.h)");
LaserRuleCompatData *laser_get_ruledata_compat(Laser *l);

typedef struct LaserRuleDynamicTaskData {
	MoveParams move;
	RING_BUFFER(cmplx) history;
} LaserRuleDynamicTaskData;

typedef struct LaserRuleDynamicData {
	const BoxedTask control_task;
	LaserRuleDynamicTaskData *const task_data;
} LaserRuleDynamicData;

Laser *create_dynamic_laser(cmplx pos, float time, float deathtime, const Color *color);
LaserRuleDynamicData *laser_get_ruledata_dynamic(Laser *l);

#define MAKE_LASER_RULE(func, data) ({ \
    union { \
		LaserRule r; \
		struct { \
			LaserRuleFunc *f; \
			typeof(data) _data; \
		} src; \
	} cvt = { \
        .src.f = func, \
        .src._data = data, \
    }; \
    static_assert(sizeof(cvt) == sizeof(LaserRule)); \
    cvt.r; \
})

#define GET_LASER_RULEDATA(_laser, _func, _data_type) ( \
	(_laser)->rule.func == _func \
		? CASTPTR_ASSUME_ALIGNED(&(_laser)->rule.data, _data_type) \
		: NULL)

#define IMPL_LASER_RULE_DATAGETTER(_getter_func, _rule_func, _data_type) \
	_data_type *_getter_func(Laser *l) { \
		return GET_LASER_RULEDATA(l, _rule_func, _data_type); \
	}
