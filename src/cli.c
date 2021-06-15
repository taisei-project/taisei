/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <getopt.h>

#include "cli.h"
#include "difficulty.h"
#include "util.h"
#include "log.h"
#include "stage.h"
#include "plrmodes.h"
#include "version.h"
#include "cutscenes/cutscene.h"
#include "cutscenes/scenes.h"

struct TsOption { struct option opt; const char *help; const char *argname; };

enum {
	OPT_RENDERER = INT_MIN,
	OPT_CUTSCENE,
	OPT_CUTSCENE_LIST,
	OPT_FORCE_INTRO,
	OPT_REREPLAY,
};

static void print_help(struct TsOption* opts) {
	tsfprintf(stdout, "Usage: taisei [OPTIONS]\nTaisei is an open source Tōhō Project fangame.\n\nOptions:\n");
	int margin = 20;
	for(struct TsOption *opt = opts; opt->opt.name; opt++) {
		if(opt->opt.val > 0) {
			tsfprintf(stdout, "  -%c, --%s ", opt->opt.val, opt->opt.name);
		} else {
			tsfprintf(stdout, "      --%s ", opt->opt.name);
		}

		int length = margin-(int)strlen(opt->opt.name);

		if(opt->argname) {
			tsfprintf(stdout, "%s", opt->argname);
			length -= (int)strlen(opt->argname);
		}

		for(int i = 0; i < length; i++) {
			fputc(' ', stdout);
		}

		fputs("  ", stdout);

		if(opt->argname && strchr(opt->help, '%')) {
			tsfprintf(stdout, opt->help, opt->argname);
		} else {
			tsfprintf(stdout, "%s", opt->help);
		}

		tsfprintf(stdout, "\n");
	}
}

