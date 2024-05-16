/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "projectile.h"
#include "util.h"
#include "random.h"

typedef struct PPBasicPriv {
	const char *sprite_name;
	Sprite *sprite;
	cmplx size;
	cmplx collision_size;
} PPBasicPriv;

static void pp_basic_preload(ProjPrototype *proto, ResourceGroup *rg) {
	res_group_preload(rg, RES_SPRITE, 0, ((PPBasicPriv*)proto->private)->sprite_name, NULL);
	// not assigning ->sprite here because it'll block the thread until loaded
}

static void pp_basic_reset(ProjPrototype *proto) {
	PPBasicPriv *pdata = proto->private;
	pdata->sprite = NULL;
}

static void pp_basic_init_projectile(ProjPrototype *proto, Projectile *p) {
	PPBasicPriv *pdata = proto->private;

	p->sprite = pdata->sprite
		? pdata->sprite
		: (pdata->sprite = res_sprite(pdata->sprite_name));

	p->size = pdata->size;
	p->collision_size = pdata->collision_size;
}

#define _PP_BASIC(sprite, width, height, colwidth, colheight) \
	ProjPrototype _pp_##sprite = { \
		.preload = pp_basic_preload, \
		.reset = pp_basic_reset, \
		.init_projectile = pp_basic_init_projectile, \
		.private = (&(PPBasicPriv) { \
			.sprite_name = "proj/" #sprite, \
			.size = (width) + (height) * I, \
			.collision_size = (colwidth) + (colheight) * I, \
		}), \
	}, *pp_##sprite = &_pp_##sprite; \

#define PP_BASIC(sprite, width, height, colwidth, colheight) _PP_BASIC(sprite, width, height, colwidth, colheight)
#include "projectile_prototypes/basic.inc.h"

// TODO: customize defaults
#define PP_PLAYER(sprite, width, height) _PP_BASIC(sprite, width, height, 0, 0)
#include "projectile_prototypes/player.inc.h"

ProjPrototype _pp_blast = {
	.preload = pp_basic_preload,
	.reset = pp_basic_reset,
	.init_projectile = pp_basic_init_projectile,
	.private = &(PPBasicPriv) {
		.sprite_name = "part/blast",
		.size = 128 * (1+I),
	},
}, *pp_blast = &_pp_blast;
