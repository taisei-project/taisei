/*
 * This software is licensed under the terms of the MIT-License
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

#define MOVE_FUNC static inline attr_must_inline MoveParams

typedef struct MoveParams {
	complex velocity, acceleration, retention;
} MoveParams;

complex move_update(MoveParams *params);

MOVE_FUNC move_linear(complex vel) {
	return (MoveParams) { vel, 0, 1 };
}

MOVE_FUNC move_accelerated(complex vel, complex accel) {
	return (MoveParams) { vel, accel, 1 };
}

MOVE_FUNC move_asymptotic(complex vel0, complex vel1, complex retention) {
	// NOTE: retention could be derived by something like: exp(-1 / halflife)
	return (MoveParams) { vel0, vel1 * (1 - retention), retention };
}

#endif // IGUARD_move_h
