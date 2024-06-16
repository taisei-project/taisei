/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "tsrtool.h"

#include "replay.h"
#include "stage.h"
#include "struct.h"

#include "plrmodes.h"
#include "util.h"
#include "util/strbuf.h"

typedef struct Command {
	const char *name;
	int min_args;
	int (*func)(int argc, char **argv);
	const char *usage;
} Command;

static struct {
	Replay rpy;
	uint16_t save_version;
	ReplayStage *active_stage;
} G = {
	.save_version = REPLAY_STRUCT_VERSION_WRITE,
};

static int cmd_reset(int argc, char **argv) {
	replay_reset(&G.rpy);
	G.active_stage = NULL;
	return 0;
}

static int cmd_load(int argc, char **argv) {
	replay_reset(&G.rpy);

	if(!replay_load_syspath(&G.rpy, argv[1], REPLAY_READ_ALL | REPLAY_READ_IGNORE_ERRORS)) {
		return -1;
	}

	return 1;
}

static int cmd_version(int argc, char **argv) {
	++argv;

	if(!strcmp(*argv, "default")) {
		G.save_version = REPLAY_STRUCT_VERSION_WRITE;
	} else {
		char *endp = *argv;
		G.save_version = strtol(*argv, &endp, 10);

		while(isspace(*endp)) {
			++endp;
		}

		if(strcmp(endp, "u")) {
			G.save_version |= REPLAY_VERSION_COMPRESSION_BIT;
		}
	}

	return 1;
}

static int cmd_save(int argc, char **argv) {
	if(!replay_save_syspath(&G.rpy, argv[1], G.save_version)) {
		return -1;
	}

	return 1;
}

#define ACTIVE_STAGE ({ \
	auto _stg = G.active_stage; \
	if(!_stg) { \
		log_error("No stage selected"); \
		return -1; \
	} \
	_stg; \
})

static int cmd_stage(int argc, char **argv) {
	int n = strtol(argv[1], NULL, 10);

	if(n >= G.rpy.stages.num_elements || n < 0) {
		log_error("Stage number %i out of range; %i loaded", n, G.rpy.stages.num_elements);
		return -1;
	}

	G.active_stage = dynarray_get_ptr(&G.rpy.stages, n);
	return 1;
}

static bool filter_drop(const void *pelem, void *dropelem) {
	return pelem != dropelem;
}

static int cmd_drop(int argc, char **argv) {
	auto stg = ACTIVE_STAGE;
	dynarray_filter(&G.rpy.stages, filter_drop, stg);
	G.active_stage = NULL;
	return 0;
}

static bool filter_isolate(const void *pelem, void *wantelem) {
	return pelem == wantelem;
}

static int cmd_isolate(int argc, char **argv) {
	auto stg = ACTIVE_STAGE;
	dynarray_filter(&G.rpy.stages, filter_isolate, stg);
	assert(G.rpy.stages.num_elements == 1);
	G.active_stage = dynarray_get_ptr(&G.rpy.stages, 0);
	return 0;
}

static int cmd_skip(int argc, char **argv) {
	auto stg = ACTIVE_STAGE;
	stg->skip_frames = strtol(argv[1], NULL, 10);
	return 1;
}

static int cmd_trim(int argc, char **argv) {
	auto stg = ACTIVE_STAGE;
	int endframe = strtol(argv[1], NULL, 10);

	dynarray_foreach(&stg->events, int i, ReplayEvent *evt, {
		if(evt->frame >= endframe) {
			stg->events.num_elements = i;
			break;
		}
	});

	replay_stage_event(stg, endframe, EV_OVER, 0);
	stg->num_events = stg->events.num_elements;

	return 1;
}

#define APPEND_FLAG_SEPARATOR() \
	if(first) { \
		first = false; \
	} else { \
		strbuf_cat(sbuf, " | "); \
	}

