/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "projectile.h"
#include "util.h"
#include "random.h"

typedef struct PPBasicPriv {
	const char *sprite_name;
	Sprite *sprite;
	complex size;
} PPBasicPriv;

static void pp_basic_preload(ProjPrototype *proto) {
	preload_resource(RES_SPRITE, ((PPBasicPriv*)proto->private)->sprite_name, RESF_PERMANENT);
	// not assigning ->sprite here because it'll block the thread until loaded
}

static void pp_basic_init_projectile(ProjPrototype *proto, Projectile *p) {
	PPBasicPriv *pdata = proto->private;

	if(!p->sprite) {
		p->sprite = pdata->sprite
			? pdata->sprite
			: (pdata->sprite = get_sprite(pdata->sprite_name));
	}

	p->size = pdata->size;
}

#define PP_BASIC(sprite, width, height) \
	ProjPrototype _pp_##sprite = { \
		.preload = pp_basic_preload, \
		.init_projectile = pp_basic_init_projectile, \
		.private = (&(PPBasicPriv) { \
			.sprite_name = "proj/" #sprite, \
			.size = (width) + (height) * I, \
		}), \
	}, *pp_##sprite = &_pp_##sprite; \

#include "projectile_prototypes/basic.inc.h"

static int pp_blast_rule(Projectile *p, int t) {
	// TODO: move this into pp_blast_init_projectile when v1.2 compat is not needed

	if(t == 1) {
		p->args[1] = frand()*360 + frand()*I;
		p->args[2] = frand()     + frand()*I;
	}

	return 1;
}

static void pp_blast_init_projectile(ProjPrototype *proto, Projectile *p) {
	pp_basic_init_projectile(proto, p);
	assert(p->rule == NULL);
	assert(p->timeout > 0);
	p->rule = pp_blast_rule;
}

ProjPrototype _pp_blast = {
	.preload = pp_basic_preload,
	.init_projectile = pp_blast_init_projectile,
	.private = &(PPBasicPriv) {
		.sprite_name = "part/blast",
		.size = 128 * (1+I),
	},
}, *pp_blast = &_pp_blast;
