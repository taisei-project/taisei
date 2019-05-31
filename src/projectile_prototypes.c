/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "projectile.h"
#include "util.h"
#include "random.h"

typedef struct PPBasicPriv {
	const char *sprite_name;
	ResourceRef sprite_ref;
	complex size;
	complex collision_size;
} PPBasicPriv;

static void pp_basic_preload(ProjPrototype *proto, ResourceRefGroup *rg) {
	// res_group_multi_add(rg, RES_SPRITE, ((PPBasicPriv*)proto->private)->sprite_name, RESF_DEFAULT);
	// not assigning ->sprite here because it'll block the thread until loaded
	PPBasicPriv *pdata = proto->private;
	pdata->sprite_ref = res_ref(RES_SPRITE, pdata->sprite_name, RESF_DEFAULT);
	res_group_add_ref(rg, pdata->sprite_ref);
}

static void pp_basic_init_projectile(ProjPrototype *proto, Projectile *p) {
	PPBasicPriv *pdata = proto->private;

	p->sprite = res_ref_data(pdata->sprite_ref);
	p->size = pdata->size;
	p->collision_size = pdata->collision_size;
}

#define _PP_BASIC(sprite, width, height, colwidth, colheight) \
	ProjPrototype _pp_##sprite = { \
		.preload = pp_basic_preload, \
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

static void pp_blast_init_projectile(ProjPrototype *proto, Projectile *p) {
	pp_basic_init_projectile(proto, p);
	assert(p->rule == NULL);
	assert(p->timeout > 0);
	p->args[1] = frand()*360 + frand()*I;
	p->args[2] = frand()     + frand()*I;
}

ProjPrototype _pp_blast = {
	.preload = pp_basic_preload,
	.init_projectile = pp_blast_init_projectile,
	.private = &(PPBasicPriv) {
		.sprite_name = "part/blast",
		.size = 128 * (1+I),
	},
}, *pp_blast = &_pp_blast;
