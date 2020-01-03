/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_plrmodes_youmu_h
#define IGUARD_plrmodes_youmu_h

#include "taisei.h"

#include "plrmodes.h"
#include "youmu_a.h"
#include "youmu_b.h"
#include "dialog/youmu.h"

extern PlayerCharacter character_youmu;

double youmu_common_property(Player *plr, PlrProperty prop);
void youmu_common_shot(Player *plr);
void youmu_common_bombbg(Player *plr);
void youmu_common_bomb_buffer_init(void);

#endif // IGUARD_plrmodes_youmu_h
