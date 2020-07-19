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
#define MACROHAX_CONCAT(a, b) a ## b
#define MACROHAX_ADDLINENUM(a) MACROHAX_EXPAND(MACROHAX_DEFER(MACROHAX_CONCAT)(a, __LINE__))
#define MACROHAX_STRINGIFY(x) #x

#endif // IGUARD_util_macrohax_h
