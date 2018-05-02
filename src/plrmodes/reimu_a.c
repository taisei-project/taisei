/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "reimu.h"

static void reimu_spirit_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"yinyang",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"sprite_yinyang",
	NULL);

	//preload_resources(RES_SFX, flags | RESF_OPTIONAL,
	//NULL);
}

static void reimu_spirit_bomb(Player *p) {
}

static void reimu_spirit_shot(Player *p) {
}

static int reimu_spirit_slave(Enemy *e, int t) {
	TIMER(&t);

	AT(EVENT_BIRTH) {
		e->pos = global.plr.pos;
		return 1;
	}

	GO_TO(e, global.plr.pos + e->args[!!(global.plr.inputflags & INFLAG_FOCUS)], 0.005 * cabs(e->args[0]));

	return 1;
}

static void reimu_spirit_respawn_slaves(Player *plr, short npow) {
	Enemy *e = plr->slaves, *tmp;
	double dmg = 56;

	while(e != 0) {
		tmp = e;
		e = e->next;
		if(tmp->hp == ENEMY_IMMUNE)
			delete_enemy(&plr->slaves, tmp);
	}

	if(npow / 100 == 1) {
		create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, 50.0*I, -50.0*I, -0.1*I, dmg);
	}

	if(npow >= 200) {
		create_enemy_p(&plr->slaves, 30.0*I+15, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, +40, +20-30*I,  1-0.1*I, dmg);
		create_enemy_p(&plr->slaves, 30.0*I-15, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, -40, -20-30*I, -1-0.1*I, dmg);
	}

	if(npow / 100 == 3) {
		create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, 50.0*I, -50.0*I, -0.1*I, dmg);
	}

	if(npow >= 400) {
		create_enemy_p(&plr->slaves,  30, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, +80, +40-20*I,  2-0.1*I, dmg);
		create_enemy_p(&plr->slaves, -30, ENEMY_IMMUNE, reimu_yinyang_visual, reimu_spirit_slave, -80, -40-20*I, -2-0.1*I, dmg);
	}

	for(e = plr->slaves; e; e = e->next) {
		e->ent.draw_layer = LAYER_PLAYER_SLAVE;
	}
}

static void reimu_spirit_init(Player *plr) {
	reimu_spirit_respawn_slaves(plr, plr->power);
}

static void reimu_spirit_power(Player *plr, short npow) {
	if(plr->power / 100 != npow / 100) {
		reimu_spirit_respawn_slaves(plr, npow);
	}
}

PlayerMode plrmode_reimu_a = {
	.name = "Spirit Sign",
	.character = &character_reimu,
	.dialog = &dialog_reimu,
	.shot_mode = PLR_SHOT_REIMU_SPIRIT,
	.procs = {
		.property = reimu_common_property,
		.init = reimu_spirit_init,
		.bomb = reimu_spirit_bomb,
		.shot = reimu_spirit_shot,
		.power = reimu_spirit_power,
		.preload = reimu_spirit_preload,
	},
};
