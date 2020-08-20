/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "cutscene.h"
#include "renderer/api.h"
#include "color.h"
#include "global.h"
#include "video.h"

#define MAX_TEXT_ENTRIES 6

typedef enum CutsceneTextType {
	CTT_INVALID,
	CTT_NARRATION,
	CTT_DIALOGUE,
	CTT_CENTERED,
} CutsceneTextType;

typedef struct CutscenePhaseTextEntry {
	CutsceneTextType type;
	const char *header;
	const char *text;
	Color color;
} CutscenePhaseTextEntry;

typedef struct CutscenePhase {
	const char *background;
	CutscenePhaseTextEntry text_entries[MAX_TEXT_ENTRIES];
} CutscenePhase;

#define SKIP_DELAY 5
#define AUTO_ADVANCE_TIME_BEFORE_TEXT FPS * 2
#define AUTO_ADVANCE_TIME_MID_SCENE   FPS * 20
#define AUTO_ADVANCE_TIME_CROSS_SCENE FPS * 180

#define COLOR_NARRATOR { 0.955, 0.934, 0.745, 1 }
#define COLOR_REIMU    { 0.955, 0.550, 0.500, 1 }
#define COLOR_YUKARI   { 0.550, 0.500, 0.950, 1 }

#define T_NARRATOR(text) { CTT_NARRATION, NULL, text, COLOR_NARRATOR }
#define T_SPEECH(speaker, text, color) { CTT_DIALOGUE, speaker, "“" text "”", color }
#define T_CENTERED(header, text) { CTT_CENTERED, header, text, COLOR_NARRATOR }

#define T_REIMU(text) T_SPEECH("Reimu", text, COLOR_REIMU)
#define T_YUKARI(text) T_SPEECH("Yukari", text, COLOR_YUKARI)

