/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "difficulty.h"
#include "difficultyselect.h"
#include "options.h"
#include "common.h"
#include "global.h"

// FIXME: put this into the menu struct somehow (drawdata is a bad system)
static Color diff_color;

void set_difficulty(MenuData *m, void *d) {
	progress.game_settings.difficulty = (Difficulty)(uintptr_t)d;
}

void update_difficulty_menu(MenuData *menu) {
	menu->drawdata[0] += (menu->cursor-menu->drawdata[0])*0.1;

	for(int i = 0; i < menu->ecount; ++i) {
		menu->entries[i].drawdata += 0.2 * (30*(i == menu->cursor) - menu->entries[i].drawdata);
	}

	diff_color = approach_color(diff_color, difficulty_color(menu->cursor + D_Easy), 0.1);
}

void create_difficulty_menu(MenuData *m) {
	create_menu(m);
	m->draw = draw_difficulty_menu;
	m->logic = update_difficulty_menu;
	m->transition = TransMenuDark;
	m->flags = MF_Transient | MF_Abortable;

	add_menu_entry(m, "“All those bullets confuse me!”\nYou will be stuck here forever", set_difficulty, (void *)D_Easy);
	add_menu_entry(m, "“So this isn’t about shooting things?”\nSomewhat challenging", set_difficulty, (void *)D_Normal);
	add_menu_entry(m, "“Why can’t I beat this?”\nActually challenging", set_difficulty, (void *)D_Hard);
	add_menu_entry(m, "“What is pain?”\nAsian mode", set_difficulty, (void *)D_Lunatic);

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

	render_color(multiply_colors(diff_color, rgba(0.1, 0.1, 0.1, 0.7)));

	render_push();
	render_translate(SCREEN_W/2+30 - 25*menu->drawdata[0], SCREEN_H/3 + 90*(0.7*menu->drawdata[0]),0);
	render_rotate_deg(4*menu->drawdata[0]-4,0,0,1);
	render_push();
	render_scale(SCREEN_W*1.5,120,1);
	render_shader_standard_notex();
	render_draw_quad();
	render_shader_standard();
	render_pop();
	render_color3(1,1,1);
	draw_text(AL_Left, 40+35*menu->drawdata[0], -12, menu->entries[menu->cursor].name, _fonts.standard);

	render_pop();

	for(int i = 0; i < menu->ecount; ++i) {
		render_push();
		render_translate(SCREEN_W/2 + SCREEN_W*sign((i&1)-0.5)*(i!=menu->cursor)*menu_fade(menu) - (int)menu->entries[i].drawdata+25*i-50, SCREEN_H/3 + 90*(i-0.3*menu->drawdata[0]),0);

		//render_color4(0,0,0,1);

		render_color3(1,1,1);
		draw_sprite(0, 0, difficulty_sprite_name(D_Easy+i));
		render_pop();
	}
}
