/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "geometry.h"
#include "list.h"
#include "memory/mempool.h"

typedef struct RectPack RectPack;
typedef struct RectPackSection RectPackSection;
typedef MEMPOOL(RectPackSection) RectPackSectionPool;
typedef struct RectPackSectionSource RectPackSectionSource;

struct RectPackSection {
	LIST_INTERFACE(RectPackSection);
	RectPackSection *parent;
	RectPackSection *sibling;
	RectPackSection *children[2];
	Rect rect;
};

struct RectPack {
	RectPackSection root;
	RectPackSection *unused_sections;
};

struct RectPackSectionSource {
	RectPackSectionPool *pool;
	MemArena *arena;
};

void rectpack_init(RectPack *rp, double width, double height)
	attr_nonnull_all;

RectPackSection *rectpack_add(
	RectPack *rp,
	RectPackSectionSource secsrc,
	double width,
	double height,
	bool allow_rotation
) attr_nonnull(1);

Rect rectpack_section_rect(RectPackSection *s)
	attr_nonnull(1);

void rectpack_reclaim(RectPack *rp, RectPackSectionSource secsrc, RectPackSection *s)
	attr_nonnull(1, 3);

bool rectpack_is_empty(RectPack *rp)
	attr_nonnull(1);
