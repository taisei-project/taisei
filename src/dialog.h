/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include <stdbool.h>
#include "resource/texture.h"

struct DialogMessage;
struct DialogSpeaker;

typedef enum {
	Right,
	Left,
	BGM
} Side;

typedef struct DialogMessage {
	Side side;
	char *msg;
	int timeout;
} DialogMessage;

typedef struct Dialog {
	DialogMessage *messages;
	Texture *images[2];

	int count;
	int pos;

	int page_time;

	int birthtime;
} Dialog;

Dialog *create_dialog(const char *left, const char *right);
void dset_image(Dialog *d, Side side, const char *name);
DialogMessage* dadd_msg(Dialog *d, Side side, const char *msg);
void delete_dialog(Dialog *d);

void draw_dialog(Dialog *dialog);
bool page_dialog(Dialog **d);
