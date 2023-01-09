/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

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

struct RectPackSection {
	LIST_INTERFACE(RectPackSection);
	Rect rect;
	RectPackSection *parent;
	RectPackSection *sibling;
	RectPackSection *children[2];
};

struct RectPack {
	RectPackSection root;
	RectPackSection *freelist;
};

static inline bool section_is_free(RectPackSection *s) {
	return s->next != s;
}

static inline void section_make_used(RectPack *rp, RectPackSection *s) {
	assert(section_is_free(s));
	list_unlink(&rp->freelist, s);
	s->next = s->prev = s;
}

RectPack* rectpack_new(double width, double height) {
	auto rp = ALLOC(RectPack, {
		.root.rect = {
			.top_left = CMPLX(0, 0),
			.bottom_right = CMPLX(width, height),
		},
	});
	list_push(&rp->freelist, &rp->root);
	assert(rectpack_is_empty(rp));
	return rp;
}

bool rectpack_is_empty(RectPack *rp) {
	if(rp->freelist == &rp->root) {
		assert(rp->root.next == NULL);
		return true;
	}

	return false;
}

static void delete_subsections(RectPackSection *restrict s) {
	if(s->children[0] != NULL) {
		assume(s->children[1] != NULL);
		assume(s->children[0]->parent == s);
		assume(s->children[1]->parent == s);

		delete_subsections(s->children[0]);
		mem_free(s->children[0]);
		s->children[0] = NULL;

		delete_subsections(s->children[1]);
		mem_free(s->children[1]);
		s->children[1] = NULL;
	}
}

void rectpack_reset(RectPack *rp) {
	delete_subsections(&rp->root);
}

void rectpack_free(RectPack *rp) {
	delete_subsections(&rp->root);
	mem_free(rp);
}

static double section_fitness(RectPackSection *s, double w, double h) {
	double sw = rect_width(s->rect);
	double sh = rect_height(s->rect);

	if(w > sw || h > sh) {
		return NAN;
	}

	// Best Long Side Fit (BLSF)
	// This method has a nice property: fitness==0 indicates an exact fit.

	return fmax(sw - w, sh - h);
}

void rectpack_reclaim(RectPack *rp, RectPackSection *s) {
	assume(s->children[0] == NULL);
	assume(s->children[1] == NULL);

	double attr_unused w = rect_width(s->rect);
	double attr_unused h = rect_height(s->rect);

	RP_DEBUG("BEGIN RECLAIM %p[%gx%g]", (void*)s, w, h);

	if(s->sibling && section_is_free(s->sibling)) {
		RP_DEBUG("has free sibling; merging and reclaiming parent");

		RectPackSection *parent = s->parent;
		assume(parent != NULL);
		assume(s->sibling->parent == parent);
		assert(!section_is_free(s->parent));
		assert(!section_is_free(s));

		list_unlink(&rp->freelist, s->sibling);

		// NOTE: the following frees s->sibling and s, in unspecified order

		mem_free(parent->children[0]);
		parent->children[0] = NULL;

		mem_free(parent->children[1]);
		parent->children[1] = NULL;

		if(parent != NULL) {
			rectpack_reclaim(rp, parent);
		}

		RP_DEBUG("done reclaiming parent of %p", (void*)s);
	} else {
		RP_DEBUG("added to free list");
		list_push(&rp->freelist, s);
		assert(s != &rp->root || rectpack_is_empty(rp));
	}

	RP_DEBUG("END RECLAIM %p[%gx%g]", (void*)s, w, h);
}

static RectPackSection *select_fittest_section(RectPack *rp, double width, double height, double *out_fitness) {
	RectPackSection *best = NULL;
	double fitness = DBL_MAX;

	RP_DEBUG("trying to fit %gx%g...", width, height);

	for(RectPackSection *s = rp->freelist; s; s = s->next) {
		assume(s->children[0] == NULL);
		assume(s->children[1] == NULL);

		double f = section_fitness(s, width, height);

		if(!isnan(f) && f < fitness) {
			best = s;
			fitness = f;
			RP_DEBUG("candidate: %g (%gx%g)", fitness, rect_width(best->rect), rect_height(best->rect));
		}
	}

	if(best) {
		RP_DEBUG("fitness for %gx%g: %f (%gx%g)", width, height, fitness, rect_width(best->rect), rect_height(best->rect));
	} else {
		RP_DEBUG("%gx%g doesn't fit at all", width, height);
	}

	if(out_fitness) {
		*out_fitness = fitness;
	}

	return best;
}

