/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "plrmodes.h"
#include "marisa_a.h"
#include "marisa_b.h"

extern PlayerCharacter character_marisa;

void marisa_common_shot(Player *plr, int dmg);
void marisa_common_slave_visual(Enemy *e, int t, bool render);
void marisa_common_masterspark_draw(int t);
