/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_plrmodes_marisa_h
#define IGUARD_plrmodes_marisa_h

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
	cmplx origin;
	cmplx size;
	float angle;
	int t;
} MarisaBeamInfo;

void marisa_common_masterspark_draw(int numBeams, MarisaBeamInfo *beamInfos, float alpha);

#endif // IGUARD_plrmodes_marisa_h
