/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "cli.h"
#include "config.h"

void taisei_init(CLIAction *cli);

void taisei_quit(void);
bool taisei_quit_requested(void);

bool taisei_is_quit_hidden(void);
bool taisei_is_kiosk_mode(void);

void taisei_commit_persistent_data(bool async);

// Inputdevice-agnostic method of checking whether a game control is pressed.
// ALWAYS use this instead of SDL_GetKeyState if you need it.
// XXX: Move this somewhere?
bool gamekeypressed(KeyIndex key);
