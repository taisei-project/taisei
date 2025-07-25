/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rectpack.h"
#include "util.h"

/*
 *  This implements a slightly modified Guillotine rect-packing algorithm.
 *  All subdivisions are tracked with a tree data structure, which enables fairly
 *  efficient deallocation.
 *  Rotations are not supported.
 */

// #define RP_DEBUG

#if defined RP_DEBUG
	#undef RP_DEBUG
	#define RP_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#define RP_DEBUG(...) ((void)0)
#endif

static inline bool section_is_unused(RectPackSection *s) {
	return s->next != s;
}

static inline void section_make_used(RectPack *rp, RectPackSection *s) {
	assert(section_is_unused(s));
	list_unlink(&rp->unused_sections, s);
	s->next = s->prev = s;
}

static RectPackSection *acquire_section(RectPackSectionSource secsrc) {
	return mempool_acquire(secsrc.pool, secsrc.arena);
}

static void release_section(RectPackSectionSource secsrc, RectPackSection *s) {
	mempool_release(secsrc.pool, s);
}

void rectpack_init(RectPack *rp, double width, double height) {
	*rp = (RectPack) {
		.root.rect = {
			.top_left = CMPLX(0, 0),
			.bottom_right = CMPLX(width, height),
		},
	};
	list_push(&rp->unused_sections, &rp->root);
	assert(rectpack_is_empty(rp));
}

bool rectpack_is_empty(RectPack *rp) {
	if(rp->unused_sections == &rp->root) {
		assert(rp->root.next == NULL);
		return true;
	}

	return false;
}

static double section_fitness(RectPackSection *s, double w, double h) {
	double sw = rect_width(s->rect);
	double sh = rect_height(s->rect);

	if(w > sw || h > sh) {
		return NAN;
	}

	return sw * sh - w * h;  // best area fit
}

void rectpack_reclaim(RectPack *rp, RectPackSectionSource secsrc, RectPackSection *s) {
	assume(s->children[0] == NULL);
	assume(s->children[1] == NULL);

	double attr_unused w = rect_width(s->rect);
	double attr_unused h = rect_height(s->rect);

	RP_DEBUG("BEGIN RECLAIM %p[%gx%g]", (void*)s, w, h);

	if(s->sibling && section_is_unused(s->sibling)) {
		RP_DEBUG("has free sibling; merging and reclaiming parent");

		RectPackSection *parent = s->parent;
		assume(parent != NULL);
		assume(s->sibling->parent == parent);
		assert(!section_is_unused(s->parent));
		assert(!section_is_unused(s));

		list_unlink(&rp->unused_sections, s->sibling);

		// NOTE: the following frees s->sibling and s, in unspecified order

		release_section(secsrc, parent->children[0]);
		parent->children[0] = NULL;

		release_section(secsrc, parent->children[1]);
		parent->children[1] = NULL;

		if(parent != NULL) {
			rectpack_reclaim(rp, secsrc, parent);
		}

		RP_DEBUG("done reclaiming parent of %p", (void*)s);
	} else {
		RP_DEBUG("added to free list");
		list_push(&rp->unused_sections, s);
		assert(s != &rp->root || rectpack_is_empty(rp));
	}

	RP_DEBUG("END RECLAIM %p[%gx%g]", (void*)s, w, h);
}

static RectPackSection *select_fittest_section(
	RectPack *rp, RectPackSectionSource secsrc, double *width, double *height, bool allow_rotation
) {
	RectPackSection *best = NULL;
	double fitness = DBL_MAX;

	RP_DEBUG("trying to fit %gx%g...", width, height);

	bool rotated = false;

	for(RectPackSection *s = rp->unused_sections; s; s = s->next) {
		assume(s->children[0] == NULL);
		assume(s->children[1] == NULL);

		double f = section_fitness(s, *width, *height);

		if(!isnan(f) && f < fitness) {
			best = s;
			fitness = f;
			rotated = false;
			RP_DEBUG("candidate: %g (%gx%g)", fitness, rect_width(best->rect), rect_height(best->rect));
		}

		if(allow_rotation) {
			f = section_fitness(s, *height, *width);

			if(!isnan(f) && f < fitness) {
				best = s;
				fitness = f;
				rotated = true;
				RP_DEBUG("candidate: %g (%gx%g) (rotated)", fitness, rect_width(best->rect), rect_height(best->rect));
			}
		}
	}

	if(best) {
		RP_DEBUG("fitness for %gx%g: %f (%gx%g)", width, height, fitness, rect_width(best->rect), rect_height(best->rect));
	} else {
		RP_DEBUG("%gx%g doesn't fit at all", width, height);
	}

	if(rotated) {
		SWAP(*width, *height);
	}

	return best;
}

