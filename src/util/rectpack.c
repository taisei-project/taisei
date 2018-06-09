/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "rectpack.h"
#include "util.h"

typedef struct Section {
	LIST_INTERFACE(struct Section);
	Rect rect;
} Section;

typedef struct RectPack {
	double width;
	double height;
	Section *sections;
} RectPack;

static void add_section(RectPack *rp, Rect *srect) {
	Section *s = calloc(1, sizeof(Section));
	memcpy(&s->rect, srect, sizeof(*srect));
	list_push(&rp->sections, s);
}

RectPack* rectpack_new(double width, double height) {
	RectPack *rp = calloc(1, sizeof(RectPack));
	rp->width = width;
	rp->height = height;
	rectpack_reset(rp);
	return rp;
}

static void delete_all_sections(RectPack *rp) {
	list_free_all(&rp->sections);
}

void rectpack_reset(RectPack *rp) {
	delete_all_sections(rp);
	add_section(rp, &(Rect) { CMPLX(0, 0), CMPLX(rp->width, rp->height) });
}

void rectpack_free(RectPack *rp) {
	delete_all_sections(rp);
	free(rp);
}

static bool fits_surface(RectPack *rp, double width, double height) {
	assert(width > 0);
	assert(height > 0);

	return width <= rp->width && height <= rp->height;
}

static double section_fitness(Section *s, double w, double h) {
	double sw = rect_width(s->rect);
	double sh = rect_height(s->rect);

	if(w > sw || h > sh) {
		return NAN;
	}

	return fmin(sw - w, sh - h);
}

static Section* select_fittest_section(RectPack *rp, double width, double height, double *out_fitness) {
	Section *ret = NULL;
	double fitness = INFINITY;

	for(Section *s = rp->sections; s; s = s->next) {
		double f = section_fitness(s, width, height);

		if(!isnan(f) && f < fitness) {
			ret = s;
			fitness = f;
		}
	}

	if(ret != NULL && out_fitness != NULL) {
		*out_fitness = fitness;
	}

	return ret;
}

static void split_horizontal(RectPack *rp, Section *s, double width, double height) {
	Rect r = { 0 };

	if(height < rect_height(s->rect)) {
		rect_set_xywh(&r,
			rect_x(s->rect),
			rect_y(s->rect) + height,
			rect_width(s->rect),
			rect_height(s->rect) - height
		);
		add_section(rp, &r);
	}

	if(width < rect_width(s->rect)) {
		rect_set_xywh(&r,
			rect_x(s->rect) + width,
			rect_y(s->rect),
			rect_width(s->rect) - width,
			height
		);
		add_section(rp, &r);
	}
}

static void split_vertical(RectPack *rp, Section *s, double width, double height) {
	Rect r = { 0 };

	if(height < rect_height(s->rect)) {
		rect_set_xywh(&r,
			rect_x(s->rect),
			rect_y(s->rect) + height,
			width,
			rect_height(s->rect) - height
		);
		add_section(rp, &r);
	}

	if(width < rect_width(s->rect)) {
		rect_set_xywh(&r,
			rect_x(s->rect) + width,
			rect_y(s->rect),
			rect_width(s->rect) - width,
			rect_height(s->rect)
		);
		add_section(rp, &r);
	}
}

static void split(RectPack *rp, Section *s, double width, double height) {

	/*
	if(rect_width(s->rect) - width < rect_height(s->rect) - height) {
		split_vertical(rp, s, width, height);
	} else {
		split_horizontal(rp, s, width, height);
	}
	*/

	if(width * (rect_height(s->rect) - height) <= height * (rect_width(s->rect) - width)) {
		split_horizontal(rp, s, width, height);
	} else {
		split_vertical(rp, s, width, height);
	}
}

bool rectpack_add(RectPack *rp, double width, double height, Rect *out_rect) {
	if(!fits_surface(rp, width, height)) {
		return false;
	}

	Section *s = select_fittest_section(rp, width, height, NULL);

	if(s == NULL) {
		return false;
	}

	list_unlink(&rp->sections, s);
	split(rp, s, width, height);
	rect_set_xywh(out_rect, rect_x(s->rect), rect_y(s->rect), width, height);
	free(s);

	return true;
}
