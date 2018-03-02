/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "../common/opengl.h"

#define GL33_MAX_TEXUNITS 8

// Internal helper functions

uint gl33_active_texunit(void);
uint gl33_activate_texunit(uint unit);
