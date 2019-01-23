/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_stages_stage4_events_h
#define IGUARD_stages_stage4_events_h

#include "taisei.h"

#include "boss.h"

void kurumi_spell_bg(Boss*, int);
void kurumi_slaveburst(Boss*, int);
void kurumi_redspike(Boss*, int);
void kurumi_aniwall(Boss*, int);
void kurumi_blowwall(Boss*, int);
void kurumi_danmaku(Boss*, int);
void kurumi_extra(Boss*, int);

void stage4_events(void);
Boss* stage4_spawn_kurumi(complex pos);

#define STAGE4_MIDBOSS_TIME 3724
#define STAGE4_MIDBOSS_MUSIC_TIME 2818

#endif // IGUARD_stages_stage4_events_h
