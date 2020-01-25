/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

/*
 * Stage 1
 */

DIALOG_TASK(reimu, Stage1PreBoss) {
	// Initialization, must be at the very top.
	DIALOG_BEGIN(Stage1PreBoss);

	// Actors start hidden by default.
	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(cirno);

	// SHOW() can be used to reveal them.
	SHOW(reimu);

	// HIDE() can be used to hide them again.
	//	HIDE(reimu);

	// "normal" is the default face.
	// FACE(reimu, normal);

	// Let's wait a bit, then change Reimu's expression.
	// All timings are in frames (60 = 1 sec).
	WAIT_SKIPPABLE(60);
	FACE(reimu, unamused);

	// MSG() makes the actor say a line, and then waits an unspecified amount of time (skippable).
	// The timeout is determined by the dialog_util_estimate_wait_timeout_from_text() function in dialog.c
	MSG(reimu, "yikes");

	// EVENT()s are handled by stage code.
	// You can find the list of events per dialogue in dialog_interface.h
	// All of them should be signaled eventually.
	EVENT(boss_appears);

	// Wait until the boss slides in.
	// WAIT() can not be skipped.
	WAIT(30);

	// MSG() also implies SHOW()
	MSG(cirno, "9");

	// Titles are not yet implemented, but this should work once they are.
	// Right now this does nothing.
	TITLE(cirno, "Cirno", "Ice Fairy of Big Dum Dum");

	WAIT_SKIPPABLE(30);

	// MSG_UNSKIPPABLE() is like MSG(), but can't be skipped and takes an explicit timeout.
	MSG_UNSKIPPABLE(cirno, 60, "asdfasdfsdfas");

	EVENT(music_changes);
	MSG(reimu, "omg");
	FACE(reimu, surprised);
	MSG(reimu, "wtf");

	// Teardown, must be at the very bottom.
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(cirno);

	// VARIANT() changes the base body sprite.
	// Bosses currently only have a "defeated" variant with some clothing damage, to be used in post-battle scenes.
	// Elly's is called "beaten" and is slightly subdued.
	VARIANT(cirno, defeated);

	// Bosses also have a "defeated" face to go along with the variant, but all the other faces can be used as well.
	// It's best to set the face to "defeated" in the beginning of a post-battle dialogue, and change it later if needed.
	FACE(cirno, defeated);

	SHOW(cirno);

	FACE(reimu, happy);
	MSG(reimu, "git rekt skrub :^)");
	MSG(cirno, "fugg");
	HIDE(cirno);
	MSG(reimu, "lalala~");

	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(reimu, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(hina);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(hina);
	VARIANT(hina, defeated);
	FACE(hina, defeated);

	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(reimu, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(wriggle);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(wriggle);
	VARIANT(wriggle, defeated);
	FACE(wriggle, defeated);

	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(reimu, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(kurumi);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(kurumi);
	VARIANT(kurumi, defeated);
	FACE(kurumi, defeated);

	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(reimu, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(iku);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);

	ACTOR_LEFT(reimu);
	FACE(reimu, surprised);

	// should be only one message with a fixed 120-frames timeout
	MSG_UNSKIPPABLE(reimu, 120, "changeme");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(iku);
	VARIANT(iku, defeated);
	FACE(iku, defeated);

	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(reimu, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(elly);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(elly);
	VARIANT(elly, beaten);
	FACE(elly, angry);

	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER reimu
#include "export_dialog_tasks.inc.h"
