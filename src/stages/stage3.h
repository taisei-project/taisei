/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "stage.h"

extern struct stage3_spells_s {
    // this struct must contain only fields of type AttackInfo
    // order of fields affects the visual spellstage number, but not its real internal ID

    struct {
        AttackInfo deadly_dance;
        AttackInfo acid_rain;
    } mid;

    struct {
        AttackInfo moonlight_rocket;
        AttackInfo wriggle_night_ignite;
        AttackInfo unspellable_spell_name;
    } boss;

    struct {
        AttackInfo moonlight_wraith;
    } extra;

    // required for iteration
    AttackInfo null;
} stage3_spells;

extern StageProcs stage3_procs;
extern StageProcs stage3_spell_procs;
