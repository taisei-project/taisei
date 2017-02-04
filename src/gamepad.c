/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "gamepad.h"
#include "taisei_err.h"
#include "config.h"
#include "events.h"
#include "global.h"

static struct {
	int initialized;
	SDL_Joystick *device;
	int axis[GAMEPAD_AXES];
} gamepad;

void gamepad_init(void) {
	memset(&gamepad, 0, sizeof(gamepad));
	
	if(!config_intval("gamepad_enabled"))
		return;
	
	if(gamepad.initialized)
		return;
	
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
		warnx("gamepad_init(): couldn't initialize SDL joystick subsystem: %s", SDL_GetError());
		return;
	}
	
	int i, cnt = gamepad_devicecount();
	printf("gamepad_init(): found %i devices\n", cnt);
	for(i = 0; i < cnt; ++i)
		printf("%i: %s\n", i, SDL_JoystickNameForIndex(i));
	
	int dev = config_intval("gamepad_device");
	if(dev < 0 || dev >= cnt) {
		warnx("gamepad_init(): device %i is not available\n", dev);
		gamepad_shutdown();
		return;
	}
	
	gamepad.device = SDL_JoystickOpen(dev);
	if(!gamepad.device) {
		warnx("gamepad_init(): failed to open device %i [%s]", dev, gamepad_devicename(dev));
		gamepad_shutdown();
		return;
	}
	
	printf("gamepad_init(): using device #%i: %s\n", dev, gamepad_devicename(dev));
	SDL_JoystickEventState(SDL_ENABLE);
	gamepad.initialized = 1;
}

void gamepad_shutdown(void) {
	if(!gamepad.initialized)
		return;
	
	if(gamepad.initialized != 2)
		printf("gamepad_shutdown(): disabled the gamepad subsystem\n");
	
	if(gamepad.device)
		SDL_JoystickClose(gamepad.device);
	
	SDL_JoystickEventState(SDL_IGNORE);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	gamepad.initialized = 0;
}

void gamepad_restart(void) {
	gamepad_shutdown();
	gamepad_init();
}

int gamepad_axis2gamekey(int id, int val) {
	val *= SIGN(gamepad_axis_sens(id));

	if(!val) {
		return -1;
	}

	if(id == tconfig.intval[GAMEPAD_AXIS_LR]) {
		return val == AXISVAL_LEFT ? KEY_LEFT : KEY_RIGHT;
	}
	
	if(id == tconfig.intval[GAMEPAD_AXIS_UD]) {
		return val == AXISVAL_UP   ? KEY_UP   : KEY_DOWN;
	}
	
	return -1;
}

int gamepad_axis2menuevt(int id, int val) {
	if(id == tconfig.intval[GAMEPAD_AXIS_LR])
		return val == AXISVAL_LEFT ? E_CursorLeft : E_CursorRight;
	
	if(id == tconfig.intval[GAMEPAD_AXIS_UD])
		return val == AXISVAL_UP   ? E_CursorUp   : E_CursorDown;
	
	return -1;
}

int gamepad_axis2gameevt(int id) {
	if(id == tconfig.intval[GAMEPAD_AXIS_LR])
		return E_PlrAxisLR;
	
	if(id == tconfig.intval[GAMEPAD_AXIS_UD])
		return E_PlrAxisUD;
	
	return -1;
}

float gamepad_axis_sens(int id) {
	if(id == tconfig.intval[GAMEPAD_AXIS_LR])
		return tconfig.fltval[GAMEPAD_AXIS_LR_SENS];
	
	if(id == tconfig.intval[GAMEPAD_AXIS_UD])
		return tconfig.fltval[GAMEPAD_AXIS_UD_SENS];
	
	return 1.0;
}

