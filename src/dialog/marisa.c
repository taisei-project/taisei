/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

DIALOG_TASK(marisa, Stage1PreBoss) {
	DIALOG_BEGIN(Stage1PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(cirno);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(cirno);
	VARIANT(cirno, defeated);
	FACE(cirno, defeated);

	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(marisa, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(hina);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(hina);
	VARIANT(hina, defeated);
	FACE(hina, defeated);

	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(marisa, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(wriggle);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(wriggle);
	VARIANT(wriggle, defeated);
	FACE(wriggle, defeated);

	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(marisa, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(kurumi);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(kurumi);
	VARIANT(kurumi, defeated);
	FACE(kurumi, defeated);

	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(marisa, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(iku);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);

	ACTOR_LEFT(marisa);
	FACE(marisa, surprised);

	// should be only one message with a fixed 120-frames timeout
	MSG_UNSKIPPABLE(marisa, 120, "changeme");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(iku);
	VARIANT(iku, defeated);
	FACE(iku, defeated);

	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(marisa, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(elly);

	EVENT(boss_appears);
	EVENT(music_changes);

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(elly);
	VARIANT(elly, beaten);
	FACE(elly, angry);

	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER marisa
#include "export_dialog_tasks.inc.h"
