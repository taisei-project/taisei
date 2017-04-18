#include "cli.h"
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include "difficulty.h"
#include "util.h"
#include "log.h"
#include "stage.h"
#include "plrmodes.h"

struct TsOption { struct option opt; const char *help; const char *argname;};

static void print_help(struct TsOption* opts) {
	tsfprintf(stdout, "Usage: taisei [OPTIONS]\nTaisei is an open source Touhou clone.\n\nOptions:\n");
	int margin = 20;
	for(struct TsOption *opt = opts; opt->opt.name; opt++) {
		tsfprintf(stdout, "  -%c, --%s ", opt->opt.val,opt->opt.name);
		int length = margin-(int)strlen(opt->opt.name);
		if(opt->argname) {
			tsfprintf(stdout, "%s", opt->argname);
			length -= (int)strlen(opt->argname);
		}
		for(int i = 0; i < length; i++)
			tsfprintf(stdout, " ");
		if(opt->argname)
			tsfprintf(stdout, opt->help, opt->argname);
		else
			tsfprintf(stdout, "%s", opt->help);
		tsfprintf(stdout, "\n");
	}
}

int cli_args(int argc, char **argv, CLIAction *a) {
	struct TsOption taisei_opts[] =
		{{{"replay", required_argument, 0, 'r'}, "Play a replay from %s", "FILE"},
#ifdef DEBUG
		{{"play", no_argument, 0, 'p'}, "Play a specific stage", 0},
		{{"sid", required_argument, 0, 'i'}, "Select stage by %s", "ID"},
		{{"diff", required_argument, 0, 'd'}, "Select a difficulty (Easy/Normal/Hard/Lunatic)", "DIFF"},
	/*	{{"sname", required_argument, 0, 'n'}, "Select stage by %s", "NAME"},*/
		{{"shotmode", required_argument, 0, 's'}, "Select a shotmode (marisaA/youmuA/marisaB/youmuB)", "SMODE"},
		{{"frameskip", optional_argument, 0, 'f'}, "Disable FPS limiter, render only every %s frame", "FRAME"},
		{{"dumpstages", no_argument, 0, 'u'}, "Print a list of all stages in the game", 0},
		{{"vfs-tree", required_argument, 0, 't'}, "Print the virtual filesystem tree starting from %s", "PATH"},
#endif
		{{"help", no_argument, 0, 'h'}, "Display this help."},
		{{0,0,0,0},0,0}
	};

	memset(a,0,sizeof(CLIAction));

	int nopts = sizeof(taisei_opts)/sizeof(taisei_opts[0]);
	struct option opts[nopts];
	char optc[2*nopts+1];
	char *ptr = optc;

	for(int i = 0; i < nopts; i++) {
		opts[i] = taisei_opts[i].opt;
		*ptr = opts[i].val;
		ptr++;

		if(opts[i].has_arg != no_argument) {
			*ptr = ':';
			ptr++;

			if(opts[i].has_arg == optional_argument) {
				*ptr = ':';
				ptr++;
			}
		}
	}
	*ptr = 0;

	// on OS X, programs get passed some strange parameter when they are run from bundles.
	for(int i = 0; i < argc; i++) {
		if(strstartswith(argv[i],"-psn_"))
			argv[i][0] = 0;
	}

	int c;
	uint16_t stageid = 0;

	#define INVALID_CHAR ((Character)-1)
	#define INVALID_SHOT ((ShotMode)-1)

	// these may be unsigned depending on the compiler.
	Character cha = INVALID_CHAR;
	ShotMode shot = INVALID_SHOT;

	while((c = getopt_long(argc, argv, optc, opts, 0)) != -1) {
		char *endptr = NULL;

		switch(c) {
		case 'h':
		case '?':
			print_help(taisei_opts);
			// a->type = CLI_Quit;
			exit(1);
			break;
		case 'r':
			a->type = CLI_PlayReplay;
			a->filename = strdup(optarg);
			break;
		case 'p':
			a->type = CLI_SelectStage;
			break;
		case 'i':
			stageid = strtol(optarg, &endptr, 16);
			if(!*optarg || endptr == optarg)
				log_fatal("Stage id '%s' is not a number", optarg);
			break;
		case 'u':
			a->type = CLI_DumpStages;
			break;
		case 'd':
			a->diff = D_Any;
			for(int i = D_Easy ; i <= NUM_SELECTABLE_DIFFICULTIES; i++) {
				if(strcasecmp(optarg, difficulty_name(i)) == 0) {
					a->diff = i;
					break;
				}
			}

			if(a->diff == D_Any) {
				log_fatal("Invalid difficulty '%s'", optarg);
			}

			break;
		case 's':
			if(plrmode_parse(optarg,&cha,&shot))
				log_fatal("Invalid shotmode '%s'",optarg);
			break;
		case 'f':
			a->frameskip = 1;

			if(optarg) {
				a->frameskip = strtol(optarg, &endptr, 10);

				if(endptr == optarg) {
					log_fatal("Frameskip value '%s' is not a number", optarg);
				}

				if(a->frameskip < 0) {
					a->frameskip = INT_MAX;
				}
			}
			break;
		case 't':
			a->type = CLI_DumpVFSTree,
			a->filename = strdup(optarg ? optarg : "");
			break;
		default:
			log_fatal("Unknown option (this shouldn’t happen)");
		}
	}

	if(stageid) {
		if(a->type != CLI_PlayReplay && a->type != CLI_SelectStage) {
			log_warn("--sid was ignored");
		} else if(!stage_get(stageid)) {
			log_fatal("Invalid stage id: %x", stageid);
		}
	}

	if(a->type != CLI_SelectStage && (cha != INVALID_CHAR || shot != INVALID_SHOT))
		log_warn("--shotmode was ignored");

	if(cha != INVALID_CHAR && shot != INVALID_SHOT) {
		a->plrcha = cha;
		a->plrshot = shot;
	}

	a->stageid = stageid;

	if(a->type == CLI_SelectStage && !stageid)
		log_fatal("StageSelect mode, but no stage id was given");

	return 0;
}

void free_cli_action(CLIAction *a) {
	free(a->filename);
}
