/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "gamepad.h"
#include "config.h"
#include "events.h"
#include "global.h"

static struct {
	int initialized;
	SDL_Joystick *device;
	signed char *axis;
} gamepad;

void gamepad_init(void) {
	memset(&gamepad, 0, sizeof(gamepad));

	if(!config_get_int(CONFIG_GAMEPAD_ENABLED))
		return;

	if(gamepad.initialized)
		return;

	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
		log_warn("SDL_InitSubSystem() failed: %s", SDL_GetError());
		return;
	}

	int i, cnt = gamepad_devicecount();
	log_info("Found %i devices", cnt);
	for(i = 0; i < cnt; ++i)
		log_info("Device #%i: %s", i, SDL_JoystickNameForIndex(i));

	int dev = config_get_int(CONFIG_GAMEPAD_DEVICE);
	if(dev < 0 || dev >= cnt) {
		log_warn("Device #%i is not available", dev);
		gamepad_shutdown();
		return;
	}

	gamepad.device = SDL_JoystickOpen(dev);
	if(!gamepad.device) {
		log_warn("Failed to open device %i: %s", dev, SDL_GetError());
		gamepad_shutdown();
		return;
	}

	gamepad.axis = reinterpret_cast<signed char*>(malloc(SDL_JoystickNumAxes(gamepad.device)));

	log_info("Using device #%i: %s", dev, gamepad_devicename(dev));
	SDL_JoystickEventState(SDL_ENABLE);
	gamepad.initialized = 1;
}

void gamepad_shutdown(void) {
	if(!gamepad.initialized)
		return;

	if(gamepad.initialized != 2)
		log_info("Disabled the gamepad subsystem");

	if(gamepad.device)
		SDL_JoystickClose(gamepad.device);

	free(gamepad.axis);
	gamepad.axis = NULL;

	SDL_JoystickEventState(SDL_IGNORE);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	gamepad.initialized = 0;
}

void gamepad_restart(void) {
	gamepad_shutdown();
	gamepad_init();
}

int gamepad_axis2gamekey(int id, int val) {
	val *= sign(gamepad_axis_sens(id));

	if(!val) {
		return -1;
	}

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR)) {
		return val == AXISVAL_LEFT ? KEY_LEFT : KEY_RIGHT;
	}

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD)) {
		return val == AXISVAL_UP   ? KEY_UP   : KEY_DOWN;
	}

	return -1;
}

int gamepad_axis2menuevt(int id, int val) {
	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR))
		return val == AXISVAL_LEFT ? E_CursorLeft : E_CursorRight;

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD))
		return val == AXISVAL_UP   ? E_CursorUp   : E_CursorDown;

	return -1;
}

int gamepad_axis2gameevt(int id) {
	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR))
		return E_PlrAxisLR;

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD))
		return E_PlrAxisUD;

	return -1;
}

float gamepad_axis_sens(int id) {
	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR))
		return config_get_float(CONFIG_GAMEPAD_AXIS_LR_SENS);

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD))
		return config_get_float(CONFIG_GAMEPAD_AXIS_UD_SENS);

	return 1.0;
}

void gamepad_axis(int id, int raw, EventHandler handler, EventFlags flags, void *arg) {
	signed char *a   = gamepad.axis;
	signed char val  = AXISVAL(raw);
	bool free = config_get_int(CONFIG_GAMEPAD_AXIS_FREE);

	bool menu = flags & EF_Menu;
	bool game = flags & EF_Game;
	bool gp   = flags & EF_Gamepad;

	//printf("axis: %i %i %i\n", id, val, raw);

	if(game && free) {
		int evt = gamepad_axis2gameevt(id);
		if(evt >= 0) {
			double sens = gamepad_axis_sens(id);
			int sens_sign = sign(sens);

			double x = raw / (double)GAMEPAD_AXIS_MAX;
			int in_sign = sign(x);

			x = pow(fabs(x), 1.0 / fabs(sens)) * in_sign * sens_sign;
			x = x ? x : 0;
			x = clamp(x * GAMEPAD_AXIS_MAX, GAMEPAD_AXIS_MIN, GAMEPAD_AXIS_MAX);

			handler(static_cast<EventType>(evt), x, arg);
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
					handler(static_cast<EventType>(evt), static_cast<EventType>(0), arg);
			}
		}
	} else {	// simulate release
		if(game) {
			int key = gamepad_axis2gamekey(id, a[id]);
			handler(E_PlrKeyUp, key, arg);
		}
		a[id] = AXISVAL_NULL;
	}

	if(gp) {
		 // we probably need a better way to pass more than an int to the handler...
		handler(E_GamepadAxis, id, arg);
		handler(E_GamepadAxisValue, raw, arg);
	}
}

void gamepad_button(int button, int state, EventHandler handler, EventFlags flags, void *arg) {
	int menu	= flags & EF_Menu;
	int game	= flags & EF_Game;
	int gpad	= flags & EF_Gamepad;

	int gpkey	= config_gamepad_key_from_gamepad_button(button);
	int key		= static_cast<int>(config_key_from_gamepad_key(static_cast<GamepadKeyIndex>(gpkey)));

	//printf("button: %i %i\n", button, state);
	//printf("gpkey: %i\n", gpkey);
	//printf("key: %i\n", key);

	if(state == SDL_PRESSED) {
		if(game) {
			if(gpkey == GAMEPAD_KEY_PAUSE) {
				handler(E_Pause, 0, arg);
			} else if(key >= 0) {
				handler(E_PlrKeyDown, key, arg);
			}
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
	int vsign;
	float deadzone = clamp(config_get_float(CONFIG_GAMEPAD_AXIS_DEADZONE), 0, 0.999);
	int minval = clamp(deadzone, 0, 1) * GAMEPAD_AXIS_MAX;

	switch(event->type) {
		case SDL_JOYAXISMOTION:
			val = event->jaxis.value;
			vsign = sign(val);
			val = abs(val);

			if(val < minval) {
				val = 0;
			} else {
				val = vsign * clamp((val - minval) / (1.0 - deadzone), 0, GAMEPAD_AXIS_MAX);
			}

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

bool gamepad_buttonpressed(int btn) {
	return SDL_JoystickGetButton(gamepad.device, btn);
}

bool gamepad_gamekeypressed(KeyIndex key) {
	if(!gamepad.initialized)
		return false;

	int gpkey = config_gamepad_key_from_key(key);

	if(gpkey < 0) {
		return false;
	}

	int cfgidx = GPKEYIDX_TO_CFGIDX(gpkey);
	int button = config_get_int(cfgidx);

	return gamepad_buttonpressed(button);
}

void gamepad_init_bare(void) {
	if(gamepad.initialized)
		return;

	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
		log_warn("SDL_InitSubSystem() failed: %s", SDL_GetError());
		return;
	}

	gamepad.initialized = 2;
}

void gamepad_shutdown_bare(void) {
	if(gamepad.initialized == 2)
		gamepad_shutdown();
}
