/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "difficulty.h"
#include "difficultyselect.h"
#include "mainmenu.h"
#include "common.h"
#include "global.h"
#include "video.h"

// FIXME: put this into the menu struct somehow (drawdata is a bad system)
static Color diff_color;

static void set_difficulty(MenuData *m, void *d) {
	progress.game_settings.difficulty = (Difficulty)(uintptr_t)d;
}

static void update_difficulty_menu(MenuData *menu) {
	menu->drawdata[0] += (menu->cursor-menu->drawdata[0])*0.1;

	for(int i = 0; i < menu->ecount; ++i) {
		menu->entries[i].drawdata += 0.2 * ((i == menu->cursor) - menu->entries[i].drawdata);
	}

	color_approach(&diff_color, difficulty_color(menu->cursor + D_Easy), 0.1);
}

MenuData* create_difficulty_menu(void) {
	MenuData *m = alloc_menu();

	m->draw = draw_difficulty_menu;
	m->logic = update_difficulty_menu;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;

	add_menu_entry(m, "“All those bullets confuse me!”\nYou will be stuck here forever", set_difficulty, (void *)D_Easy);
	add_menu_entry(m, "“Do you even graze?”\nSomewhat challenging", set_difficulty, (void *)D_Normal);
	add_menu_entry(m, "“Pain is my ally!”\nActually challenging", set_difficulty, (void *)D_Hard);
	add_menu_entry(m, "“I have no fear.”\nWe never playtested this one", set_difficulty, (void *)D_Lunatic);

	for(int i = 0; i < m->ecount; ++i) {
		Difficulty d = (Difficulty)(uintptr_t)m->entries[i].arg;

		if(d == progress.game_settings.difficulty) {
			m->cursor = i;
			break;
		}
	}

	return m;
}

void draw_difficulty_menu(MenuData *menu) {
	draw_main_menu_bg(menu, 0, 0, 0.05, "menu/mainmenubg", "stage1/cirnobg");
	draw_menu_title(menu, "Select Difficulty");

	Color c = diff_color;
	r_color(color_mul(&c, RGBA(0.07, 0.07, 0.07, 0.7)));

	r_mat_push();
	r_mat_translate(SCREEN_W/2, SCREEN_H/2,0);
	r_mat_rotate_deg(4*menu->drawdata[0]-4,0,0,1);
	r_mat_push();
	r_mat_scale(SCREEN_W*1.5,120,1);
	r_shader_standard_notex();
	r_draw_quad();
	r_mat_pop();
	r_color3(1,1,1);

	r_shader("text_default");

	float amp = menu->cursor/2.;
	float shake = 0.3*amp*menu->frames;
	text_draw(menu->entries[menu->cursor].name, &(TextParams) {
		.pos = { 120+15*menu->drawdata[0]+amp*sin(shake), -12+amp*cos(1.57*shake) },
	});

	r_shader("sprite_default");

	

	for(int i = 0; i < menu->ecount; ++i) {
		r_mat_push();
		r_mat_translate(0, 240*tanh(0.7*(i-menu->drawdata[0])),0);
		float scale = 0.5+menu->entries[i].drawdata;
		r_color4(scale, scale, scale, scale);
		r_mat_scale(scale, scale, 1);

		draw_sprite_batched(0, 0, difficulty_sprite_name(D_Easy+i));
		r_mat_pop();
		r_color3(1,1,1);
	}

	r_mat_pop();
	r_shader_standard();
}
