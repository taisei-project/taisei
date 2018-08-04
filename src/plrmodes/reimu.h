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
#include "dialog/reimu.h"

extern PlayerCharacter character_reimu;
extern PlayerMode plrmode_reimu_a;
extern PlayerMode plrmode_reimu_b;

double reimu_common_property(Player *plr, PlrProperty prop);
void reimu_common_shot(Player *plr, int dmg);
int reimu_common_ofuda(Projectile *p, int t);
void reimu_common_draw_yinyang(Enemy *e, int t, const Color *c);
void reimu_common_bomb_bg(Player *p, float alpha);
void reimu_common_bomb_buffer_init(void);
