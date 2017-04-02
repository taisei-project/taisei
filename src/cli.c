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
		tsfprintf(stdout, "  -%c, --%s ",opt->opt.val,opt->opt.name);
		int length = margin-(int)strlen(opt->opt.name);
		if(opt->argname) {
			tsfprintf(stdout, opt->argname);
			length -= (int)strlen(opt->argname);
		}
		for(int i = 0; i < length; i++)
			tsfprintf(stdout, " ");
		if(opt->argname)
			tsfprintf(stdout, opt->help, opt->argname);
		else
			tsfprintf(stdout, opt->help);
		tsfprintf(stdout,"\n");
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
		{{"dumpstages", no_argument, 0, 'u'}, "Print a list of all stages in the game", 0},
		{{"dumprestables", no_argument, 0, 't'}, "Print information about the resource hashtables", 0},
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
		}
	}
	*ptr = 0;


	int c;
	int stageid = -1;
	Character cha = -1;
	ShotMode shot = -1;
	while((c = getopt_long(argc, argv, optc ,opts, 0)) != -1) {
		char *endptr;
		switch(c) {
		case 'h':
		case '?':
			print_help(taisei_opts);
			a->type = CLI_Quit;
			break;
		case 'r':
			a->type = CLI_PlayReplay;
			a->filename = strdup(optarg);
			break;
		case 'p':
			a->type = CLI_SelectStage;
			break;
		case 'i':
			stageid = strtol(optarg,&endptr, 16);
			if(*optarg == 0 || *endptr != 0)
				log_fatal("stage id '%s' is not a number", optarg);
			break;
		case 'u':
			a->type = CLI_DumpStages;
			break;
		case 't':
			a->type = CLI_DumpResTables;
			break;
		case 'd':
			for(int i = D_Easy ; i <= NUM_SELECTABLE_DIFFICULTIES; i++) {
				if(strcmp(optarg,difficulty_name(i)) == 0) {
					a->diff = i;
					break;
				}
			}
			break;
		case 's':
			if(plrmode_parse(optarg,&cha,&shot))
				log_fatal("Invalid shotmode '%s'",optarg);
			break;
		default:
			log_fatal("Unknown option (this shouldn’t happen)");
		}
	}

	if(stageid != -1 && a->type != CLI_PlayReplay && a->type != CLI_SelectStage)
		log_warn("--sid was ignored");
	if(stageid != -1 && !stage_get(stageid))
		log_fatal("Invalid stage id: %x",stageid);

	if(a->type != CLI_SelectStage && (cha != -1 || shot != -1))
		log_warn("--shotmode was ignored");

	if(cha != -1 && shot != -1) {
		a->plrcha = cha;
		a->plrshot = shot;
	}

	a->stageid = stageid;

	if(a->type == CLI_SelectStage && stageid == -1)
		log_fatal("StageSelect mode, but no stage id was given");

	return 0;
}

void free_cli_action(CLIAction *a) {
	free(a->filename);
}
