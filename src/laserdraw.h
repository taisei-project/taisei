/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_laserdraw_h
#define IGUARD_laserdraw_h

#include "taisei.h"

#include "entity.h"

void laserdraw_preload(void);
void laserdraw_init(void);
void laserdraw_shutdown(void);

void laserdraw_ent_drawfunc(EntityInterface *ent);

typedef struct LaserRenderData LaserRenderData;
struct LaserRenderData {
	LIST_INTERFACE(LaserRenderData);
	int tile;

	struct {
		FloatOffset top_left, bottom_right;
	} bbox;
};

#endif // IGUARD_laserdraw_h
