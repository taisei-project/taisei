/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef STAGE6_H
#define STAGE6_H

#include "stage.h"

extern StageProcs stage6_procs;
extern StageProcs stage6_spell_procs;
extern AttackInfo stage6_spells[];

void start_fall_over(void);

#endif
