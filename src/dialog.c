/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "dialog.h"
#include "global.h"
#include <stdlib.h>
#include <string.h>

Dialog *create_dialog(char *left, char *right) {
	Dialog *d = malloc(sizeof(Dialog));
	memset(d, 0, sizeof(Dialog));

	if(left)
		d->images[Left] = get_tex(left);
	if(right)
		d->images[Right] = get_tex(right);

	d->page_time = global.frames;
	d->birthtime = global.frames;
	return d;
}

void dset_image(Dialog *d, Side side, char *name) {
	d->images[side] = get_tex(name);
}

void dadd_msg(Dialog *d, Side side, char *msg) {
	d->messages = realloc(d->messages, (++d->count)*sizeof(DialogMessage));
	d->messages[d->count-1].side = side;
	d->messages[d->count-1].msg = malloc(strlen(msg) + 1);
	strlcpy(d->messages[d->count-1].msg, msg, strlen(msg) + 1);
}

void delete_dialog(Dialog *d) {
	int i;
	for(i = 0; i < d->count; i++)
		free(d->messages[i].msg);

	free(d->messages);
	free(d);
}

void draw_dialog(Dialog *dialog) {
	glPushMatrix();

	glTranslatef(VIEWPORT_W/2.0, VIEWPORT_H*3.0/4.0, 0);

	int i;
	for(i = 0; i < 2; i++) {
		glPushMatrix();
		if(i == Left) {
			glCullFace(GL_FRONT);
			glScalef(-1,1,1);
		}

		if(global.frames - dialog->birthtime < 30)
			glTranslatef(120 - (global.frames - dialog->birthtime)*4, 0, 0);

		int cur = dialog->messages[dialog->pos].side;
		int pre = 2;
		if(dialog->pos > 0)
			pre = dialog->messages[dialog->pos-1].side;

		short dir = (1 - 2*(i == dialog->messages[dialog->pos].side));
		if(global.frames - dialog->page_time < 10 && ((i != pre && i == cur) || (i == pre && i != cur))) {
			int time = (global.frames - dialog->page_time) * dir;
			glTranslatef(time, time, 0);
			float clr = 1.0 - 0.07*time;
			glColor3f(clr, clr, clr);
		} else {
			glTranslatef(dir*10, dir*10, 0);
			glColor3f(1 - dir*0.7, 1 - dir*0.7, 1 - dir*0.7);
		}

		glTranslatef(VIEWPORT_W*7.0/18.0, 0, 0);
		if(dialog->images[i])
			draw_texture_p(0, 0, dialog->images[i]);
		glPopMatrix();

		glColor3f(1,1,1);
	}



	glCullFace(GL_BACK);
	glPopMatrix();

	glPushMatrix();
	if(global.frames - dialog->birthtime < 25)
		glTranslatef(0, 100-(global.frames-dialog->birthtime)*4, 0);
	glColor4f(0,0,0,0.8);

	glPushMatrix();
	glTranslatef(VIEWPORT_W/2, VIEWPORT_H-75, 0);
	glScalef(VIEWPORT_W-40, 110, 1);

	draw_quad();
	glPopMatrix();
	glColor4f(1,1,1,1);

	if(dialog->messages[dialog->pos].side == Right)
		glColor3f(0.6,0.6,1);
	draw_text(AL_Center, VIEWPORT_W/2, VIEWPORT_H-110, dialog->messages[dialog->pos].msg, _fonts.standard);

	if(dialog->messages[dialog->pos].side == Right)
		glColor3f(1,1,1);
	glPopMatrix();
}

void page_dialog(Dialog **d) {
	(*d)->pos++;
	(*d)->page_time = global.frames;

	if((*d)->pos >= (*d)->count) {
		delete_dialog(*d);
		*d = NULL;
		if(!global.boss)
			global.timer++;
	}
}
