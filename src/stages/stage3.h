/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "stage.h"

extern struct stage3_spells_s {
    // this struct must contain only fields of type AttackInfo
    // order of fields affects the visual spellstage number, but not its real internal ID

    struct {
        AttackInfo deadly_dance;
    } mid;

    struct {
        AttackInfo moonlight_rocket;
        AttackInfo wriggle_night_ignite;
        AttackInfo firefly_storm;
    } boss;

    struct {
        AttackInfo light_singularity;
    } extra;

    // required for iteration
    AttackInfo null;
} stage3_spells;

extern StageProcs stage3_procs;
extern StageProcs stage3_spell_procs;
