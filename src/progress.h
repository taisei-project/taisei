/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdbool.h>
#include <SDL.h>
#include "ending.h"

#define PROGRESS_FILE "storage/progress.dat"
#define PROGRESS_MAXFILESIZE 4096

#ifdef DEBUG
    // #define PROGRESS_UNLOCK_ALL
#endif

typedef enum ProgfileCommand {
// do not reorder this!

    PCMD_UNLOCK_STAGES                     = 0x00,
    PCMD_UNLOCK_STAGES_WITH_DIFFICULTY     = 0x01,
    PCMD_HISCORE                           = 0x02,
    PCMD_STAGE_PLAYINFO                    = 0x03,
    PCMD_ENDINGS                           = 0x04,
} ProgfileCommand;

typedef struct StageProgress {
    // keep this struct small if you can
    // see stage_get_progress_from_info() in stage.c for more information

    unsigned int unlocked : 1;

    uint32_t num_played;
    uint32_t num_cleared;
} StageProgress;

struct UnknownCmd;

typedef struct GlobalProgress {
    uint32_t hiscore;
    uint32_t achieved_endings[NUM_ENDINGS];
    struct UnknownCmd *unknown;
} GlobalProgress;

extern GlobalProgress progress;

void progress_load(void);
void progress_save(void);
void progress_unload(void);

#endif