static CutscenePhase test_cutscene[] = {
#if 1
	{ "cutscenes/locations/hakurei", {
		T_NARRATOR("— The Hakurei Shrine"),
		T_NARRATOR("The shrine at the border of fantasy and reality.\n"),
		T_NARRATOR("The Tower had powered down, yet the instigators were nowhere to be found."),
		T_NARRATOR("Reimu returned to her shrine, unsure if she had done anything at all."),
		T_NARRATOR("The Gods and yōkai of the Mountain shuffled about the shrine quietly, with bated breath."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/01", {
		T_NARRATOR("Reimu sighed, and made her announcement.\n"),
		T_REIMU("Listen up! I stopped the spread of madness, but until further notice, Yōkai Mountain is off limits!"),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/01", {
		T_NARRATOR("Nobody was pleased with that.\n"),
		T_NARRATOR("The Moriya Gods were visibly shocked."),
		T_NARRATOR("The kappa demanded that they be able to fetch their equipment and tend to their hydroponic cucumber farms."),
		T_NARRATOR("The tengu furiously scribbled down notes, once again as if they’d had the scoop of the century, before realizing what had just been said, and got up in arms too."),
		{ 0 },
	}},
	{ "", {
		T_NARRATOR("Once Reimu had managed to placate the crowd, she sat in the back of the Hakurei Shrine, bottle of sake in hand.\n"),
		T_NARRATOR("She didn’t feel like drinking, however. She nursed it without even uncorking it."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_YUKARI("A little unsatisfying, isn’t it?"),
		T_NARRATOR("\nReimu was never surprised by Yukari’s sudden appearances anymore."),
		T_NARRATOR("Ever since she had gained ‘gap’ powers of her own during the Urban Legends Incident, Reimu could feel Yukari’s presence long before she made herself known."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_YUKARI("My oh my, it’s unlike you to stop until everything is business as usual, Reimu."),
		T_YUKARI("Depressed?"),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_REIMU("Yeah. That place sucked."),
		T_YUKARI("Perhaps I should’ve gone with you."),
		T_REIMU("Huh? Why? Do you wanna gap the whole thing out of Gensōkyō?"),
		T_YUKARI("That might be a bit much, even for me, my dear."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_REIMU("Whatever that ‘madness’ thing was, it was getting to me, too. It made me feel frustrated and… lonely?"),
		T_REIMU("But why would I feel lonely? It doesn’t make any sense."),
		T_YUKARI("So, what will you do?"),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_REIMU("Find a way to get rid of it, eventually. I’m sure there’s an answer somewhere out there…"),
		T_REIMU("Maybe Alice or Yūka know something."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_NARRATOR("Yukari smiled.\n"),
		T_YUKARI("Keep trying until you figure it out, yes?"),
		{ 0 },
	}},
#endif
	{ "", {
		T_CENTERED("- Bad End -", "Try to reach the end without using a Continue!"),
		{ 0 },
	}},

	{ NULL },
};

typedef struct CutsceneTextState {
	float alpha;
} CutsceneTextState;

typedef struct CutsceneBGState {
	Texture *scene;
	Texture *next_scene;
	float alpha;
	bool fade_out;
} CutsceneBGState;

typedef struct CutsceneTextVisual {
	LIST_INTERFACE(struct CutsceneTextVisual);
	const CutscenePhaseTextEntry *entry;
	float alpha;
	float target_alpha;
} CutsceneTextVisual;

typedef struct CutsceneState {
	const CutscenePhase *phase;
	const CutscenePhaseTextEntry *text_entry;
	CallChain cc;

	CutsceneBGState bg_state;
	LIST_ANCHOR(CutsceneTextVisual) text_visuals;

	int skip_timer;
	int advance_timer;
} CutsceneState;

static void clear_text(CutsceneState *st) {
	for(CutsceneTextVisual *tv = st->text_visuals.first; tv; tv = tv->next) {
		tv->target_alpha = 0;
	}
}

static void switch_bg(CutsceneState *st, const char *texture) {
	Texture *scene = *texture ? res_texture(texture) : NULL;

	if(st->bg_state.scene == NULL) {
		st->bg_state.scene = scene;
		st->bg_state.fade_out = false;
	} else {
		st->bg_state.next_scene = scene;

		if(st->bg_state.scene == st->bg_state.next_scene) {
			st->bg_state.next_scene = NULL;
			st->bg_state.fade_out = false;
		} else {
			st->bg_state.fade_out = true;
		}
	}
}

static void reset_timers(CutsceneState *st) {
	st->skip_timer = SKIP_DELAY;

	if(st->text_entry) {
		if(st->text_entry[1].text) {
			st->advance_timer = AUTO_ADVANCE_TIME_MID_SCENE;
		} else {
			st->advance_timer = AUTO_ADVANCE_TIME_CROSS_SCENE;
		}
	} else {
		st->advance_timer = AUTO_ADVANCE_TIME_BEFORE_TEXT;
	}

	log_debug("st->advance_timer = %i", st->advance_timer);
}

static void cutscene_advance(CutsceneState *st) {
	if(st->skip_timer > 0) {
		return;
	}

	if(st->phase) {
		if(st->text_entry == NULL) {
			st->text_entry = &st->phase->text_entries[0];
		} else if((++st->text_entry)->text == NULL) {
			st->text_entry = NULL;
			clear_text(st);

			if((++st->phase)->background == NULL) {
				st->phase = NULL;
			} else {
				switch_bg(st, st->phase->background);
			}
		}

		if(st->text_entry && st->text_entry->text) {
			CutsceneTextVisual *tv = calloc(1, sizeof(*tv));
			tv->target_alpha = 1;
			tv->entry = st->text_entry;
			alist_append(&st->text_visuals, tv);
		}
	}

	reset_timers(st);
}

static bool cutscene_event(SDL_Event *evt, void *ctx) {
	CutsceneState *st = ctx;

	if(evt->type == MAKE_TAISEI_EVENT(TE_MENU_ACCEPT)) {
		cutscene_advance(st);
	}

	return false;
}

static LogicFrameAction cutscene_logic_frame(void *ctx) {
	CutsceneState *st = ctx;

	events_poll((EventHandler[]) {
		{ .proc = cutscene_event, .arg = st, .priority = EPRIO_NORMAL },
		{ NULL },
	}, EFLAG_MENU);

	if(st->skip_timer > 0) {
		st->skip_timer--;
	}

	if(st->advance_timer > 0) {
		st->advance_timer--;
	}

	if(st->advance_timer == 0 || gamekeypressed(KEY_SKIP)) {
		cutscene_advance(st);
	}

	if(st->bg_state.fade_out) {
		if(fapproach_p(&st->bg_state.alpha, 0, 1/40.0f) == 0) {
			st->bg_state.scene = st->bg_state.next_scene;
			st->bg_state.next_scene = NULL;
			st->bg_state.fade_out = false;
		}
	} else if(st->bg_state.scene != NULL) {
		fapproach_p(&st->bg_state.alpha, 1, 1/60.0f);
	}

	for(CutsceneTextVisual *tv = st->text_visuals.first, *next; tv; tv = next) {
		next = tv->next;

		float rate = 0.1;

		if(tv->target_alpha > 0 && st->text_entry != tv->entry) {
			// if we skipped past this one and it's still fading in, speed it up
			rate *= 5;
		}

		if(fapproach_asymptotic_p(&tv->alpha, tv->target_alpha, rate, 1e-4) == 0) {
			free(alist_unlink(&st->text_visuals, tv));
		}
	}

	return LFRAME_WAIT;
}

#include "util/glm.h"

static void draw_background(CutsceneState *st) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_scale(SCREEN_W, SCREEN_H, 1);
	r_mat_mv_translate(0.5, 0.5, 0);

	float th0, th1, x;
	x = 1.0f - st->bg_state.alpha;

	th0 = x;
	th1 = x * (x + 1.5f * (1.0f - x));

	if(st->bg_state.fade_out) {
		th0 = 1 - th0;
		th1 = 1 - th1;
	}

	r_shader("cutscene");
	r_uniform_vec2("thresholds", th0, th1);

	if(st->bg_state.scene) {
		r_uniform_sampler("tex", st->bg_state.scene);
	}

	r_uniform_sampler("noise_tex", "cell_noise");
	r_uniform_sampler("paper_tex", "cutscenes/paper");
	r_uniform_float("distort_strength", 0.01);

	r_draw_quad();

	r_mat_mv_pop();
	r_state_pop();
}

static void draw_center_text(
	const CutsceneTextVisual *tv,
	const CutscenePhaseTextEntry *e,
	TextParams p
) {
	p.font_ptr = res_font("big");
	p.align = ALIGN_CENTER;
	p.pos.x = SCREEN_W/2;
	p.pos.y = SCREEN_H/2 - font_get_lineskip(p.font_ptr);
	text_draw(e->header, &p);
	p.pos.y += font_get_lineskip(p.font_ptr) * 1.2;
	p.font_ptr = res_font("standard");
	text_draw(e->text, &p);
}

static void draw_text(CutsceneState *st) {
	Font *font = res_font("standard");
	const float lines = 7;
	const float offset_x = 16;
	const float offset_y = 24;

	float width = SCREEN_W - 2 * offset_x;
	float speaker_width = width * 0.1;
	float height = lines * font_get_lineskip(font);
	float opacity = 1;

	float x = offset_x, y = offset_y;

	FloatRect textbox = {
		.extent = { width + offset_x * 2, height + offset_y * 2 },
		.offset = { x + width/2, y + height/2 - 2*offset_y/3 },
	};

	r_state_push();

	TextParams p = {
		.shader_ptr = res_shader("text_cutscene"),
		.aux_textures = { res_texture("cell_noise") },
		.shader_params = &(ShaderCustomParams) {{ (1.0 - (0.2 + 0.8 * (1 - opacity))), 0 }},
// 		.color = RGBA(0.8, 0.9, 0.5, 1),
// 		.color = HSLA(0.15, 0.7, 0.85, 1),
		.pos = { x, y },
		.align = ALIGN_LEFT,
		.font_ptr = font,
// 		.overlay_projection = &textbox,
	};

	for(CutsceneTextVisual *tv = st->text_visuals.first; tv; tv = tv->next) {
		const CutscenePhaseTextEntry *e = tv->entry;

		p.color = color_mul_scalar(COLOR_COPY(&e->color), tv->alpha * tv->alpha);

		if(e->type == CTT_CENTERED) {
			draw_center_text(tv, e, p);
			continue;
		}

		float w = width;
		p.pos.x = x * tv->alpha;
		p.pos.y = y * tv->alpha;

		if(e->header != NULL) {
			float ofs = 8;
			p.pos.x += speaker_width - ofs;
			w -= speaker_width;
			p.align = ALIGN_RIGHT;
			text_draw(e->header, &p);
			p.align = ALIGN_LEFT;
			p.pos.x += ofs;
		}

		char buf[strlen(e->text) * 2 + 1];
		text_wrap(font, e->text, w, buf, sizeof(buf));
		text_draw(buf, &p);

		y += text_height(font, buf, 0) * tv->alpha;
	}

	r_state_pop();
}

static RenderFrameAction cutscene_render_frame(void *ctx) {
	CutsceneState *st = ctx;
	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 1), 1);
	set_ortho(SCREEN_W, SCREEN_H);
	draw_background(st);
	draw_text(st);
	return RFRAME_SWAP;
}

static void cutscene_end_loop(void *ctx) {
	CutsceneState *st = ctx;

	for(CutsceneTextVisual *tv = st->text_visuals.first, *next; tv; tv = next) {
		next = tv->next;
		free(tv);
	}

	CallChain cc = st->cc;
	free(st);
	run_call_chain(&cc, NULL);
}

static void cutscene_preload(CutscenePhase phases[]) {
	for(CutscenePhase *p = phases; p->background; ++p) {
		if(*p->background) {
			preload_resource(RES_TEXTURE, p->background, RESF_DEFAULT);
		}
	}
}

static CutsceneState *cutscene_state_new(CutscenePhase phases[]) {
	cutscene_preload(phases);
	CutsceneState *st = calloc(1, sizeof(*st));
	st->phase = &phases[0];
	switch_bg(st, st->phase->background);
	reset_timers(st);
	return st;
}

void cutscene_enter(CallChain next) {
	CutsceneState *st = cutscene_state_new(test_cutscene);
	st->cc = next;
	eventloop_enter(st, cutscene_logic_frame, cutscene_render_frame, cutscene_end_loop, FPS);
}
