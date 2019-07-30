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
	DIALOG_SET_BGM,
	DIALOG_SET_FACE_RIGHT,
	DIALOG_SET_FACE_LEFT,
} DialogActionType;

typedef struct DialogAction {
	DialogActionType type;
	const char *data;
	int timeout;
} DialogAction;

typedef struct Dialog {
	DialogAction *actions;
	Sprite *spr_base[2];
	Sprite *spr_face[2];

	int count;
	int pos;
	int page_time;
	int birthtime;

	float opacity;
} Dialog;

Dialog *dialog_create(void)
	attr_returns_allocated;

void dialog_set_base(Dialog *d, DialogSide side, const char *sprite)
	attr_nonnull(1);

void dialog_set_base_p(Dialog *d, DialogSide side, Sprite *sprite)
	attr_nonnull(1);

void dialog_set_face(Dialog *d, DialogSide side, const char *sprite)
	attr_nonnull(1);

void dialog_set_face_p(Dialog *d, DialogSide side, Sprite *sprite)
	attr_nonnull(1);

void dialog_set_char(Dialog *d, DialogSide side, const char *char_name, const char *char_face, const char *char_variant)
	attr_nonnull(1, 3, 4);

DialogAction *dialog_add_action(Dialog *d, const DialogAction *action)
	attr_nonnull(1, 2);

void dialog_destroy(Dialog *d)
	attr_nonnull(1);

void dialog_draw(Dialog *dialog);

bool dialog_page(Dialog **d) attr_nonnull(1);
void dialog_update(Dialog **d) attr_nonnull(1);

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
