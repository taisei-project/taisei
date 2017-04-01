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
	PCMD_UNLOCK_STAGES,
	PCMD_UNLOCK_STAGES_WITH_DIFFICULTY,
    PCMD_HISCORE,
} ProgfileCommand;

typedef struct StageProgress {
	// keep this struct small if you can, it leaks
	// see stage_get_progress_from_info() in stage.c for more information

	bool unlocked;
	// short num_played;
	// short num_cleared;
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
