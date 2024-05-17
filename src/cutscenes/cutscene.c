/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "cutscene.h"
#include "scene_impl.h"
#include "scenes.h"

#include "audio/audio.h"
#include "color.h"
#include "eventloop/eventloop.h"
#include "events.h"
#include "global.h"
#include "progress.h"
#include "renderer/api.h"
#include "replay/demoplayer.h"
#include "transition.h"
#include "util/fbmgr.h"
#include "util/glm.h"
#include "util/graphics.h"
#include "video.h"
#include "watchdog.h"

#define SKIP_DELAY 3
#define AUTO_ADVANCE_TIME_BEFORE_TEXT FPS * 2
#define AUTO_ADVANCE_TIME_MID_SCENE   FPS * 20
#define AUTO_ADVANCE_TIME_CROSS_SCENE FPS * 180

// TODO maybe make transitions configurable?
enum {
	CUTSCENE_FADE_OUT = 200,
	CUTSCENE_INTERRUPT_FADE_OUT = 15,
};

typedef struct CutsceneBGState {
	Texture *scene;
	Texture *next_scene;
	float alpha;
	float transition_rate;
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
	ResourceGroup rg;

	CutsceneBGState bg_state;
	LIST_ANCHOR(CutsceneTextVisual) text_visuals;

	ManagedFramebufferGroup *mfb_group;
	Framebuffer *text_fb;
	FBPair erase_mask_fbpair;

	int skip_timer;
	int advance_timer;
	int fadeout_timer;

	bool interruptible;
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

static bool skip_text_animation(CutsceneState *st) {
	bool animation_skipped = false;

	for(CutsceneTextVisual *tv = st->text_visuals.first; tv; tv = tv->next) {
		if(tv->target_alpha > 0 && tv->alpha < tv->target_alpha * 0.6) {
			tv->alpha = tv->target_alpha;
			animation_skipped = true;
		}
	}

	return animation_skipped;
}

static void begin_fadeout(CutsceneState *st, int fade_frames) {
	if(st->fadeout_timer) {
		log_debug("Already fading out, timer = %i", st->fadeout_timer);
		return;
	}

	audio_bgm_stop((FPS * fade_frames) / 4000.0);
	set_transition(TransFadeBlack, fade_frames, fade_frames, NO_CALLCHAIN);
	st->fadeout_timer = fade_frames;
	st->bg_state.next_scene = NULL;
	st->bg_state.fade_out = true;
	st->bg_state.transition_rate = 1.0f / fade_frames;
}

static void cutscene_advance(CutsceneState *st) {
	if(st->skip_timer > 0) {
		log_debug("Skip too soon");
		return;
	}

	if(st->phase) {
		if(st->text_entry == NULL) {
			st->text_entry = &st->phase->text_entries[0];
		} else if((++st->text_entry)->text == NULL) {
			if(skip_text_animation(st)) {
				--st->text_entry;
				return;
			}

			st->text_entry = NULL;
			clear_text(st);

			if((++st->phase)->background == NULL) {
				st->phase = NULL;
				begin_fadeout(st, CUTSCENE_FADE_OUT);
			} else {
				switch_bg(st, st->phase->background);
			}
		}

		if(st->text_entry && st->text_entry->text) {
			alist_append(&st->text_visuals, ALLOC(CutsceneTextVisual, {
				.alpha = 0.1,
				.target_alpha = 1,
				.entry = st->text_entry,
			}));
		}
	}

	reset_timers(st);
}

static void cutscene_interrupt(CutsceneState *st) {
	begin_fadeout(st, CUTSCENE_INTERRUPT_FADE_OUT);
}

static bool cutscene_event(SDL_Event *evt, void *ctx) {
	CutsceneState *st = ctx;

	if(evt->type == MAKE_TAISEI_EVENT(TE_MENU_ACCEPT)) {
		cutscene_advance(st);
	}

	if(evt->type == MAKE_TAISEI_EVENT(TE_MENU_ABORT) && st->interruptible) {
		cutscene_interrupt(st);
	}

	return false;
}

static LogicFrameAction cutscene_logic_frame(void *ctx) {
	CutsceneState *st = ctx;

	if(watchdog_signaled()) {
		cutscene_interrupt(st);
	}

	update_transition();
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
		if(fapproach_p(&st->bg_state.alpha, 0, st->bg_state.transition_rate) == 0) {
			st->bg_state.scene = st->bg_state.next_scene;
			st->bg_state.next_scene = NULL;
			st->bg_state.fade_out = false;
		}
	} else if(st->bg_state.scene != NULL) {
		fapproach_p(&st->bg_state.alpha, 1, st->bg_state.transition_rate);
	}

	for(CutsceneTextVisual *tv = st->text_visuals.first, *next; tv; tv = next) {
		next = tv->next;

		float rate = 1/120.0f;

		if(tv->target_alpha > 0 && st->text_entry != tv->entry) {
			// if we skipped past this one and it's still fading in, speed it up
			rate *= 5;
		}

		if(fapproach_p(&tv->alpha, tv->target_alpha, rate) == 0) {
			mem_free(alist_unlink(&st->text_visuals, tv));
		}
	}

	if(st->fadeout_timer > 0) {
		if(!(--st->fadeout_timer)) {
			return LFRAME_STOP;
		}
	}

	return LFRAME_WAIT;
}