int cli_args(int argc, char **argv, CLIAction *a) {
	const char *const _renderer_list =
	#define R(r) ","#r
	TAISEI_BUILDCONF_RENDERER_BACKENDS
	#undef R
	;

	char renderer_list[strlen(_renderer_list) + 2];
	snprintf(renderer_list, sizeof(renderer_list), "{%s}", _renderer_list+1);

	struct TsOption taisei_opts[] = {
		{{"replay",             required_argument,  0, 'r'},            "Play a replay from %s", "FILE"},
		{{"verify-replay",      required_argument,  0, 'R'},            "Play a replay from %s in headless mode, crash as soon as it desyncs unless --rereplay is used", "FILE"},
		{{"rereplay",           required_argument,  0, OPT_REREPLAY},   "Re-record replay into %s; specify input with -r or -R", "OUTFILE"},
#ifdef DEBUG
		{{"play",               no_argument,        0, 'p'},            "Play a specific stage"},
		{{"sid",                required_argument,  0, 'i'},            "Select stage by %s", "ID"},
		{{"diff",               required_argument,  0, 'd'},            "Select a difficulty (Easy/Normal/Hard/Lunatic)", "DIFF"},
		{{"shotmode",           required_argument,  0, 's'},            "Select a shotmode (marisaA/youmuA/marisaB/youmuB)", "SMODE"},
		{{"dumpstages",         no_argument,        0, 'u'},            "Print a list of all stages in the game"},
		{{"vfs-tree",           required_argument,  0, 't'},            "Print the virtual filesystem tree starting from %s", "PATH"},
		{{"cutscene",           required_argument,  0, OPT_CUTSCENE},   "Play cutscene by numeric %s and exit", "ID"},
		{{"list-cutscenes",     no_argument,        0, OPT_CUTSCENE_LIST}, "List all registered cutscenes with their numeric IDs and names, then exit" },
		{{"intro",              no_argument,        0, OPT_FORCE_INTRO}, "Play the intro cutscene even if already seen"},
		{{"skip-to-bookmark",   required_argument,  0, 'b'},            "Fast-forward stage to a specific STAGE_BOOKMARK call"},
#endif
		{{"frameskip",          optional_argument,  0, 'f'},            "Disable FPS limiter, render only every %s frame", "FRAME"},
		{{"credits",            no_argument,        0, 'c'},            "Show the credits scene and exit"},
		{{"renderer",           required_argument,  0, OPT_RENDERER},   "Choose the rendering backend", renderer_list},
		{{"help",               no_argument,        0, 'h'},            "Print help and exit"},
		{{"version",            no_argument,        0, 'v'},            "Print version and exit"},
		{ 0 }
	};

	memset(a, 0, sizeof(*a));

	int nopts = ARRAY_SIZE(taisei_opts);
	struct option opts[nopts];
	char optc[2*nopts+1];
	char *ptr = optc;

	for(int i = 0; i < nopts; i++) {
		opts[i] = taisei_opts[i].opt;

		if(opts[i].val <= 0) {
			continue;
		}

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
	PlayerMode *plrmode = NULL;

	while((c = getopt_long(argc, argv, optc, opts, 0)) != -1) {
		char *endptr = NULL;

		switch(c) {
		case 'h':
		case '?':
			print_help(taisei_opts);
			// a->type = CLI_Quit;
			exit(0);
			break;
		case 'r':
			a->type = CLI_PlayReplay;
			a->filename = strdup(optarg);
			break;
		case 'R':
			a->type = CLI_VerifyReplay;
			a->filename = strdup(optarg);
			break;
		case OPT_REREPLAY:
			a->out_replay = strdup(optarg);
			env_set("TAISEI_REPLAY_DESYNC_CHECK_FREQUENCY", 1, false);
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
				char *end;
				int dval = strtol(optarg, &end, 10);
				if(dval >= D_Easy && dval <= D_Lunatic && end == optarg + strlen(optarg)) {
					a->diff = dval;
				}
			}

			if(a->diff == D_Any) {
				log_fatal("Invalid difficulty '%s'", optarg);
			}

			break;
		case 's':
			if(!(plrmode = plrmode_parse(optarg)))
				log_fatal("Invalid shotmode '%s'", optarg);
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
		case 'c':
			a->type = CLI_Credits;
			break;
		case OPT_RENDERER:
			env_set("TAISEI_RENDERER", optarg, true);
			break;
		case OPT_CUTSCENE:
			a->type = CLI_Cutscene;
			a->cutscene = strtol(optarg, &endptr, 16);

			if(!*optarg || endptr == optarg || (uint)a->cutscene >= NUM_CUTSCENE_IDS) {
				log_fatal("Invalid cutscene ID '%s'", optarg);
			}

			break;
		case OPT_CUTSCENE_LIST:
			for(CutsceneID i = 0; i < NUM_CUTSCENE_IDS; ++i) {
				const Cutscene *cs = g_cutscenes + i;
				tsfprintf(stdout, "%2d : %s\n", i, cs->phases ? cs->name : "!! UNIMPLEMENTED !!");
			}

			exit(0);
		case OPT_FORCE_INTRO:
			a->force_intro = true;
			break;
		case 'b':
			env_set("TAISEI_SKIP_TO_BOOKMARK", optarg, true);
			break;
		case 'v':
			tsfprintf(stdout, "%s %s\n", TAISEI_VERSION_FULL, TAISEI_VERSION_BUILD_TYPE);
			exit(0);
		default:
			UNREACHABLE;
		}
	}

	if(stageid) {
		switch(a->type) {
			case CLI_PlayReplay:
			case CLI_VerifyReplay:
			case CLI_SelectStage:
				if(stageinfo_get_by_id(stageid) == NULL) {
					log_fatal("Invalid stage id: %X", stageid);
				}
				break;

			default:
				log_warn("--sid was ignored");
				break;
		}
	}

	if(plrmode) {
		if(a->type == CLI_SelectStage) {
			a->plrmode = plrmode;
		} else {
			log_warn("--shotmode was ignored");
		}
	}

	a->stageid = stageid;

	if(a->type == CLI_SelectStage && !stageid) {
		log_fatal("StageSelect mode, but no stage id was given");
	}

	if(a->out_replay && a->type != CLI_PlayReplay && a->type != CLI_VerifyReplay) {
		log_fatal("--rereplay requires --replay or --verify-replay");
	}

	return 0;
}

void free_cli_action(CLIAction *a) {
	free(a->filename);
	a->filename = NULL;
	free(a->out_replay);
	a->out_replay = NULL;
}
