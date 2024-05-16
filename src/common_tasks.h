/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "coroutine.h"
#include "item.h"
#include "move.h"
#include "entity.h"
#include "global.h"
#include "util/glm.h"

DECLARE_EXTERN_TASK(
	common_drop_items,
	{ cmplx *pos; ItemCounts items; }
);

DECLARE_EXTERN_TASK(
	common_move,
	{ cmplx *pos; MoveParams move_params; BoxedEntity ent; }
);

DECLARE_EXTERN_TASK(
	common_move_ext,
	{ cmplx *pos; MoveParams *move_params; BoxedEntity ent; }
);

DECLARE_EXTERN_TASK(
	common_call_func,
	{ void (*func)(void); }
);

DECLARE_EXTERN_TASK(
	common_start_bgm,
	{ const char *bgm; }
);

#define COMMON_CHARGE_SOUND_CHARGE "charge_generic"
#define COMMON_CHARGE_SOUND_DISCHARGE "discharge"
#define COMMON_CHARGE_SOUNDS { COMMON_CHARGE_SOUND_CHARGE, COMMON_CHARGE_SOUND_DISCHARGE }

DECLARE_EXTERN_TASK(
	common_charge,
	{
		cmplx pos;
		const Color *color;
		int time;
		BoxedEntity bind_to_entity;
		const cmplx *anchor;
		const Color *color_ref;

		struct {
			const char *charge;
			const char *discharge;
		} sound;
	}
);

void common_drop_items(cmplx pos, const ItemCounts *items)
	attr_nonnull(2);

int common_charge(int time, const cmplx *anchor, cmplx offset, const Color *color)
	attr_nonnull(2, 4);

int common_charge_static(int time, cmplx pos, const Color *color)
	attr_nonnull(3);

int common_charge_custom(
	int time,
	const cmplx *anchor,
	cmplx offset,
	const Color *color,
	const char *snd_charge,
	const char *snd_discharge
) attr_nonnull(4);

void common_move_loop(cmplx *restrict pos, MoveParams *restrict mp);

INLINE Rect viewport_bounds(double margin) {
	return (Rect) {
		.top_left = CMPLX(margin, margin),
		.bottom_right = CMPLX(VIEWPORT_W - margin, VIEWPORT_H - margin),
	};
}

cmplx common_wander(cmplx origin, double dist, Rect bounds);
void common_rotate_velocity(MoveParams *move, real angle, int duration);

DECLARE_EXTERN_TASK(
	common_set_bitflags,
	{
		uint *pflags;
		uint mask;
		uint set;
	}
);

DECLARE_EXTERN_TASK(
	common_kill_projectile,
	{
		BoxedProjectile proj;
	}
);

DECLARE_EXTERN_TASK(
	common_kill_enemy,
	{
		BoxedEnemy enemy;
	}
);

DECLARE_EXTERN_TASK(
	common_easing_animate,
	{
		float *value;
		float to;
		int duration;
		glm_ease_t ease;
	}
);

DECLARE_EXTERN_TASK(
	common_easing_animate_vec3,
	{
		vec3 *value;
		vec3 to;
		int duration;
		glm_ease_t ease;
	}
);

DECLARE_EXTERN_TASK(
	common_easing_animate_vec4,
	{
		vec4 *value;
		vec4 to;
		int duration;
		glm_ease_t ease;
	}
);

DECLARE_EXTERN_TASK(
	common_rotate_velocity,
	{
		MoveParams *move;
		real angle;
		int duration;
	}
);

DECLARE_EXTERN_TASK(
	common_easing_animated,
	{
		double *value;
		double to;
		int duration;
		glm_ease_t ease;
	}
);

DECLARE_EXTERN_TASK(common_play_sfx, { const char *name; });

// TODO: move this elsewhere?

typedef struct RadialLoop {
	cmplx dir, turn;
	int cnt, i;
} RadialLoop;

INLINE RadialLoop _radial_loop_init(int cnt, cmplx dir) {
	return (RadialLoop) {
		.dir = dir,
		.cnt = abs(cnt),
		.turn = cdir(M_TAU/cnt),
	};
}

#define RADIAL_LOOP(_loop_var, _cnt_init, _dir_init) \
	for( \
		RadialLoop _loop_var = _radial_loop_init(_cnt_init, _dir_init); \
		_loop_var.i < _loop_var.cnt; \
		++_loop_var.i, _loop_var.dir *= _loop_var.turn)
