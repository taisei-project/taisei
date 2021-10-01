/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "stageutils.h"

/*
 * NOTE: This is a stage background debugging facility that provides FPS-like keyboard-and-mouse
 * fly-mode camera control. This code is not linked into non-developer builds, and usually is not
 * referenced anywhere — it is expected to be used transiently while working on backgrounds.
 *
 * To use it, simply call `camcontrol_init` in your background animation setup code instead of
 * invoking normal animation code. For best results, disable anything that can alter the camera
 * state directly. Or you can allocate a secondary camera for this purpose.
 *
 * See camcontrol.c for controls and adjustable parameters.
 */

void camcontrol_init(Camera3D *cam);
