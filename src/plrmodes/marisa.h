/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "plrmodes.h"
#include "marisa_a.h"
#include "marisa_b.h"
#include "dialog/marisa.h"
#include "color.h"

extern PlayerCharacter character_marisa;

double marisa_common_property(Player *plr, PlrProperty prop);
void marisa_common_shot(Player *plr, float dmg);
void marisa_common_slave_visual(Enemy *e, int t, bool render);

typedef struct MarisaBeamInfo {
	complex origin;
	complex size;
	float angle;
	int t;
} MarisaBeamInfo;

void marisa_common_masterspark_draw(int numBeams, MarisaBeamInfo *beamInfos, float alpha);
