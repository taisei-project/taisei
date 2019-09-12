/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_dialog_h
#define IGUARD_dialog_h

#include "taisei.h"

#include "resource/sprite.h"

struct DialogAction;

typedef enum {
	DIALOG_RIGHT,
	DIALOG_LEFT,
} DialogSide;

typedef enum {
	DIALOG_MSG_RIGHT = DIALOG_RIGHT,
	DIALOG_MSG_LEFT = DIALOG_LEFT,
	DIALOG_SET_BGM
} DialogActionType;

typedef struct DialogAction {
	DialogActionType type;
	char *msg;
	int timeout;
} DialogAction;

typedef struct Dialog {
	DialogAction *actions;
	Sprite *images[2];

	int count;
	int pos;
	int page_time;
	int birthtime;

	float opacity;
} Dialog;

Dialog *dialog_create(void)
	attr_returns_allocated;

void dialog_set_image(Dialog *d, DialogSide side, const char *name)
	attr_nonnull(1);

DialogAction *dialog_add_action(Dialog *d, DialogActionType side, const char *msg)
	attr_nonnull(1, 3);

void dialog_destroy(Dialog *d)
	attr_nonnull(1);

void dialog_draw(Dialog *dialog);

bool dialog_page(Dialog **d) attr_nonnull(1);
void dialog_update(Dialog **d) attr_nonnull(1);

bool dialog_is_active(Dialog *d);

void dialog_preload(void);

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