static RectPackSection *split_horizontal(
	RectPack *rp, RectPackSectionSource secsrc, RectPackSection *s, double width, double height);
static RectPackSection *split_vertical(
	RectPack *rp, RectPackSectionSource secsrc, RectPackSection *s, double width, double height);

static RectPackSection *split_horizontal(
	RectPack *rp, RectPackSectionSource secsrc,  RectPackSection *s, double width, double height
) {
	RP_DEBUG("splitting section %p of size %gx%g for rect %gx%g", (void*)s, rect_width(s->rect), rect_height(s->rect), width, height);

	assert(rect_width(s->rect) >= width);
	assert(rect_height(s->rect) >= height);

	if(rect_height(s->rect) == height) {
		assert(rect_width(s->rect) > width);
		RP_DEBUG("delegated to vertical split");
		return split_vertical(rp, secsrc, s, width, height);
	}

	auto sub = acquire_section(secsrc);
	rect_set_xywh(&sub->rect,
		rect_x(s->rect),
		rect_y(s->rect),
		rect_width(s->rect),
		height
	);

	section_make_used(rp, sub);

	sub->parent = s;
	s->children[0] = sub;
	s->children[1] = acquire_section(secsrc);
	rect_set_xywh(&s->children[1]->rect,
		rect_x(s->rect),
		rect_y(s->rect) + height,
		rect_width(s->rect),
		rect_height(s->rect) - height
	);
	s->children[1]->parent = s;
	sub->sibling = s->children[1];
	s->children[1]->sibling = sub;
	list_push(&rp->unused_sections, s->children[1]);

	RP_DEBUG("made new subsections from %p: %p[%gx%g]; %p[%gx%g]",
		(void*)s,
		(void*)s->children[0], rect_width(s->children[0]->rect), rect_height(s->children[0]->rect),
		(void*)s->children[1], rect_width(s->children[1]->rect), rect_height(s->children[1]->rect)
	);

	if(rect_width(sub->rect) != width) {
		sub = split_vertical(rp, secsrc, sub, width, height);
	}

	return sub;
}

static RectPackSection *split_vertical(
	RectPack *rp, RectPackSectionSource secsrc, RectPackSection *s, double width, double height
) {
	assert(rect_width(s->rect) >= width);
	assert(rect_height(s->rect) >= height);

	RP_DEBUG("splitting section %p of size %gx%g for rect %gx%g", (void*)s, rect_width(s->rect), rect_height(s->rect), width, height);

	if(rect_width(s->rect) == width) {
		assert(rect_height(s->rect) > height);
		RP_DEBUG("delegated to horizontal split");
		return split_horizontal(rp, secsrc, s, width, height);
	}

	auto sub = acquire_section(secsrc);
	rect_set_xywh(&sub->rect,
		rect_x(s->rect),
		rect_y(s->rect),
		width,
		rect_height(s->rect)
	);

	section_make_used(rp, sub);

	sub->parent = s;
	s->children[0] = sub;
	s->children[1] = acquire_section(secsrc);
	rect_set_xywh(&s->children[1]->rect,
		rect_x(s->rect) + width,
		rect_y(s->rect),
		rect_width(s->rect) - width,
		rect_height(s->rect)
	);
	s->children[1]->parent = s;
	sub->sibling = s->children[1];
	s->children[1]->sibling = sub;
	list_push(&rp->unused_sections, s->children[1]);

	RP_DEBUG("made new subsections from %p: %p[%gx%g]; %p[%gx%g]",
		(void*)s,
		(void*)s->children[0], rect_width(s->children[0]->rect), rect_height(s->children[0]->rect),
		(void*)s->children[1], rect_width(s->children[1]->rect), rect_height(s->children[1]->rect)
	);

	if(rect_height(sub->rect) != height) {
		sub = split_horizontal(rp, secsrc, sub, width, height);
	}

	return sub;
}

static RectPackSection *split(
	RectPack *rp, RectPackSectionSource secsrc, RectPackSection *s, double width, double height
) {
	// short leftover axis split

	if(rect_width(s->rect) - width < rect_height(s->rect) - height) {
		return split_horizontal(rp, secsrc, s, width, height);
	} else {
		return split_vertical(rp, secsrc, s, width, height);
	}
}

RectPackSection *rectpack_add(
	RectPack *rp,
	RectPackSectionSource secsrc,
	double width,
	double height,
	bool allow_rotation
) {
	RectPackSection *s = select_fittest_section(rp, secsrc, &width, &height, allow_rotation);

	if(s == NULL) {
		return NULL;
	}

	section_make_used(rp, s);

	if(rect_width(s->rect) == width && rect_height(s->rect) == height) {
		return s;
	}

	return split(rp, secsrc, s, width, height);
}

Rect rectpack_section_rect(RectPackSection *s) {
	return s->rect;
}
