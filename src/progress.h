/*
* This software is licensed under the terms of the MIT-License
* See COPYING for further information.
* ---
* Copyright (C) 2011, Lukas Weber <laochailan@web.de>
*/

#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdbool.h>
#include <SDL.h>

#define PROGRESS_FILENAME "progress.dat"
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
    struct UnknownCmd *unknown;
} GlobalProgress;

extern GlobalProgress progress;

void progress_load(void);
void progress_save(void);
void progress_unload(void);

#endif
