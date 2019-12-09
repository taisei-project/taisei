/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

#define LEFT_BASE marisa

#define RIGHT_BASE cirno

DIALOG_SCRIPT(marisa_stage1_pre_boss) {
	LEFT_FACE(puzzled);
	LEFT("Aw man, snow again? I just put away my winter coat.");
	RIGHT_FACE(normal);
	RIGHT("Nice, right? It's my snow. I did it!");
	LEFT_FACE(normal);
	LEFT("Mind if I borrow some money for the drycleaning bill? I was gonna get it done in the fall, but now…");
	RIGHT("No, but I can lend you some ice cream! After I turn *you* into ice cream!");
	LEFT_FACE(smug);
	LEFT("I prefer shaved ice, myself.");
}

DIALOG_SCRIPT(marisa_stage1_post_boss) {
	LEFT_FACE(smug);
	RIGHT_FACE(defeated);
	LEFT("Why don't ya just go play with your new fairy friend, the one from Hell? I'm sure she'd love to test your, uh, 'invincibility.'");
	RIGHT("Ouch- I mean, last time she lost! And next time you will too!");
	LEFT("Sure, sure.");
}

#undef RIGHT_BASE
#define RIGHT_BASE hina

DIALOG_SCRIPT(marisa_stage2_pre_boss) {
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(concerned);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(unamused);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(serious);
	LEFT_FACE(puzzled);
	RIGHT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(marisa_stage2_post_boss) {
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(happy);
	LEFT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE wriggle

DIALOG_SCRIPT(marisa_stage3_pre_boss) {
	LEFT_FACE(puzzled);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(outraged);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(proud);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT_FACE(calm);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(proud);
	RIGHT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(marisa_stage3_post_boss) {
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(outraged_unlit);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(happy);
	RIGHT_FACE(defeated);
	LEFT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE kurumi

DIALOG_SCRIPT(marisa_stage4_pre_boss) {
	LEFT_FACE(surprised);
	RIGHT_FACE(tsun);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(normal);
	LEFT_FACE(normal);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(tsun_blush);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(dissatisfied);
	LEFT_FACE(happy);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(tsun);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(sweat_smile);
	LEFT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(marisa_stage4_post_boss) {
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(dissatisfied);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(happy);
	LEFT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE iku

DIALOG_SCRIPT(marisa_stage5_pre_boss) {
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(puzzled);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(serious);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(eyes_closed);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(happy);
	RIGHT_FACE(normal);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT_FACE(eyes_closed);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT_FACE(smile);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(surprised);
	RIGHT_FACE(serious);
	RIGHT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(marisa_stage5_post_midboss) {
	// this dialog must contain only one page
	LEFT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(marisa_stage5_post_boss) {
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(happy);
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(sweat_smile);
	LEFT("FIXME: Removed due to issue #179");
}

#undef RIGHT_BASE
#define RIGHT_BASE elly

DIALOG_SCRIPT(marisa_stage6_pre_boss) {
	LEFT("FIXME: Removed due to issue #179");
	LEFT_FACE(surprised);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(puzzled);
	RIGHT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(normal);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT_FACE(angry);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(smug);
	LEFT("FIXME: Removed due to issue #179");
	RIGHT("FIXME: Removed due to issue #179");
}

DIALOG_SCRIPT(marisa_stage6_pre_final) {
	RIGHT_FACE(angry);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(puzzled);
	RIGHT("FIXME: Removed due to issue #179");
	LEFT_FACE(surprised);
	RIGHT_FACE(shouting);
	RIGHT("FIXME: Removed due to issue #179");
}

EXPORT_DIALOG_SCRIPT(marisa)
