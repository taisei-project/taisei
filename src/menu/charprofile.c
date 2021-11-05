/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "events.h"
#include "resource/resource.h"
#include "taisei.h"

#include "charprofile.h"
#include "mainmenu.h"
#include "portrait.h"
#include "common.h"
#include "progress.h"
#include "options.h"
#include "global.h"
#include "util/glm.h"
#include "video.h"

static void update_char_draw_order(MenuData *m) {
	CharProfileContext *ctx = m->context;

	for(int i = 0; i < NUM_PROFILES; ++i) {
		if(ctx->char_draw_order[i] == m->cursor) {
			while(i) {
				ctx->char_draw_order[i] = ctx->char_draw_order[i - 1];
				ctx->char_draw_order[i - 1] = m->cursor;
				--i;
			}

			break;
		}
	}
}

static int check_unlocked_profile(int i) {
	int selected = LOCKED_PROFILE;
	if(!profiles[i].unlock) {
		// for protagonists
		selected = i;
	} else if(strcmp(profiles[i].unlock, "lockedboss")) {
		if(progress_is_bgm_unlocked(profiles[i].unlock)) selected = i;
	}
	return selected;
}

static void charprofile_logic(MenuData *m) {
	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		e->drawdata += 0.05 * ((m->cursor != i) - e->drawdata);
	});
	MenuEntry *cursor_entry = dynarray_get_ptr(&m->entries, m->cursor);
	Font *font = res_font("small");
	char buf[512] = { 0 };
	int j = check_unlocked_profile(m->cursor);
	text_wrap(font, profiles[j].description, DESCRIPTION_WIDTH, buf, sizeof(buf));
	double height = text_height(font, buf, 0) + font_get_lineskip(font) * 2;

	fapproach_asymptotic_p(&m->drawdata[0], 1, 0.1, 1e-5);
	fapproach_asymptotic_p(&m->drawdata[1], 1 - cursor_entry->drawdata, 0.1, 1e-5);
	fapproach_asymptotic_p(&m->drawdata[2], height, 0.1, 1e-5);
}

static void charprofile_draw(MenuData *m) {
	assert(m->cursor < NUM_PROFILES);
	r_state_push();

	CharProfileContext *ctx = m->context;

	draw_main_menu_bg(m, SCREEN_W/4+100, 0, 0.1 * (0.5 + 0.5 * m->drawdata[1]), "menu/mainmenubg", profiles[m->cursor].background);
	draw_menu_title(m, "Character Profiles");
	draw_menu_list(m, 100, 100, NULL, SCREEN_H);

	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W/4, SCREEN_H/3, 0);

	float f = m->drawdata[0];
	float descbg_ofs = 100 + 30 * f - 20 * f - font_get_lineskip(res_font("small")) * 0.7;

	r_color4(0, 0, 0, 0.5);
	r_shader_standard_notex();
	r_mat_mv_translate(-150, descbg_ofs + m->drawdata[2] * 0.5, 0);
	r_mat_mv_scale(650, m->drawdata[2], 1);
	r_draw_quad();
	r_shader_standard();
	r_mat_mv_pop();

	CharProfiles i = ctx->char_draw_order[m->cursor];

	MenuEntry *e = dynarray_get_ptr(&m->entries, m->cursor);

	float o = 1 - e->drawdata*2;
	float pbrightness = 0.6 + 0.4 * o;

	float pofs = fmax(0.0f, e->drawdata * 1.5f - 0.5f);
	pofs = glm_ease_back_in(pofs);

	if(i != m->selected) {
		pofs = lerp(pofs, 1, menu_fade(m));
	}

	int selected = check_unlocked_profile(m->cursor);

	if(selected != LOCKED_PROFILE) {
		Sprite *spr = profiles[selected].sprite;
		SpriteParams portrait_params = {
			.pos = { SCREEN_W/2 + 240 + 320 * pofs, SCREEN_H - spr->h * 0.5 },
			.sprite_ptr = spr,
			.shader = "sprite_default",
			.color = RGBA(pbrightness, pbrightness, pbrightness, 1),
		};

		r_draw_sprite(&portrait_params);
		portrait_params.sprite_ptr = portrait_get_face_sprite(profiles[selected].name, profiles[selected].faces[ctx->face]);
		r_draw_sprite(&portrait_params);
	}
	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W/4, SCREEN_H/3, 0);
	r_mat_mv_push();
	r_mat_mv_push();

	if(e->drawdata != 0) {
		r_mat_mv_translate(0, -300 * e->drawdata, 0);
		r_mat_mv_rotate(M_PI * e->drawdata, 1, 0, 0);
	}

	text_draw(profiles[selected].fullname, &(TextParams) {
		.align = ALIGN_CENTER,
		.font = "big",
		.shader = "text_default",
		.color = RGBA(o, o, o, o),
	});
	r_mat_mv_pop();

	if(e->drawdata) {
		o = 1 - e->drawdata * 3;
	} else {
		o = 1;
	}

	text_draw(profiles[selected].title, &(TextParams) {
		.align = ALIGN_CENTER,
		.pos = { 20*(1-o), 30 },
		.shader = "text_default",
		.color = RGBA(o, o, o, o),
	});
	r_mat_mv_pop();

	r_color4(1, 1, 1, 0.5);
	text_draw_wrapped(profiles[selected].description, DESCRIPTION_WIDTH, &(TextParams) {
		.align = ALIGN_LEFT,
		.pos = { -190, 120 },
		.font = "small",
		.shader = "text_default",
		.color = RGBA(o, o, o, o),
	});
	r_mat_mv_pop();

	o = 0.3*sin(m->frames/20.0)+0.5;
	r_shader("sprite_default");

	r_draw_sprite(&(SpriteParams) {
		.sprite = "menu/arrow",
		.pos = { 30, SCREEN_H/3+10 },
		.color = RGBA(o, o, o, o),
		.scale = { 0.5, 0.7 },
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite = "menu/arrow",
		.pos = { 30 + 340, SCREEN_H/3+10 },
		.color = RGBA(o, o, o, o),
		.scale = { 0.5, 0.7 },
		.flip.x = true,
	});

	r_state_pop();
}

