/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "memory/arena.h"

typedef struct ChoiceDialogParams {
	const char *window_title;
	const char *ok_label;
	const char *cancel_label;
	const char *text;
	const char **choices;
	unsigned int num_choices;
	unsigned int initial_selection;
} ChoiceDialogParams;

int show_choice_dialog(MemArena *arena, const ChoiceDialogParams *params);
