/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <SDL.h>
#include <stdbool.h>
#include "events.h"
#include "config.h"

void gamepad_init(void);
void gamepad_shutdown(void);
void gamepad_restart(void);
float gamepad_axis_sens(int);
void gamepad_event(SDL_Event*, EventHandler, EventFlags, void*);

int gamepad_devicecount(void);
const char* gamepad_devicename(int);
const void gamepad_deviceguid(int num, char *guid_str, size_t guid_str_sz);
int gamepad_numfromguid(const char *guid_str);
int gamepad_currentdevice(void);

bool gamepad_buttonpressed(int btn);
bool gamepad_gamekeypressed(KeyIndex key);

const char* gamepad_button_name(SDL_GameControllerButton btn);
const char* gamepad_axis_name(SDL_GameControllerAxis btn);

// shitty workaround for the options menu. Used to list devices while the gamepad subsystem is off.
// only initializes the SDL subsystem so you can use gamepad_devicecount/gamepad_devicename.
// if gamepad has been initialized already, these do nothing.
void gamepad_init_bare(void);
void gamepad_shutdown_bare(void);

enum {
	AXISVAL_LEFT  = -1,
	AXISVAL_RIGHT =  1,

	AXISVAL_UP    = -1,
	AXISVAL_DOWN  =  1,

	AXISVAL_NULL  = 0
};

#define GAMEPAD_AXIS_MAX 32767
#define GAMEPAD_AXIS_MIN -32768
#define AXISVAL sign

#endif
