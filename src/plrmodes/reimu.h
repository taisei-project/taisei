/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "plrmodes.h"

extern PlayerCharacter character_reimu;
extern PlayerMode plrmode_reimu_a;
extern PlayerMode plrmode_reimu_b;

double reimu_common_property(Player *plr, PlrProperty prop);
Projectile *reimu_common_ofuda_swawn_trail(Projectile *p);
void reimu_common_bomb_bg(Player *p, float alpha);
void reimu_common_bomb_buffer_init(void);
