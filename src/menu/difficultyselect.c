/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "difficulty.h"
#include "difficultyselect.h"
#include "options.h"
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
		menu->entries[i].drawdata += 0.2 * (30*(i == menu->cursor) - menu->entries[i].drawdata);
	}

	color_approach(&diff_color, difficulty_color(menu->cursor + D_Easy), 0.1);
}

void create_difficulty_menu(MenuData *m) {
	create_menu(m);
	m->draw = draw_difficulty_menu;
	m->logic = update_difficulty_menu;
	m->transition = TransMenuDark;
	m->flags = MF_Transient | MF_Abortable;

	add_menu_entry(m, "“All those bullets confuse me!”\nYou will be stuck here forever", set_difficulty, (void *)D_Easy);
	add_menu_entry(m, "“So it's not just about shooting stuff?”\nSomewhat challenging", set_difficulty, (void *)D_Normal);
	add_menu_entry(m, "“Pain is my ally!”\nActually challenging", set_difficulty, (void *)D_Hard);
	add_menu_entry(m, "“I have no fear.”\nWe never playtested this one", set_difficulty, (void *)D_Lunatic);

	for(int i = 0; i < m->ecount; ++i) {
		Difficulty d = (Difficulty)(uintptr_t)m->entries[i].arg;

		if(d == progress.game_settings.difficulty) {
			m->cursor = i;
			break;
		}
	}
}

void draw_difficulty_menu(MenuData *menu) {
	draw_options_menu_bg(menu);
	draw_menu_title(menu, "Select Difficulty");

	Color c = diff_color;
	r_color(color_mul(&c, RGBA(0.07, 0.07, 0.07, 0.7)));

	r_mat_push();
	r_mat_translate(SCREEN_W/2+30 - 25*menu->drawdata[0], SCREEN_H/3 + 90*(0.7*menu->drawdata[0]),0);
	r_mat_rotate_deg(4*menu->drawdata[0]-4,0,0,1);
	r_mat_push();
	r_mat_scale(SCREEN_W*1.5,120,1);
	r_shader_standard_notex();
	r_draw_quad();
	r_mat_pop();
	r_color3(1,1,1);

	r_shader("text_default");
	text_draw(menu->entries[menu->cursor].name, &(TextParams) {
		.pos = { 40+35*menu->drawdata[0], -12 },
	});

	r_shader("sprite_default");
	r_mat_pop();

	for(int i = 0; i < menu->ecount; ++i) {
		r_mat_push();
		r_mat_translate(SCREEN_W/2 + SCREEN_W*sign((i&1)-0.5)*(i!=menu->cursor)*menu_fade(menu) - (int)menu->entries[i].drawdata+25*i-50, SCREEN_H/3 + 90*(i-0.3*menu->drawdata[0]),0);

		r_color3(1,1,1);
		draw_sprite_batched(0, 0, difficulty_sprite_name(D_Easy+i));
		r_mat_pop();
	}

	r_shader_standard();
}
