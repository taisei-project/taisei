/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
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

cmplx move_update(cmplx *restrict pos, MoveParams *restrict params);
cmplx move_update_multiple(uint times, cmplx *restrict pos, MoveParams *restrict params);

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

INLINE MoveParams move_towards(cmplx target, cmplx attraction) {
	return (MoveParams) {
		.attraction = attraction,
		.attraction_point = target,
		.attraction_exponent = 1
	};
}

INLINE MoveParams move_towards_power(cmplx target, cmplx attraction, real exponent) {
	return (MoveParams) {
		.attraction = attraction,
		.attraction_point = target,
		.attraction_exponent = exponent
	};
}

INLINE MoveParams move_stop(cmplx retention) {
	return (MoveParams) { .retention = retention };
}
