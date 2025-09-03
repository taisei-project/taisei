/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "difficulty.h"
#include "i18n/i18n.h"
#include "resource/resource.h"

typedef struct DiffDef {
	const char *name;
	const char *spr_name;
	Color color;
} DiffDef;

static DiffDef diffs[] = {
	{ N_("Easy"),    "difficulty/easy",    { 0.5, 1.0, 0.5, 1.0 } },
	{ N_("Normal"),  "difficulty/normal",  { 0.5, 0.5, 1.0, 1.0 } },
	{ N_("Hard"),    "difficulty/hard",    { 1.0, 0.5, 0.5, 1.0 } },
	{ N_("Lunatic"), "difficulty/lunatic", { 1.0, 0.5, 1.0, 1.0 } },

	// TODO: sprite for this
	{ "Extra",   "difficulty/lunatic", { 0.5, 1.0, 1.0, 1.0 } },
};

static inline DiffDef *get_diff_def(Difficulty diff) {
	uint idx = diff - D_Easy;

	if(idx < sizeof(diffs)/sizeof(*diffs)) {
		return diffs + idx;
	}

	return NULL;
}

const char *difficulty_name(Difficulty diff) {
	DiffDef *d = get_diff_def(diff);
	return d ? _(d->name) : _("Unknown");
}

const char *difficulty_sprite_name(Difficulty diff) {
	DiffDef *d = get_diff_def(diff);
	return d ? d->spr_name : "difficulty/unknown";
}

const Color *difficulty_color(Difficulty diff) {
	static Color unknown_clr = { 0.5, 0.5, 0.5, 1.0 };
	DiffDef *d = get_diff_def(diff);
	return d ? &d->color : &unknown_clr;
}

void difficulty_preload(ResourceGroup *rg) {
	for(Difficulty diff = D_Easy; diff < NUM_SELECTABLE_DIFFICULTIES + D_Easy; ++diff) {
		res_group_preload(rg, RES_SPRITE, RESF_DEFAULT, difficulty_sprite_name(diff), NULL);
	}
}
