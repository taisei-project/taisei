/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_stages_stage5_events_h
#define IGUARD_stages_stage5_events_h

#include "taisei.h"

#include "boss.h"

void iku_spell_bg(Boss*, int);
void iku_atmospheric(Boss*, int);
void iku_lightning(Boss*, int);
void iku_cathode(Boss*, int);
void iku_induction(Boss*, int);
void iku_extra(Boss*, int);

void stage5_events(void);
Boss* stage5_spawn_iku(complex pos);

#endif // IGUARD_stages_stage5_events_h
