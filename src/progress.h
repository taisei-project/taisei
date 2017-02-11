/*
* This software is licensed under the terms of the MIT-License
* See COPYING for further information.
* ---
* Copyright (C) 2011, Lukas Weber <laochailan@web.de>
*/

#ifndef PROGRESS_H
#define PROGRESS_H

#include "global.h"

#define PROGRESS_FILENAME "progress.dat"
#define PROGRESS_MAXFILESIZE 4096

#ifdef DEBUG
	// #define PROGRESS_UNLOCK_ALL
#endif

typedef enum ProgfileCommand {
	PCMD_UNLOCK_STAGES,
} ProgfileCommand;

void progress_load(void);
void progress_save(void);

#endif
