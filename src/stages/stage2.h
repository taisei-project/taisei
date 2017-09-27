/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "stage.h"

extern struct stage2_spells_s {
    // this struct must contain only fields of type AttackInfo
    // order of fields affects the visual spellstage number, but not its real internal ID

    struct {
        AttackInfo amulet_of_harm;
        AttackInfo bad_pick;
        AttackInfo wheel_of_fortune_easy;
        AttackInfo wheel_of_fortune_hard;
    } boss;

    struct {
        AttackInfo monty_hall_danmaku;
    } extra;

    // required for iteration
    AttackInfo null;
} stage2_spells;

extern StageProcs stage2_procs;
extern StageProcs stage2_spell_procs;


