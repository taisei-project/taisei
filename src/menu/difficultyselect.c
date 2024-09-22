/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "difficultyselect.h"

#include "common.h"
#include "difficulty.h"
#include "mainmenu.h"
#include "progress.h"
#include "resource/font.h"
#include "video.h"

// FIXME: put this into the menu struct somehow (drawdata is a bad system)
static Color diff_color;

static void set_difficulty(MenuData *m, void *d) {
	progress.game_settings.difficulty = (Difficulty)(uintptr_t)d;
}

static void update_difficulty_menu(MenuData *menu) {
	menu->drawdata[0] += (menu->cursor-menu->drawdata[0])*0.1;

	dynarray_foreach(&menu->entries, int i, MenuEntry *e, {
		e->drawdata += 0.2 * ((i == menu->cursor) - e->drawdata);
	});

	color_approach(&diff_color, difficulty_color(menu->cursor + D_Easy), 0.1);
}

MenuData* create_difficulty_menu(void) {
	MenuData *m = alloc_menu();

	m->draw = draw_difficulty_menu;
	m->logic = update_difficulty_menu;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;

	add_menu_entry(m, "For those just beginning\ntheir academic careers", set_difficulty, (void *)D_Easy);
	add_menu_entry(m, "All those years of study\nhave finally paid off", set_difficulty, (void *)D_Normal);
	add_menu_entry(m, "You must really\nlove books", set_difficulty, (void *)D_Hard);
	add_menu_entry(m, "You either die a student,\nor live long enough to\nbecome a professor", set_difficulty, (void *)D_Lunatic);

	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		Difficulty d = (Difficulty)(uintptr_t)e->arg;

		if(d == progress.game_settings.difficulty) {
			m->cursor = i;
			break;
		}
	});

	return m;
}

void draw_difficulty_menu(MenuData *menu) {
	r_state_push();

	draw_main_menu_bg(menu, 0, 0, 0.05, "menu/mainmenubg", "stage1/cirnobg");
	draw_menu_title(menu, "Select Difficulty");

	Color c = diff_color;
	r_color(color_mul(&c, RGBA(0.07, 0.07, 0.07, 0.7)));

	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W/2, SCREEN_H/2,0);
	r_mat_mv_rotate((4 * menu->drawdata[0] - 4) * DEG2RAD, 0, 0, 1);
	r_mat_mv_push();
	r_mat_mv_scale(SCREEN_W*1.5,120,1);
	r_shader_standard_notex();
	r_draw_quad();
	r_mat_mv_pop();
	r_color3(1,1,1);

	r_shader("text_default");

	float amp = menu->cursor/2.;
	float shake = 0.3*amp*menu->frames;
	text_draw(dynarray_get(&menu->entries, menu->cursor).name, &(TextParams) {
		.pos = { 120+15*menu->drawdata[0]+amp*sin(shake), -12+amp*cos(1.57*shake) },
	});

	r_shader("sprite_default");

	dynarray_foreach(&menu->entries, int i, MenuEntry *e, {
		float scale = 0.5 + e->drawdata;
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = res_sprite(difficulty_sprite_name(D_Easy + i)),
			.pos = { 0, 240 * tanh(0.7 * (i - menu->drawdata[0])) },
			.color = RGBA(scale, scale, scale, scale),
			.scale.both = scale,
		});
	});

	r_mat_mv_pop();
	r_state_pop();
}