static void draw_background(CutsceneState *st, Texture *erase_mask) {
	r_state_push();
	r_blend(BLEND_NONE);
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
	r_uniform_sampler("erase_mask_tex", erase_mask);
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

	float x = offset_x, y = offset_y;

	FloatRect textbox = {
		.extent = { width + offset_x * 2, height + offset_y * 2 },
		.offset = { x + width/2, y + height/2 - 2*offset_y/3 },
	};

	r_state_push();

	ShaderCustomParams cparams = { 0 };

	TextParams p = {
		.shader_ptr = res_shader("text_cutscene"),
		.aux_textures = { res_texture("cell_noise") },
		.shader_params = &cparams,
		.pos = { x, y },
		.align = ALIGN_LEFT,
		.font_ptr = font,
		.overlay_projection = &textbox,
	};

	for(CutsceneTextVisual *tv = st->text_visuals.first; tv; tv = tv->next) {
		const CutscenePhaseTextEntry *e = tv->entry;

		p.color = &e->color;
		cparams.vector[0] = tv->alpha;

		if(e->type == CTT_CENTERED) {
			draw_center_text(tv, e, p);
			continue;
		}

		float w = width;
		p.pos.x = x;
		p.pos.y = y;

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

		y += text_height(font, buf, 0) * glm_ease_quad_in(min(3.0f * tv->alpha, 1.0f));
	}

	r_state_pop();
}

static RenderFrameAction cutscene_render_frame(void *ctx) {
	CutsceneState *st = ctx;
	r_clear(BUFFER_ALL, RGBA(0, 0, 0, 1), 1);
	set_ortho(SCREEN_W, SCREEN_H);

	r_state_push();

	r_framebuffer(st->text_fb);
	r_clear(BUFFER_ALL, RGBA(0, 0, 0, 0), 1);
	draw_text(st);

	r_shader_standard();
	r_blend(BLEND_NONE);

	r_framebuffer(st->erase_mask_fbpair.back);
	r_clear(BUFFER_ALL, RGBA(0, 0, 0, 0), 1);
	draw_framebuffer_tex(st->text_fb, SCREEN_W, SCREEN_H);
	fbpair_swap(&st->erase_mask_fbpair);

	FloatRect mask_vp;
	r_framebuffer_viewport_current(st->erase_mask_fbpair.back, &mask_vp);
	r_shader("blur9");
	r_uniform_vec2("blur_resolution", mask_vp.w, mask_vp.h);

	r_framebuffer(st->erase_mask_fbpair.back);
	r_clear(BUFFER_ALL, RGBA(0, 0, 0, 0), 1);
	r_uniform_vec2("blur_direction", 1, 0);
	draw_framebuffer_tex(st->erase_mask_fbpair.front, SCREEN_W, SCREEN_H);
	fbpair_swap(&st->erase_mask_fbpair);

	r_framebuffer(st->erase_mask_fbpair.back);
	r_clear(BUFFER_ALL, RGBA(0, 0, 0, 0), 1);
	r_uniform_vec2("blur_direction", 0, 1);
	draw_framebuffer_tex(st->erase_mask_fbpair.front, SCREEN_W, SCREEN_H);
	fbpair_swap(&st->erase_mask_fbpair);

	r_state_pop();

	draw_background(st, r_framebuffer_get_attachment(st->erase_mask_fbpair.front, FRAMEBUFFER_ATTACH_COLOR0));
	draw_framebuffer_tex(st->text_fb, SCREEN_W, SCREEN_H);

	draw_transition();

	return RFRAME_SWAP;
}

