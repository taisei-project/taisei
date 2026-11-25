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
#include "dialog.h"
#include "enemy.h"
#include "framerate.h"
#include "item.h"
#include "lasers/laser.h"
#include "projectile.h"
#include "random.h"
#include "replay/state.h"
#include "stageinfo.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_platform.h>

typedef struct {






} Global;

extern Global global;

void init_global(CLIAction *cli);

void taisei_commit_persistent_data(bool async);

// XXX: Move this somewhere?
bool gamekeypressed(KeyIndex key);
