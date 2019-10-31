/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

#define LEFT_BASE youmu

#define RIGHT_BASE cirno

DIALOG_SCRIPT(youmu_stage1_pre_boss) {
	LEFT_FACE(relaxed);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(surprised);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(unamused);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(angry);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(youmu_stage1_post_boss) {
	LEFT_FACE(happy);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	LEFT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE hina

DIALOG_SCRIPT(youmu_stage2_pre_boss) {
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(surprised);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT_FACE(concerned);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(sigh);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT_FACE(serious);
	RIGHT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(youmu_stage2_post_boss) {
	LEFT_FACE(happy);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(relaxed);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	LEFT("FIXME: Removed due to issue #179");
	LEFT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE wriggle

DIALOG_SCRIPT(youmu_stage3_pre_boss) {
	LEFT_FACE(puzzled);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(proud);
	LEFT_FACE(normal);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(eeeeh);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT_FACE(outraged);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(calm);
	LEFT_FACE(relaxed);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(youmu_stage3_post_boss) {
	LEFT_FACE(relaxed);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(happy);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(relaxed);
	RIGHT_FACE(proud);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(unamused);
	LEFT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE kurumi

DIALOG_SCRIPT(youmu_stage4_pre_boss) {
	LEFT_FACE(surprised);
	RIGHT_FACE(tsun);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(normal);
	LEFT_FACE(puzzled);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(tsun_blush);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(dissatisfied);
	LEFT_FACE(unamused);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(normal);
	LEFT_FACE(relaxed);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(tsun_blush);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(sigh);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(tsun);
	LEFT_FACE(normal);
	RIGHT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(youmu_stage4_post_boss) {
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(relaxed);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(tsun_blush);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(dissatisfied);
	LEFT_FACE(happy);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(relaxed);
	RIGHT_FACE(defeated);
	RIGHT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE iku

DIALOG_SCRIPT(youmu_stage5_pre_boss) {
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(eyes_closed);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(normal);
	LEFT_FACE(eeeeh);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(unamused);
	RIGHT_FACE(eyes_closed);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(serious);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(sigh);
	LEFT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(youmu_stage5_post_midboss) {
	// this dialog must contain only one page
	LEFT_FACE(surprised);
	LEFT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(youmu_stage5_post_boss) {
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(relaxed);
	LEFT("FIXME: Removed due to issue #179");
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE elly

DIALOG_SCRIPT(youmu_stage6_pre_boss) {
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(surprised);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(puzzled);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(sigh);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(angry);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(surprised);
	RIGHT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(youmu_stage6_pre_final) {
	RIGHT_FACE(angry);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(puzzled);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(surprised);
	RIGHT_FACE(shouting);
	RIGHT("FIXME: Removed due to issue #179");
}

EXPORT_DIALOG_SCRIPT(youmu)
