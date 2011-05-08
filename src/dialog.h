/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef DIALOG_H
#define DIALOG_H

#include "texture.h"

struct DialogMessage;
struct DialogSpeaker;

typedef enum {
	Right,
	Left
} Side;

typedef struct DialogMessage {	
	Side side;
	char *msg;
} DialogMessage;

typedef struct Dialog {
	DialogMessage *messages;
	Texture *images[2];
	
	int count;
	int pos;
	
	int page_time;
	
	int birthtime;
} Dialog;

Dialog *create_dialog(char *left, char *right);
void dset_image(Dialog *d, Side side, char *name);
void dadd_msg(Dialog *d, Side side, char *msg);
void delete_dialog(Dialog *d);

void draw_dialog(Dialog *dialog);
void page_dialog(Dialog **d);

#endif