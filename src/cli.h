#ifndef CLI_H
#define CLI_H

#include "player.h"

typedef enum {
	CLI_RunNormally = 0,
	CLI_PlayReplay,
	CLI_SelectStage,
	CLI_DumpStages,
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

#endif
