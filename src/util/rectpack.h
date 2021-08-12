/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "geometry.h"

typedef struct RectPack RectPack;
typedef struct RectPackSection RectPackSection;

RectPack *rectpack_new(double width, double height)
	attr_returns_max_aligned attr_returns_nonnull attr_nodiscard;

void rectpack_reset(RectPack *rp)
	attr_nonnull(1);

void rectpack_free(RectPack *rp)
	attr_nonnull(1);

RectPackSection *rectpack_add(RectPack *rp, double width, double height)
	attr_nonnull(1);

Rect rectpack_section_rect(RectPackSection *s)
	attr_nonnull(1);

void rectpack_reclaim(RectPack *rp, RectPackSection *s)
	attr_nonnull(1, 2);

bool rectpack_is_empty(RectPack *rp)
	attr_nonnull(1);
