/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "geometry.h"

typedef struct RectPack RectPack;

RectPack* rectpack_new(double width, double height);
void rectpack_reset(RectPack *rp);
void rectpack_free(RectPack *rp);
bool rectpack_add(RectPack *rp, double width, double height, Rect *out_rect);