#define APPEND_FLAG_NAME(name, bit) \
	if(flags & (1 << bit)) { \
		APPEND_FLAG_SEPARATOR() \
		strbuf_cat(sbuf, #name); \
		flags &= ~(1 << bit); \
	} \

static void dump_gflags(StringBuffer *sbuf, uint32_t flags) {
	bool first = true;

	#define REPLAY_GFLAG(name, bit) \
		APPEND_FLAG_NAME(REPLAY_GFLAG_##name, bit)

	REPLAY_GFLAGS
	#undef REPLAY_GFLAG

	if(flags) {
		APPEND_FLAG_SEPARATOR()
		strbuf_printf(sbuf, "0x%X", flags);
	}
}

static void dump_sflags(StringBuffer *sbuf, uint32_t flags) {
	bool first = true;

	#define REPLAY_SFLAG(name, bit) \
		APPEND_FLAG_NAME(REPLAY_SFLAG_##name, bit)

	REPLAY_SFLAGS
	#undef REPLAY_SFLAG

	if(flags) {
		APPEND_FLAG_SEPARATOR()
		strbuf_printf(sbuf, "0x%X", flags);
	}
}

static void dump_inflags(StringBuffer *sbuf, uint32_t flags) {
	bool first = true;

	#define INFLAG(name, bit) \
		APPEND_FLAG_NAME(INFLAG_##name, bit)

	INFLAGS
	#undef INFLAG

	if(flags) {
		APPEND_FLAG_SEPARATOR()
		strbuf_printf(sbuf, "0x%X", flags);
	}
}

#undef APPEND_FLAG_SEPARATOR
#undef APPEND_FLAG_NAME

static void flushline(StringBuffer *sbuf) {
	fputs(sbuf->start, stdout);
	fputc('\n', stdout);
	strbuf_clear(sbuf);
}

static int cmd_info(int argc, char **argv) {
	StringBuffer sbuf = {};

	strbuf_printf(&sbuf,
		"Struct version: %i %scompressed",
		G.rpy.version & ~REPLAY_VERSION_COMPRESSION_BIT,
		(G.rpy.version & REPLAY_VERSION_COMPRESSION_BIT) ? "" : "un"
	);
	flushline(&sbuf);

	strbuf_cat(&sbuf, "Game version: ");
	taisei_version_tostrbuf(&sbuf, &G.rpy.game_version);
	flushline(&sbuf);

	strbuf_printf(&sbuf, "Player: %s", G.rpy.playername);
	flushline(&sbuf);

	strbuf_printf(&sbuf, "Flags: 0x%04X : ", G.rpy.flags);
	dump_gflags(&sbuf, G.rpy.flags);
	flushline(&sbuf);

	strbuf_printf(&sbuf, "Stages: %u", G.rpy.stages.num_elements);
	flushline(&sbuf);

	dynarray_foreach(&G.rpy.stages, int stgnum, ReplayStage *stg, {
		char tmp[128];

		strbuf_printf(&sbuf, "\nStage #%i", stgnum);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "ID: 0x%04X", stg->stage);
		flushline(&sbuf);

		StageInfo *si = stageinfo_get_by_id(stg->stage);

		if(si) {
			strbuf_printf(&sbuf, "Title: %s - %s", si->title, si->subtitle);
		} else {
			strbuf_cat(&sbuf, "Title: <unknown>");
		}

		flushline(&sbuf);

		PlayerMode *pm = plrmode_find(stg->plr_char, stg->plr_shot);

		if(pm) {
			plrmode_repr(tmp, sizeof(tmp), pm, false);
			strbuf_printf(&sbuf, "Character: %s", tmp);
		} else {
			strbuf_printf(&sbuf,
				"Character: Unknown (0x%02X, 0x%02X)", stg->plr_char, stg->plr_shot);
		}

		flushline(&sbuf);

		strbuf_printf(&sbuf, "Difficulty: %s", difficulty_name(stg->diff));
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Flags: 0x%04X : ", stg->flags);
		dump_sflags(&sbuf, stg->flags);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Random seed: 0x%016llx", (unsigned long long)stg->rng_seed);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Skip frames: %u", stg->skip_frames);
		flushline(&sbuf);

		time_t t = stg->start_time;
		struct tm *timeinfo = localtime(&t);
		strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M", timeinfo);
		strbuf_printf(&sbuf, "Starting time: %s", tmp);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting position: (%i, %i)", stg->plr_pos_x, stg->plr_pos_y);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting power: %i", stg->plr_power);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting lives: %i, %i fragments", stg->plr_lives, stg->plr_life_fragments);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting bombs: %i, %i fragments", stg->plr_bombs, stg->plr_bomb_fragments);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting graze: %u", stg->plr_graze);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting PIV: %u", stg->plr_point_item_value);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting input: 0x%04X : ", stg->plr_inputflags);
		dump_inflags(&sbuf, stg->plr_inputflags);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting total lives used: %u", stg->plr_total_lives_used);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting total bombs used: %u", stg->plr_total_bombs_used);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting total continues used: %u", stg->plr_total_continues_used);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Starting score: %llu", (unsigned long long)stg->plr_points);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Final score: %llu", (unsigned long long)stg->plr_points_final);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Final lives used (this stage): %u", stg->plr_stage_lives_used_final);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Final bombs used (this stage): %u", stg->plr_stage_bombs_used_final);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Final continues used (this stage): %u", stg->plr_stage_continues_used_final);
		flushline(&sbuf);

		strbuf_printf(&sbuf, "Events: %u", stg->events.num_elements);
		flushline(&sbuf);
	});

	strbuf_free(&sbuf);
	return 0;
}

static Command commands[] = {
	{ "reset",              0, cmd_reset, },
	{ "load",               1, cmd_load,           "load <filename.tsr>" },
	{ "save",               1, cmd_save,           "save <filename.tsr>" },
	{ "version",            1, cmd_version,        "version <default>|(<version>[u])" },
	{ "stage",              1, cmd_stage,          "stage <num>" },
	{ "skip",               1, cmd_skip,           "skip <numframes>" },
	{ "trim",               1, cmd_trim,           "trim <numframes>" },
	{ "drop",               0, cmd_drop, },
	{ "isolate",            0, cmd_isolate, },
	{ "info",               0, cmd_info, },
};

int tsrtool_main(int argc, char **argv) {
	char **end = argv + argc;
	++argv;

	while(argv < end) {
		bool found = false;

		for(int i = 0; i < ARRAY_SIZE(commands); ++i) {
			if(!strcmp(*argv, commands[i].name)) {
				found = true;

				if(end - argv - 1 < commands[i].min_args) {
					log_error(
						"Not enough arguments to '%s'. Usage: %s",
						*argv, commands[i].usage
					);
					return 2;
				}

				int result = commands[i].func(end - argv, argv);

				if(result < 0) {
					return -result;
				}

				argv += 1 + result;

				break;
			}
		}

		if(!found) {
			log_error("No such command: %s", *argv);
			return 3;
		}
	}

	return 0;
}
