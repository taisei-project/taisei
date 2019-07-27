/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_dialog_dialog_macros_h
#define IGUARD_dialog_dialog_macros_h

#include "taisei.h"

#include "dialog.h"

#define DIALOG_SCRIPT(name) static void dialog_##name(Dialog *d)
#define MSG(_side, _text) \
	dialog_add_action(d, &(DialogAction) { .type = (_side), .msg = (_text) })
#define FACE(_side, _face) \
	dialog_add_action(d, &(DialogAction) { .type = (_side), .face = DIALOG_FACE_##_face })
#define LEFT(text) MSG(DIALOG_MSG_LEFT, text)
#define RIGHT(text) MSG(DIALOG_MSG_RIGHT, text)
#define LEFT_FACE(face) FACE(DIALOG_SET_FACE_LEFT, face)
#define RIGHT_FACE(face) FACE(DIALOG_SET_FACE_RIGHT, face)

#define EXPORT_DIALOG_SCRIPT(name) \
	PlayerDialogProcs dialog_##name = { \
		.stage1_pre_boss = dialog_##name##_stage1_pre_boss, \
		.stage1_post_boss = dialog_##name##_stage1_post_boss, \
		.stage2_pre_boss = dialog_##name##_stage2_pre_boss, \
		.stage2_post_boss = dialog_##name##_stage2_post_boss, \
		.stage3_pre_boss = dialog_##name##_stage3_pre_boss, \
		.stage3_post_boss = dialog_##name##_stage3_post_boss, \
		.stage4_pre_boss = dialog_##name##_stage4_pre_boss, \
		.stage4_post_boss = dialog_##name##_stage4_post_boss, \
		.stage5_post_midboss = dialog_##name##_stage5_post_midboss, \
		.stage5_pre_boss = dialog_##name##_stage5_pre_boss, \
		.stage5_post_boss = dialog_##name##_stage5_post_boss, \
		.stage6_pre_boss = dialog_##name##_stage6_pre_boss, \
		.stage6_pre_final = dialog_##name##_stage6_pre_final, \
	};

#endif // IGUARD_dialog_dialog_macros_h