static void action_show_character(MenuData *m, void *arg) {
	return;
}

static void add_character(MenuData *m, int i) {
	if(strcmp(profiles[i].name, "locked")) {
		log_debug("adding character: %s", profiles[i].name);
		portrait_preload_base_sprite(profiles[i].name, NULL, RESF_PERMANENT);
		profiles[i].sprite = portrait_get_base_sprite(profiles[i].name, NULL);
		MenuEntry *e = add_menu_entry(m, NULL, action_show_character, NULL);
		e->transition = NULL;
	}
}

static void charprofile_free(MenuData *m) {
	dynarray_foreach_elem(&m->entries, MenuEntry *e, {
		free(e->arg);
	});
}

static bool charprofile_input_handler(SDL_Event *event, void *arg) {
	MenuData *m = arg;
	CharProfileContext *ctx = m->context;
	TaiseiEvent type = TAISEI_EVENT(event->type);

	int prev_cursor = m->cursor;

	if(type == TE_MENU_CURSOR_LEFT) {
		m->cursor--;
		ctx->face = 0;
	} else if(type == TE_MENU_CURSOR_RIGHT) {
		m->cursor++;
		ctx->face = 0;
	} else if(type == TE_MENU_ACCEPT) {
		// show different expressions for selected character
		ctx->face++;
		if(!profiles[m->cursor].faces[ctx->face]) ctx->face = 0;
	} else if(type == TE_MENU_ABORT) {
		play_sfx_ui("hit");
		close_menu(m);
	}

	m->cursor = (m->cursor % m->entries.num_elements) + m->entries.num_elements * (m->cursor < 0);

	if(m->cursor != prev_cursor) {
		if(ctx->prev_selected_char != m->cursor || dynarray_get(&m->entries, m->cursor).drawdata > 0.95) {
			ctx->prev_selected_char = prev_cursor;
			update_char_draw_order(m);
		}
	}

	return false;
}

static void charprofile_input(MenuData *m) {
	events_poll((EventHandler[]){
		{ .proc = charprofile_input_handler, .arg = m },
		{ NULL }
	}, EFLAG_MENU);
}

MenuData *create_charprofile_menu(void) {
	MenuData *m = alloc_menu();

	m->input = charprofile_input;
	m->draw = charprofile_draw;
	m->logic = charprofile_logic;
	m->end = charprofile_free;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;

	CharProfileContext *ctx = calloc(1, sizeof(*ctx));
	ctx->prev_selected_char = -1;
	m->context = ctx;

	for(int i = 0; i < NUM_PROFILES; i++) {
		add_character(m, i);
	}

	for(CharProfiles c = 0; c < NUM_PROFILES; ++c) {
		ctx->char_draw_order[c] = c;
	}

	m->drawdata[1] = 1;

	return m;
}