void gamepad_axis(int id, int raw, EventHandler handler, EventFlags flags, void *arg) {
	int *a   = gamepad.axis;
	int val  = AXISVAL(raw);
	int free = tconfig.intval[GAMEPAD_AXIS_FREE];
	
	int menu = flags & EF_Menu;
	int game = flags & EF_Game;
	
	if(id >= GAMEPAD_AXES) {
		warnx("gamepad_axis(): axis %i is out of range (%i axes supported)", id, GAMEPAD_AXES);
		return;
	}
	
	//printf("axis: %i %i %i\n", id, val, raw);
	
	if(game && free) {
		int evt = gamepad_axis2gameevt(id);
		if(evt >= 0) {
			double sens = gamepad_axis_sens(id);
			int sens_sign = SIGN(sens);

			double x = raw / (double)GAMEPAD_AXIS_RANGE;
			int in_sign = SIGN(x);

			x = pow(fabs(x), 1.0 / fabs(sens)) * in_sign * sens_sign;
			x = x ? x : 0;
			x = clamp(x * GAMEPAD_AXIS_RANGE, -GAMEPAD_AXIS_RANGE-1, GAMEPAD_AXIS_RANGE);

			handler(evt, x, arg);
		}
	}
	
	if(val) {	// simulate press
		if(!a[id]) {
			a[id] = val;
			
			if(game && !free) {
				int key = gamepad_axis2gamekey(id, val);
				if(key >= 0)
					handler(E_PlrKeyDown, key, arg);
			}
			
			if(menu) {
				int evt = gamepad_axis2menuevt(id, val);
				if(evt >= 0)
					handler(evt, 0, arg);
			}
		}
	} else {	// simulate release
		if(game) {
			int key = gamepad_axis2gamekey(id, a[id]);
			handler(E_PlrKeyUp, key, arg);
		}
		a[id] = AXISVAL_NULL;
	}
}

void gamepad_button(int button, int state, EventHandler handler, EventFlags flags, void *arg) {
	int menu	= flags & EF_Menu;
	int game	= flags & EF_Game;
	int gpad	= flags & EF_Gamepad;
	
	int gpkey	= config_button2gpkey(button);
	int key		= config_gpkey2key(gpkey);
	
	//printf("button: %i %i\n", button, state);
	//printf("gpkey: %i\n", gpkey);
	//printf("key: %i\n", key);
	
	if(state == SDL_PRESSED) {
		if(game) {
			if(gpkey == GP_PAUSE) {
				printf("should pause\n");
				handler(E_Pause, 0, arg);
			} else if(key >= 0)
				handler(E_PlrKeyDown, key, arg);
		}
		
		if(menu) switch(key) {
			case KEY_UP:		handler(E_CursorUp,    0, arg);		break;
			case KEY_DOWN:		handler(E_CursorDown,  0, arg);		break;
			case KEY_LEFT:		handler(E_CursorLeft,  0, arg);		break;
			case KEY_RIGHT:		handler(E_CursorRight, 0, arg);		break;
			case KEY_FOCUS:		handler(E_MenuAbort,   0, arg);		break;
			case KEY_SHOT:		handler(E_MenuAccept,  0, arg);		break;
		}
		
		if(gpad)
			handler(E_GamepadKeyDown, button, arg);
	} else {
		if(game && key >= 0)
			handler(E_PlrKeyUp, key, arg);
		
		if(gpad)
			handler(E_GamepadKeyUp, button, arg);
	}
}

void gamepad_event(SDL_Event *event, EventHandler handler, EventFlags flags, void *arg) {
	if(!gamepad.initialized)
		return;
	
	int val;
	int sens = clamp(tconfig.fltval[GAMEPAD_AXIS_DEADZONE], 0, 1) * GAMEPAD_AXIS_RANGE;
	
	switch(event->type) {
		case SDL_JOYAXISMOTION:
			val = event->jaxis.value;
			if(val < -sens || val > sens || !val)
				gamepad_axis(event->jaxis.axis, val, handler, flags, arg);
		break;
		
		case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP:
			gamepad_button(event->jbutton.button, event->jbutton.state, handler, flags, arg);
		break;
	}
}

int gamepad_devicecount(void) {
	return SDL_NumJoysticks();
}

char* gamepad_devicename(int id) {
	return (char*)SDL_JoystickNameForIndex(id);
}

int gamepad_buttonpressed(int btn) {
	return SDL_JoystickGetButton(gamepad.device, btn);
}

int gamepad_gamekeypressed(int key) {
	if(!gamepad.initialized)
		return 0;
	
	// this sucks
	int i; for(i = CONFIG_GPKEY_FIRST; i <= CONFIG_GPKEY_LAST; ++i)
		if(key == config_gpkey2key(i) && gamepad_buttonpressed(tconfig.intval[i]))
			return 1;
	
	return 0;
}

void gamepad_init_bare(void) {
	if(gamepad.initialized)
		return;
	
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
		warnx("gamepad_init(): couldn't initialize SDL joystick subsystem: %s", SDL_GetError());
		return;
	}
	
	gamepad.initialized = 2;
}

void gamepad_shutdown_bare(void) {
	if(gamepad.initialized == 2)
		gamepad_shutdown();
}
