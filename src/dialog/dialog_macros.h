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

// NOTE: partial compatibility layer with the old dialog system; to be removed.

#define DIALOG_EVAL(a, b) a b

#define DIALOG_SCRIPT_(name, base_left, base_right) \
	static void _dialog_##name##_script(Dialog *dialog, DialogActor *a_left, DialogActor *a_right); \
	TASK(dialog_##name, { Dialog *d; }) { \
		Dialog *d = ARGS.d; \
		DialogActor a_left, a_right; \
		dialog_add_actor(d, &a_left, #base_left, DIALOG_SIDE_LEFT); \
		dialog_add_actor(d, &a_right, #base_right, DIALOG_SIDE_RIGHT); \
		_dialog_##name##_script(d, &a_left, &a_right); \
		dialog_end(d); \
	} \
	static void dialog_##name(Dialog *d) { \
		INVOKE_TASK(dialog_##name, d); \
	} \
	static void _dialog_##name##_script(Dialog *dialog, DialogActor *a_left, DialogActor *a_right)

#define DIALOG_SCRIPT(name)\
	DIALOG_EVAL(DIALOG_SCRIPT_, (name, LEFT_BASE, RIGHT_BASE))

#define LEFT(text) dialog_message(dialog, a_left, text);
#define RIGHT(text) dialog_message(dialog, a_right, text);
#define LEFT_FACE(face) dialog_actor_set_face(a_left, #face);
#define RIGHT_FACE(face) dialog_actor_set_face(a_right, #face);

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
