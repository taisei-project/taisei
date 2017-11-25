/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>
#include <stdbool.h>
#include "events.h"
#include "config.h"

enum {
    AXISVAL_LEFT  = -1,
    AXISVAL_RIGHT =  1,

    AXISVAL_UP    = -1,
    AXISVAL_DOWN  =  1,

    AXISVAL_NULL  = 0
};

typedef enum {
    PLRAXIS_LR, // aka X
    PLRAXIS_UD, // aka Y
} GamepadPlrAxis;

void gamepad_init(void);
void gamepad_shutdown(void);
void gamepad_restart(void);
float gamepad_axis_sens(int);
void gamepad_event(SDL_Event*, EventHandler, EventFlags, void*);

int gamepad_devicecount(void);
const char* gamepad_devicename(int);
void gamepad_deviceguid(int num, char *guid_str, size_t guid_str_sz);
int gamepad_numfromguid(const char *guid_str);
int gamepad_currentdevice(void);

bool gamepad_buttonpressed(int btn);
bool gamepad_gamekeypressed(KeyIndex key);

const char* gamepad_button_name(SDL_GameControllerButton btn);
const char* gamepad_axis_name(SDL_GameControllerAxis btn);

int gamepad_get_player_axis_value(GamepadPlrAxis paxis);

#define GAMEPAD_AXIS_MAX 32767
#define GAMEPAD_AXIS_MIN -32768
#define AXISVAL sign
