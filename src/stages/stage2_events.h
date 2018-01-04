/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "boss.h"

void hina_spell_bg(Boss*, int);
void hina_amulet(Boss*, int);
void hina_bad_pick(Boss*, int);
void hina_wheel(Boss*, int);
void hina_monty(Boss*, int);

void stage2_events(void);
Boss* stage2_spawn_hina(complex pos);
