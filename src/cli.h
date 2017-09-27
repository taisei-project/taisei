/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "player.h"

typedef enum {
	CLI_RunNormally = 0,
	CLI_PlayReplay,
	CLI_SelectStage,
	CLI_DumpStages,
	CLI_DumpVFSTree,
	CLI_Quit,
} CLIActionType;

typedef struct CLIAction CLIAction;
struct CLIAction {
	CLIActionType type;
	char *filename;
	int stageid;
	int diff;
	int frameskip;

	Character plrcha;
	ShotMode plrshot;
};

int cli_args(int argc, char **argv, CLIAction *a);
void free_cli_action(CLIAction *a);


