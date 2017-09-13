/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef STAGE3_EVENTS_H
#define STAGE3_EVENTS_H

#include "boss.h"

void stage3_mid_spellbg(Boss*, int t);
void stage3_boss_spellbg(Boss*, int t);
void stage3_mid_a1(Boss*, int t);
void stage3_mid_a2(Boss*, int t);
void stage3_boss_a1(Boss*, int t);
void stage3_boss_a2(Boss*, int t);
void stage3_boss_a3(Boss*, int t);
void stage3_boss_extra(Boss*, int t);

void stage3_events(void);

#endif