static void cutscene_end_loop(void *ctx) {
	CutsceneState *st = ctx;
	res_group_release(&st->rg);

	for(CutsceneTextVisual *tv = st->text_visuals.first, *next; tv; tv = next) {
		next = tv->next;
		mem_free(tv);
	}

	fbmgr_group_destroy(st->mfb_group);

	CallChain cc = st->cc;
	mem_free(st);

	demoplayer_resume();
	run_call_chain(&cc, NULL);
}

static void cutscene_preload(const CutscenePhase phases[], ResourceGroup *rg) {
	for(const CutscenePhase *p = phases; p->background; ++p) {
		if(*p->background) {
			res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT, p->background, NULL);
		}
	}

	res_group_preload(rg, RES_BGM, RESF_DEFAULT, "ending", NULL);
}

static CutsceneState *cutscene_state_new(const CutscenePhase phases[]) {
	auto st = ALLOC(CutsceneState, {
		.phase = &phases[0],
		.mfb_group = fbmgr_group_create(),
	});

	res_group_init(&st->rg);
	cutscene_preload(phases, &st->rg);

	switch_bg(st, st->phase->background);
	reset_timers(st);

	FBAttachmentConfig a = { 0 };
	a.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a.tex_params.type = TEX_TYPE_RGBA_8;
	a.tex_params.filter.min = TEX_FILTER_LINEAR;
	a.tex_params.filter.mag = TEX_FILTER_LINEAR;
	a.tex_params.wrap.s = TEX_WRAP_MIRROR;
	a.tex_params.wrap.s = TEX_WRAP_MIRROR;

	FramebufferConfig fbconf = { 0 };
	fbconf.attachments = &a;
	fbconf.num_attachments = 1;
	fbconf.resize_strategy.resize_func = fbmgr_resize_strategy_screensized;

	st->text_fb = fbmgr_group_framebuffer_create(st->mfb_group, "Cutscene text", &fbconf);

	fbconf.resize_strategy.resize_func = NULL;
	a.tex_params.width = SCREEN_W / 4;
	a.tex_params.height= SCREEN_H / 4;
	a.tex_params.type = TEX_TYPE_RGBA_8;
	fbmgr_group_fbpair_create(st->mfb_group, "Cutscene erase mask", &fbconf, &st->erase_mask_fbpair);

	return st;
}

void cutscene_enter(CallChain next, CutsceneID id) {
	assert((uint)id < NUM_CUTSCENE_IDS);

	bool interruptible = progress_is_cutscene_unlocked(id);
	progress_unlock_cutscene(id);
	const Cutscene *cs = g_cutscenes + id;

	if(cs->phases == NULL) {
		log_error("Cutscene %i not implemented!", id);
		run_call_chain(&next, NULL);
		return;
	}

	log_info("Playing cutscene ID: #%i", id);
	CutsceneState *st = cutscene_state_new(cs->phases);
	st->cc = next;
	st->bg_state.transition_rate = 1/80.0f;
	st->interruptible = interruptible;
	progress_unlock_bgm(cs->bgm);
	audio_bgm_play(res_bgm(cs->bgm), true, 0, 1);
	demoplayer_suspend();
	eventloop_enter(st, cutscene_logic_frame, cutscene_render_frame, cutscene_end_loop, FPS);
}

const char *cutscene_get_name(CutsceneID id) {
	assert((uint)id < NUM_CUTSCENE_IDS);
	return g_cutscenes[id].name;
}
