/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_util_macrohax_h
#define IGUARD_util_macrohax_h

#include "taisei.h"

#define MACROHAX_FIRST_(f, ...) f
#define MACROHAX_FIRST(...) MACROHAX_FIRST_(__VA_ARGS__, _)
#define MACROHAX_EMPTY()
#define MACROHAX_DEFER(id) id MACROHAX_EMPTY()
#define MACROHAX_OBSTRUCT(...) __VA_ARGS__ MACROHAX_DEFER(MACROHAX_EMPTY)()
#define MACROHAX_EXPAND(...) __VA_ARGS__
#define MACROHAX_CONCAT_(a, b) a ## b
#define MACROHAX_CONCAT(a, b) MACROHAX_CONCAT_(a, b)
#define MACROHAX_ADDLINENUM(a) MACROHAX_CONCAT(a, __LINE__)
#define MACROHAX_STRINGIFY(x) #x

// Expands to 0 if argument list is empty, 1 otherwise.
// Up to 32 arguments supported.
#define MACROHAX_HASARGS(...)  \
	MACROHAX_EXPAND(MACROHAX_DEFER(MACROHAX_ARGCOUNT_HELPER)(\
		_, ##__VA_ARGS__, MACROHAX_HASARGS_RSEQ_N() \
	))

// Expands to the number of its arguments.
// Up to 32 arguments supported.
#define MACROHAX_NARGS(...)  \
	MACROHAX_EXPAND(MACROHAX_DEFER(MACROHAX_ARGCOUNT_HELPER)(\
		_, ##__VA_ARGS__, MACROHAX_NARGS_RSEQ_N() \
	))

#define MACROHAX_HASARGS_RSEQ_N() \
	1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, \
	1, 1, 1, 1, 1, 1, 1, 1, 0,

#define MACROHAX_NARGS_RSEQ_N() \
	32, 31, 30, 29, 28, 27, 26, 25,  \
	24, 23, 22, 21, 20, 19, 18, 17,  \
	16, 15, 14, 13, 12, 11, 10,  9,  \
	 8,  7,  6,  5,  4,  3,  2,  1,  0,

#define MACROHAX_ARGCOUNT_HELPER(_0, \
	__1, __2, __3, __4, __5, __6, __7, __8, \
	__9, _10, _11, _12, _13, _14, _15, _16, \
	_17, _18, _19, _20, _21, _22, _23, _24, \
	_25, _26, _27, _28, _29, _30, _31, _32, N, ...) N

// Expands to baseN, where N is the number of arguments.
#define MACROHAX_OVERLOAD_NARGS(base, ...) \
	MACROHAX_CONCAT(base, MACROHAX_NARGS(__VA_ARGS__))

// Expands to baseN, where N is 0 if the argument list is empty, 1 otherwise.
#define MACROHAX_OVERLOAD_HASARGS(base, ...) \
	MACROHAX_CONCAT(base, MACROHAX_HASARGS(__VA_ARGS__))


#endif // IGUARD_util_macrohax_h
