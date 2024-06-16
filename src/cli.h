/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
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
	CLI_QuitLate,
	CLI_Credits,
	CLI_Cutscene,
} CLIActionType;

typedef struct CLIAction CLIAction;
struct CLIAction {
	char *filename;
	char *out_replay;
	PlayerMode *plrmode;
	CLIActionType type;
	int stageid;
	int diff;
	int frameskip;
	CutsceneID cutscene;
	bool force_intro;
	bool unlock_all;
	int width;
	int height;
};

int cli_args(int argc, char **argv, CLIAction *a);
void free_cli_action(CLIAction *a);