static RectPackSection *split_horizontal(RectPack *rp, RectPackSection *s, double width, double height);
static RectPackSection *split_vertical(RectPack *rp, RectPackSection *s, double width, double height);

static RectPackSection *split_horizontal(RectPack *rp, RectPackSection *s, double width, double height) {
	RP_DEBUG("spliting section %p of size %gx%g for rect %gx%g", (void*)s, rect_width(s->rect), rect_height(s->rect), width, height);

	assert(rect_width(s->rect) >= width);
	assert(rect_height(s->rect) >= height);

	if(rect_height(s->rect) == height) {
		assert(rect_width(s->rect) > width);
		RP_DEBUG("delegated to vertical split");
		return split_vertical(rp, s, width, height);
	}

	auto sub = ALLOC(RectPackSection);
	rect_set_xywh(&sub->rect,
		rect_x(s->rect),
		rect_y(s->rect),
		rect_width(s->rect),
		height
	);

	section_make_used(rp, sub);

	sub->parent = s;
	s->children[0] = sub;

	s->children[1] = ALLOC(typeof(*s->children[1]));
	rect_set_xywh(&s->children[1]->rect,
		rect_x(s->rect),
		rect_y(s->rect) + height,
		rect_width(s->rect),
		rect_height(s->rect) - height
	);
	s->children[1]->parent = s;
	sub->sibling = s->children[1];
	s->children[1]->sibling = sub;
	list_push(&rp->freelist, s->children[1]);

	RP_DEBUG("made new subsections from %p: %p[%gx%g]; %p[%gx%g]",
		(void*)s,
		(void*)s->children[0], rect_width(s->children[0]->rect), rect_height(s->children[0]->rect),
		(void*)s->children[1], rect_width(s->children[1]->rect), rect_height(s->children[1]->rect)
	);

	if(rect_width(sub->rect) != width) {
		sub = split_vertical(rp, sub, width, height);
	}

	return sub;
}

static RectPackSection *split_vertical(RectPack *rp, RectPackSection *s, double width, double height) {
	assert(rect_width(s->rect) >= width);
	assert(rect_height(s->rect) >= height);

	RP_DEBUG("spliting section %p of size %gx%g for rect %gx%g", (void*)s, rect_width(s->rect), rect_height(s->rect), width, height);

	if(rect_width(s->rect) == width) {
		assert(rect_height(s->rect) > height);
		RP_DEBUG("delegated to horizontal split");
		return split_horizontal(rp, s, width, height);
	}

	auto sub = ALLOC(RectPackSection);
	rect_set_xywh(&sub->rect,
		rect_x(s->rect),
		rect_y(s->rect),
		width,
		rect_height(s->rect)
	);

	section_make_used(rp, sub);

	sub->parent = s;
	s->children[0] = sub;

	s->children[1] = ALLOC(typeof(*s->children[1]));
	rect_set_xywh(&s->children[1]->rect,
		rect_x(s->rect) + width,
		rect_y(s->rect),
		rect_width(s->rect) - width,
		rect_height(s->rect)
	);
	s->children[1]->parent = s;
	sub->sibling = s->children[1];
	s->children[1]->sibling = sub;
	list_push(&rp->freelist, s->children[1]);

	RP_DEBUG("made new subsections from %p: %p[%gx%g]; %p[%gx%g]",
		(void*)s,
		(void*)s->children[0], rect_width(s->children[0]->rect), rect_height(s->children[0]->rect),
		(void*)s->children[1], rect_width(s->children[1]->rect), rect_height(s->children[1]->rect)
	);

	if(rect_height(sub->rect) != height) {
		sub = split_vertical(rp, sub, width, height);
	}

	return sub;
}

static RectPackSection *split(RectPack *rp, RectPackSection *s, double width, double height) {
	if(width * (rect_height(s->rect) - height) <= height * (rect_width(s->rect) - width)) {
		return split_horizontal(rp, s, width, height);
	} else {
		return split_vertical(rp, s, width, height);
	}
}

RectPackSection *rectpack_add(RectPack *rp, double width, double height) {
	double fitness;
	RectPackSection *s = select_fittest_section(rp, width, height, &fitness);

	if(s == NULL) {
		return NULL;
	}

	section_make_used(rp, s);

	if(fitness == 0) {   // exact fit
		assert(rect_width(s->rect) == width);
		assert(rect_height(s->rect) == height);
		return s;
	}

	return split(rp, s, width, height);
}

Rect rectpack_section_rect(RectPackSection *s) {
	return s->rect;
}
