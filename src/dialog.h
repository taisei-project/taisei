/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_dialog_h
#define IGUARD_dialog_h

#include "taisei.h"

#include "resource/sprite.h"

struct DialogMessage;

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

	float opacity;
} Dialog;

Dialog *create_dialog(const char *left, const char *right)
	attr_returns_nonnull attr_nodiscard;

void dset_image(Dialog *d, Side side, const char *name)
	attr_nonnull(1, 3);

DialogMessage* dadd_msg(Dialog *d, Side side, const char *msg)
	attr_nonnull(1, 3);

void delete_dialog(Dialog *d)
	attr_nonnull(1);

void draw_dialog(Dialog *dialog);

bool page_dialog(Dialog **d) attr_nonnull(1);
void process_dialog(Dialog **d) attr_nonnull(1);

bool dialog_is_active(Dialog *d);

// FIXME: might not be the best place for this
typedef struct PlayerDialogProcs {
	void (*stage1_pre_boss)(Dialog *d);
	void (*stage1_post_boss)(Dialog *d);
	void (*stage2_pre_boss)(Dialog *d);
	void (*stage2_post_boss)(Dialog *d);
	void (*stage3_pre_boss)(Dialog *d);
	void (*stage3_post_boss)(Dialog *d);
	void (*stage4_pre_boss)(Dialog *d);
	void (*stage4_post_boss)(Dialog *d);
	void (*stage5_post_midboss)(Dialog *d);
	void (*stage5_pre_boss)(Dialog *d);
	void (*stage5_post_boss)(Dialog *d);
	void (*stage6_pre_boss)(Dialog *d);
	void (*stage6_pre_final)(Dialog *d);
} PlayerDialogProcs;

#endif // IGUARD_dialog_h
