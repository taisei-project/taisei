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
#include "memory/allocator.h"

typedef struct RectPack RectPack;
typedef struct RectPackSection RectPackSection;

struct RectPackSection {
	LIST_INTERFACE(RectPackSection);
	Rect rect;
	RectPackSection *parent;
	RectPackSection *sibling;
	RectPackSection *children[2];
};

struct RectPack {
	RectPackSection root;
	RectPackSection *unused_sections;
	RectPackSection *sections_freelist;
	Allocator *allocator;
};

void rectpack_init(RectPack *rp, Allocator *alloc, double width, double height)
	attr_nonnull_all;

void rectpack_reset(RectPack *rp)
	attr_nonnull(1);

void rectpack_deinit(RectPack *rp)
	attr_nonnull(1);

RectPackSection *rectpack_add(RectPack *rp, double width, double height, bool allow_rotation)
	attr_nonnull(1);

#define _rectpack_add_3(rp, width, height) \
	rectpack_add(rp, width, height, false)
#define _rectpack_add_4(rp, width, height, allow_rotation) \
	rectpack_add(rp, width, height, allow_rotation)
#define rectpack_add(...) \
	MACROHAX_OVERLOAD_NARGS(_rectpack_add_, __VA_ARGS__)(__VA_ARGS__)

Rect rectpack_section_rect(RectPackSection *s)
	attr_nonnull(1);

void rectpack_reclaim(RectPack *rp, RectPackSection *s)
	attr_nonnull(1, 2);

bool rectpack_is_empty(RectPack *rp)
	attr_nonnull(1);
