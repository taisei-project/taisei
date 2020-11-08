/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_util_glm_h
#define IGUARD_util_glm_h

#include "taisei.h"

// This is a wrapper header to include cglm the "right way".
// Please include this in any C file where access to the glm_* functions is needed.
// You don't need to include this if you just want the types.

#include "util.h"

DIAGNOSTIC(push)
DIAGNOSTIC(ignored "-Wdeprecated-declarations")
#include <cglm/cglm.h>
DIAGNOSTIC(pop)

typedef float (*glm_ease_t)(float);

INLINE float glm_ease_apply(glm_ease_t ease, float f) {
	return ease ? ease(f) : f;
}

#endif // IGUARD_util_glm_h
