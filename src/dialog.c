/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "dialog.h"
#include "global.h"
#include <stdlib.h>
#include <string.h>

Dialog *create_dialog(const char *left, const char *right) {
	Dialog *d = malloc(sizeof(Dialog));
	memset(d, 0, sizeof(Dialog));

	if(left) {
		d->images[Left] = get_sprite(left);
	}

	if(right) {
		d->images[Right] = get_sprite(right);
	}

	d->page_time = global.frames;
	d->birthtime = global.frames;
	return d;
}

void dset_image(Dialog *d, Side side, const char *name) {
	d->images[side] = get_sprite(name);
}

DialogMessage* dadd_msg(Dialog *d, Side side, const char *msg) {
	d->messages = realloc(d->messages, (++d->count)*sizeof(DialogMessage));
	d->messages[d->count-1].side = side;
	d->messages[d->count-1].msg = malloc(strlen(msg) + 1);
	d->messages[d->count-1].timeout = 0;
	strlcpy(d->messages[d->count-1].msg, msg, strlen(msg) + 1);
	return &d->messages[d->count-1];
}

void delete_dialog(Dialog *d) {
	int i;
	for(i = 0; i < d->count; i++)
		free(d->messages[i].msg);

	free(d->messages);
	free(d);
}

void draw_dialog(Dialog *dialog) {
	r_mat_push();

	r_mat_translate(VIEWPORT_W/2.0, VIEWPORT_H*3.0/4.0, 0);

	int i;
	for(i = 0; i < 2; i++) {
		r_mat_push();
		if(i == Left) {
			glCullFace(GL_FRONT);
			r_mat_scale(-1,1,1);
		}

		if(global.frames - dialog->birthtime < 30)
			r_mat_translate(120 - (global.frames - dialog->birthtime)*4, 0, 0);

		int cur = dialog->messages[dialog->pos].side;
		int pre = 2;
		if(dialog->pos > 0)
			pre = dialog->messages[dialog->pos-1].side;

		short dir = (1 - 2*(i == dialog->messages[dialog->pos].side));
		if(global.frames - dialog->page_time < 10 && ((i != pre && i == cur) || (i == pre && i != cur))) {
			int time = (global.frames - dialog->page_time) * dir;
			r_mat_translate(time, time, 0);
			float clr = min(1.0 - 0.07*time,1);
			r_color3(clr, clr, clr);
		} else {
			r_mat_translate(dir*10, dir*10, 0);
			r_color3(1 - (dir>0)*0.7, 1 - (dir>0)*0.7, 1 - (dir>0)*0.7);
		}

		r_mat_translate(VIEWPORT_W*7.0/18.0, 0, 0);
		if(dialog->images[i])
			draw_sprite_p(0, 0, dialog->images[i]);
		
		r_mat_pop();

		r_color3(1,1,1);
	}

	glCullFace(GL_BACK);
	r_mat_pop();

	r_mat_push();
	if(global.frames - dialog->birthtime < 25)
		r_mat_translate(0, 100-(global.frames-dialog->birthtime)*4, 0);
	
	r_color4(0,0,0,0.8);

	r_mat_push();
	r_mat_translate(VIEWPORT_W/2, VIEWPORT_H-75, 0);
	r_mat_scale(VIEWPORT_W-40, 110, 1);

	r_shader_standard_notex();
	r_draw_quad();
	r_shader_standard();

	r_mat_pop();
	r_color4(1,1,1,1);

	if(dialog->messages[dialog->pos].side == Right)
		r_color3(0.6,0.6,1);

	draw_text_auto_wrapped(AL_Center, VIEWPORT_W/2, VIEWPORT_H-110, dialog->messages[dialog->pos].msg, VIEWPORT_W * 0.85, _fonts.standard);

	if(dialog->messages[dialog->pos].side == Right)
		r_color3(1,1,1);

	r_mat_pop();
}

bool page_dialog(Dialog **d) {
	int to = (*d)->messages[(*d)->pos].timeout;

	if(to && to > global.frames) {
		return false;
	}

	(*d)->pos++;
	(*d)->page_time = global.frames;

	if((*d)->pos >= (*d)->count) {
		delete_dialog(*d);
		*d = NULL;

		// XXX: maybe this can be handled elsewhere?
		if(!global.boss)
			global.timer++;
	}

	return true;
}
