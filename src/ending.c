/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "ending.h"
#include "global.h"
#include "video.h"
#include "progress.h"

typedef struct EndingEntry {
	char *msg;
	Sprite *sprite;
	int time;
} EndingEntry;

typedef struct Ending {
	EndingEntry *entries;
	int count;
	int duration;
	int pos;
	CallChain cc;
} Ending;

static void track_ending(int ending) {
	assert(ending >= 0 && ending < NUM_ENDINGS);
	progress.achieved_endings[ending]++;
}

static void add_ending_entry(Ending *e, int dur, const char *msg, const char *sprite) {
	EndingEntry *entry;
	e->entries = realloc(e->entries, (++e->count)*sizeof(EndingEntry));
	entry = &e->entries[e->count-1];

	entry->time = 0;

	if(e->count > 1) {
		entry->time = 1<<23; // nobody will ever! find out
	}

	entry->msg = NULL;
	stralloc(&entry->msg, msg);

	if(sprite) {
		entry->sprite = get_sprite(sprite);
	} else {
		entry->sprite = NULL;
	}
}

/*
 * These ending callbacks are referenced in plrmodes/ code
 */

void bad_ending_marisa(Ending *e) {
	add_ending_entry(e, -1, "FIXME: Removed due to issue #179", NULL);
	add_ending_entry(e, 300, "-BAD END-\nTry a clear without continues next time!", NULL);
	track_ending(ENDING_BAD_1);
}

void bad_ending_youmu(Ending *e) {
	add_ending_entry(e, -1, "FIXME: Removed due to issue #179", NULL);
	add_ending_entry(e, -1, "-BAD END-\nTry a clear without continues next time!", NULL);
	track_ending(ENDING_BAD_2);
}

void bad_ending_reimu(Ending *e) {
	add_ending_entry(e, -1, "FIXME: Removed due to issue #179", NULL);
	add_ending_entry(e, -1, "-BAD END-\nTry a clear without continues next time!", NULL);
	track_ending(ENDING_BAD_3);
}

void good_ending_marisa(Ending *e) {
	add_ending_entry(e, -1, "FIXME: Removed due to issue #179", NULL);
	add_ending_entry(e, -1, "-GOOD END-\nExtra Stage has opened up for you! Do you have what it takes to discover the hidden truth behind everything?", NULL);

	track_ending(ENDING_GOOD_1);
}

void good_ending_youmu(Ending *e) {
	add_ending_entry(e, -1, "FIXME: Removed due to issue #179", NULL);
	add_ending_entry(e, -1, "-GOOD END-\nExtra Stage has opened up for you! Do you have what it takes to discover the hidden truth behind everything?", NULL);
	track_ending(ENDING_GOOD_2);
}

void good_ending_reimu(Ending *e) {
	add_ending_entry(e, -1, "FIXME: Removed due to issue #179", NULL);
	add_ending_entry(e, -1, "-GOOD END-\nExtra Stage has opened up for you! Do you have what it takes to discover the hidden truth behind everything?", NULL);
	track_ending(ENDING_GOOD_3);
}

static void init_ending(Ending *e) {
	if(global.plr.continues_used) {
		global.plr.mode->character->ending.bad(e);
	} else {
		global.plr.mode->character->ending.good(e);
		add_ending_entry(e, 400, "Sorry, extra stage isn’t done yet. ^^", NULL);
	}

	add_ending_entry(e, 400, "", NULL); // this is important
	e->duration = 1<<23;
}

static void free_ending(Ending *e) {
	int i;
	for(i = 0; i < e->count; i++)
		free(e->entries[i].msg);
	free(e->entries);
}

static void ending_draw(Ending *e) {
	float s, d;

	int t1 = global.frames-e->entries[e->pos].time;
	int t2 = e->entries[e->pos+1].time-global.frames;

	d = 1.0/ENDING_FADE_TIME;

	if(t1 < ENDING_FADE_TIME)
		s = clamp(d*t1, 0.0, 1.0);
	else
		s = clamp(d*t2, 0.0, 1.0);

	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 1), 1);

	r_color4(s, s, s, s);

	if(e->entries[e->pos].sprite) {
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = e->entries[e->pos].sprite,
			.pos = { SCREEN_W/2, SCREEN_H/2 },
			.shader = "sprite_default",
			.color = r_color_current(),
		});
	}

	r_shader("text_default");
	text_draw_wrapped(e->entries[e->pos].msg, SCREEN_W * 0.85, &(TextParams) {
		.pos = { SCREEN_W/2, VIEWPORT_H*4/5 },
		.align = ALIGN_CENTER,
	});
	r_shader_standard();

	r_color4(1,1,1,1);
}

void ending_preload(void) {
	preload_resource(RES_BGM, "ending", RESF_OPTIONAL);
}

static void ending_advance(Ending *e) {
	if(
		e->pos < e->count-1 &&
		e->entries[e->pos].time + ENDING_FADE_TIME < global.frames &&
		global.frames < e->entries[e->pos+1].time - ENDING_FADE_TIME
	) {
		e->entries[e->pos+1].time = global.frames+(e->pos != e->count-2)*ENDING_FADE_TIME;
	}
}

static bool ending_input_handler(SDL_Event *event, void *arg) {
	Ending *e = arg;

	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(code == KEY_SHOT) {
				ending_advance(e);
			}
			break;
		default:
			break;
	}
	return false;
}

static LogicFrameAction ending_logic_frame(void *arg) {
	Ending *e = arg;

	update_transition();
	events_poll((EventHandler[]){
		{ .proc = ending_input_handler, .arg=e },
		{ NULL }
	}, EFLAG_GAME);

	global.frames++;

	if(e->pos < e->count-1 && global.frames >= e->entries[e->pos+1].time) {
		e->pos++;
		if(e->pos == e->count-1) {
			fade_bgm((FPS * ENDING_FADE_OUT) / 4000.0);
			set_transition(TransFadeWhite, ENDING_FADE_OUT, ENDING_FADE_OUT);
			e->duration = global.frames+ENDING_FADE_OUT;
		}

	}

	if(global.frames >= e->duration) {
		return LFRAME_STOP;
	}

	if(gamekeypressed(KEY_SKIP)) {
		ending_advance(e);
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static RenderFrameAction ending_render_frame(void *arg) {
	Ending *e = arg;

	if(e->pos < e->count-1)
		ending_draw(e);

	draw_transition();
	return RFRAME_SWAP;
}

static void ending_loop_end(void *ctx) {
	Ending *e = ctx;
	CallChain cc = e->cc;
	free_ending(e);
	free(e);
	progress_unlock_bgm("ending");
	run_call_chain(&cc, NULL);
}

void ending_enter(CallChain next) {
	ending_preload();

	Ending *e = calloc(1, sizeof(Ending));
	init_ending(e);
	e->cc = next;

	global.frames = 0;
	set_ortho(SCREEN_W, SCREEN_H);
	start_bgm("ending");

	eventloop_enter(e, ending_logic_frame, ending_render_frame, ending_loop_end, FPS);
}
