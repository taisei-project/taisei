/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_cli_h
#define IGUARD_cli_h

#include "taisei.h"

#include "plrmodes.h"

typedef enum {
	CLI_RunNormally = 0,
	CLI_PlayReplay,
	CLI_VerifyReplay,
	CLI_SelectStage,
	CLI_DumpStages,
	CLI_DumpVFSTree,
	CLI_Quit,
	CLI_Credits,
	CLI_Cutscene,
} CLIActionType;

typedef struct CLIAction CLIAction;
struct CLIAction {
	CLIActionType type;
	bool force_intro;
	int stageid;
	int diff;
	int frameskip;
	CutsceneID cutscene;
	char *filename;
	char *out_replay;
	PlayerMode *plrmode;
};

int cli_args(int argc, char **argv, CLIAction *a);
void free_cli_action(CLIAction *a);

#endif // IGUARD_cli_h
