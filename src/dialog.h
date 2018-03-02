/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <stdbool.h>
#include "resource/sprite.h"

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
	Sprite *images[2];

	int count;
	int pos;

	int page_time;

	int birthtime;
} Dialog;

Dialog *create_dialog(const char *left, const char *right)
	attr_returns_nonnull attr_nodiscard;

void dset_image(Dialog *d, Side side, const char *name)
	attr_nonnull(1, 3);

DialogMessage* dadd_msg(Dialog *d, Side side, const char *msg)
	attr_nonnull(1, 3);

void delete_dialog(Dialog *d)
	attr_nonnull(1);

void draw_dialog(Dialog *dialog)
	attr_nonnull(1);

bool page_dialog(Dialog **d);
