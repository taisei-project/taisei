/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

/*
 * Simple generalized projectile movement based on laochailan's idea
 */

typedef struct MoveParams {
	cmplx velocity, acceleration, retention;
	cmplx attraction;
	cmplx attraction_point;
	real attraction_exponent;
} MoveParams;

cmplx move_update(cmplx *restrict pos, MoveParams *restrict params) attr_hot attr_nonnull_all;
cmplx move_update_multiple(uint times, cmplx *restrict pos, MoveParams *restrict params);

INLINE MoveParams move_next(cmplx pos, MoveParams move) {
	move_update(&pos, &move);
	return move;
}

INLINE MoveParams move_linear(cmplx vel) {
	return (MoveParams) { vel, 0, 1 };
}

INLINE MoveParams move_accelerated(cmplx vel, cmplx accel) {
	return (MoveParams) { vel, accel, 1 };
}

INLINE MoveParams move_asymptotic(cmplx vel0, cmplx vel1, cmplx retention) {
	return (MoveParams) { vel0, vel1 * (1 - retention), retention };
}

INLINE MoveParams move_asymptotic_halflife(cmplx vel0, cmplx vel1, double halflife) {
	return move_asymptotic(vel0, vel1, exp2(-1.0 / halflife));
}

INLINE MoveParams move_asymptotic_simple(cmplx vel, double boost_factor) {
	// NOTE: this matches the old asymptotic rule semantics exactly
	double retention = 0.8;
	return move_asymptotic(vel * (1 + boost_factor), vel, retention);
}

INLINE MoveParams move_towards(cmplx vel, cmplx target, cmplx attraction) {
	return (MoveParams) {
		.velocity = vel,
		.attraction = attraction,
		.attraction_point = target,
		.attraction_exponent = 1
	};
}

#define _move_towards_nargs3 move_towards

attr_deprecated(
	"Use move_towards(velocity, target, attraction) or "
	"move_from_towards(origin, target, attraction) instead")
INLINE MoveParams _move_towards_nargs2(cmplx target, cmplx attraction) {
	return move_towards(0, target, attraction);
}

#define move_towards(...) \
	MACROHAX_OVERLOAD_NARGS(_move_towards_nargs, __VA_ARGS__)(__VA_ARGS__)

INLINE MoveParams move_from_towards(cmplx origin, cmplx target, cmplx attraction) {
	return move_next(origin, move_towards(0, target, attraction));
}

INLINE MoveParams move_towards_exp(cmplx vel, cmplx target, cmplx attraction, real exponent) {
	return (MoveParams) {
		.velocity = vel,
		.attraction = attraction,
		.attraction_point = target,
		.attraction_exponent = exponent
	};
}

INLINE MoveParams move_from_towards_exp(cmplx origin, cmplx target, cmplx attraction, real exponent) {
	return move_next(origin, move_towards_exp(0, target, attraction, exponent));
}

attr_deprecated("Use move_towards_exp instead")
INLINE MoveParams move_towards_power(cmplx target, cmplx attraction, real exponent) {
	return move_towards_exp(0, target, attraction, exponent);
}

INLINE MoveParams move_dampen(cmplx vel, cmplx retention) {
	return (MoveParams) {
		.velocity = vel,
		.retention = retention,
	};
}

attr_deprecated("Use move_dampen instead")
INLINE MoveParams move_stop(cmplx retention) {
	return (MoveParams) { .retention = retention };
}
