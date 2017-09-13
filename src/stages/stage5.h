/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef STAGE5_H
#define STAGE5_H

#include "stage.h"

extern struct stage5_spells_s {
    // this struct must contain only fields of type AttackInfo
    // order of fields affects the visual spellstage number, but not its real internal ID

    struct {
        AttackInfo atmospheric_discharge;
        AttackInfo artificial_lightning;
        AttackInfo natural_cathode;
        AttackInfo induction_field;
        AttackInfo inductive_resonance;
    } boss;

    struct {
        AttackInfo overload;
    } extra;

    // required for iteration
    AttackInfo null;
} stage5_spells;

extern StageProcs stage5_procs;
extern StageProcs stage5_spell_procs;

#endif
