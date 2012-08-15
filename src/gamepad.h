/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef GAMEPAD_H
#define GAMEPAD_H

#include "events.h"

void gamepad_init(void);
void gamepad_shutdown(void);
void gamepad_restart(void);
void gamepad_event(SDL_Event*, EventHandler, EventFlags, void*);

enum {
	AXISVAL_LEFT  = -1,
	AXISVAL_RIGHT =  1,
	
	AXISVAL_UP    = -1,
	AXISVAL_DOWN  =  1,
	
	AXISVAL_NULL  = 0
};

#define GAMEPAD_AXES 16
#define AXISVAL(x) ((x > 0) - (x < 0))

#endif
