/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_move_h
#define IGUARD_move_h

#include "taisei.h"

/*
 * Simple generalized projectile movement based on laochailan's idea
 */

typedef struct MoveParams {
	complex velocity, acceleration, retention;
	complex attraction;
	complex attraction_point;
	double attraction_max_speed;
} MoveParams;

complex move_update(complex *restrict pos, MoveParams *restrict params);

INLINE MoveParams move_linear(complex vel) {
	return (MoveParams) { vel, 0, 1 };
}

INLINE MoveParams move_accelerated(complex vel, complex accel) {
	return (MoveParams) { vel, accel, 1 };
}

INLINE MoveParams move_asymptotic(complex vel0, complex vel1, complex retention) {
	// NOTE: retention could be derived by something like: exp(-1 / halflife)
	return (MoveParams) { vel0, vel1 * (1 - retention), retention };
}

INLINE MoveParams move_asymptotic_simple(complex vel, double boost_factor) {
	// NOTE: this matches the old asymptotic rule semantics exactly
	double retention = 0.8;
	return move_asymptotic(vel * (1 + boost_factor * retention), vel, retention);
}

INLINE MoveParams move_towards(complex target, complex attraction) {
	return (MoveParams) {
		.attraction = attraction,
		.attraction_point = target,
	};
}

#endif // IGUARD_move_h
