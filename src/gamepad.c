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
	bool initialized;
	int current_devnum;
	SDL_GameController *device;
	SDL_JoystickID instance;
	signed char *axis;

	struct {
		int *id_map;
		size_t count;
	} devices;
} gamepad;

static void gamepad_update_device_list(void) {
	int cnt = SDL_NumJoysticks();
	log_info("Updating gamepad devices list");

	free(gamepad.devices.id_map);
	memset(&gamepad.devices, 0, sizeof(gamepad.devices));

	if(!cnt) {
		return;
	}

	int *idmap_ptr = gamepad.devices.id_map = malloc(sizeof(int) * cnt);

	for(int i = 0; i < cnt; ++i) {
		if(!SDL_IsGameController(i)) {
			continue;
		}

		*idmap_ptr = i;
		int num = (int)(uintmax_t)(idmap_ptr - gamepad.devices.id_map);
		++idmap_ptr;
		gamepad.devices.count = (uintptr_t)(idmap_ptr - gamepad.devices.id_map);

		char guid[33] = {0};
		gamepad_deviceguid(num, guid, sizeof(guid));
		log_info("Gamepad device '%s' (#%i): %s", guid, num, SDL_GameControllerNameForIndex(i));
	}
}

static int gamepad_find_device_by_guid(const char *guid_str, char *guid_out, size_t guid_out_sz, int *out_localdevnum) {
	for(int i = 0; i < gamepad.devices.count; ++i) {
		*guid_out = 0;

		SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(gamepad.devices.id_map[i]);
		SDL_JoystickGetGUIDString(guid, guid_out, guid_out_sz);

		if(!*guid_out) {
			log_warn("Failed to read GUID of device %i: %s", gamepad.devices.id_map[i], SDL_GetError());
			continue;
		}

		if(!strcasecmp(guid_str, guid_out) || !strcasecmp(guid_str, "default")) {
			*out_localdevnum = i;
			return gamepad.devices.id_map[i];
		}
	}

	*out_localdevnum = -1;
	return -1;
}

static int gamepad_find_device(char *guid_out, size_t guid_out_sz, int *out_localdevnum) {
	int dev;
	const char *want_guid = config_get_str(CONFIG_GAMEPAD_DEVICE);

	dev = gamepad_find_device_by_guid(want_guid, guid_out, guid_out_sz, out_localdevnum);

	if(dev >= 0) {
		return dev;
	}

	if(strcasecmp(want_guid, "default")) {
		log_warn("Requested device '%s' is not available, trying first usable", want_guid);
		dev = gamepad_find_device_by_guid("default", guid_out, guid_out_sz, out_localdevnum);

		if(dev >= 0) {
			return dev;
		}
	}

	*out_localdevnum = -1;
	strcpy(guid_out, want_guid);

	return -1;
}

static void gamepad_cfg_enabled_callback(ConfigIndex idx, ConfigValue v) {
	config_set_int(idx, v.i);

	if(v.i) {
		gamepad_init();
	} else {
		gamepad_shutdown();
	}
}

static void gamepad_setup_cfg_callbacks(void) {
	static bool done = false;

	if(done) {
		return;
	}

	done = true;

	config_set_callback(CONFIG_GAMEPAD_ENABLED, gamepad_cfg_enabled_callback);
}

void gamepad_init(void) {
	gamepad_setup_cfg_callbacks();

	if(!config_get_int(CONFIG_GAMEPAD_ENABLED) || gamepad.initialized) {
		return;
	}

	if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
		log_warn("SDL_InitSubSystem() failed: %s", SDL_GetError());
		return;
	}

	gamepad_update_device_list();

	char guid[33];
	int dev = gamepad_find_device(guid, sizeof(guid), &gamepad.current_devnum);

	if(dev < 0) {
		log_warn("Device '%s' is not available", guid);
		gamepad_shutdown();
		return;
	}

	gamepad.device = SDL_GameControllerOpen(dev);

	if(!gamepad.device) {
		log_warn("Failed to open device '%s' (#%i): %s", guid, dev, SDL_GetError());
		gamepad_shutdown();
		return;
	}

	SDL_Joystick *joy = SDL_GameControllerGetJoystick(gamepad.device);
	gamepad.instance = SDL_JoystickInstanceID(joy);
	gamepad.axis = malloc(max(SDL_JoystickNumAxes(joy), 1));

	log_info("Using device '%s' (#%i): %s", guid, dev, gamepad_devicename(dev));
	SDL_GameControllerEventState(SDL_ENABLE);

	config_set_str(CONFIG_GAMEPAD_DEVICE, guid);
	gamepad.initialized = true;
}

void gamepad_shutdown(void) {
	if(!gamepad.initialized) {
		return;
	}

	log_info("Disabled the gamepad subsystem");

	if(gamepad.device) {
		SDL_GameControllerClose(gamepad.device);
	}

	free(gamepad.axis);
	free(gamepad.devices.id_map);

	SDL_GameControllerEventState(SDL_IGNORE);
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);

	memset(&gamepad, 0, sizeof(gamepad));
}

void gamepad_restart(void) {
	gamepad_shutdown();
	gamepad_init();
}

int gamepad_axis2gamekey(SDL_GameControllerAxis id, int val) {
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

int gamepad_axis2menuevt(SDL_GameControllerAxis id, int val) {
	if(id == SDL_CONTROLLER_AXIS_LEFTX || id == SDL_CONTROLLER_AXIS_RIGHTX)
		return val == AXISVAL_LEFT ? E_CursorLeft : E_CursorRight;

	if(id == SDL_CONTROLLER_AXIS_LEFTY || id == SDL_CONTROLLER_AXIS_RIGHTY)
		return val == AXISVAL_UP   ? E_CursorUp   : E_CursorDown;

	return -1;
}

int gamepad_axis2gameevt(SDL_GameControllerAxis id) {
	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR))
		return E_PlrAxisLR;

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD))
		return E_PlrAxisUD;

	return -1;
}

float gamepad_axis_sens(SDL_GameControllerAxis id) {
	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR))
		return config_get_float(CONFIG_GAMEPAD_AXIS_LR_SENS);

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD))
		return config_get_float(CONFIG_GAMEPAD_AXIS_UD_SENS);

	return 1.0;
}

void gamepad_axis(SDL_GameControllerAxis id, int raw, EventHandler handler, EventFlags flags, void *arg) {
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

			handler(evt, x, arg);
		}
	}

	if(val) {	// simulate press
		if(!a[id]) {
			a[id] = val;

			if(game && !free) {
				int key = gamepad_axis2gamekey(id, val);
				if(key >= 0) {
					handler(E_PlrKeyDown, key, arg);
				}
			}

			if(menu) {
				int evt = gamepad_axis2menuevt(id, val);
				if(evt >= 0) {
					handler(evt, 0, arg);
				}
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

void gamepad_button(SDL_GameControllerButton button, int state, EventHandler handler, EventFlags flags, void *arg) {
	int menu	= flags & EF_Menu;
	int game	= flags & EF_Game;
	int gpad	= flags & EF_Gamepad;

	int gpkey	= config_gamepad_key_from_gamepad_button(button);
	int key		= config_key_from_gamepad_key(gpkey);

	if(state == SDL_PRESSED) {
		if(game) switch(button) {
			case SDL_CONTROLLER_BUTTON_START:
			case SDL_CONTROLLER_BUTTON_BACK:
				handler(E_Pause, 0, arg); break;

			default:
				if(key >= 0) {
					handler(E_PlrKeyDown, key, arg);
				} break;
		}

		if(menu) switch(button) {
			case SDL_CONTROLLER_BUTTON_DPAD_UP:		handler(E_CursorUp,    0, arg);		break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:	handler(E_CursorDown,  0, arg);		break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:	handler(E_CursorLeft,  0, arg);		break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:	handler(E_CursorRight, 0, arg);		break;

			case SDL_CONTROLLER_BUTTON_A:
			case SDL_CONTROLLER_BUTTON_START:
				handler(E_MenuAccept,  0, arg);	break;

			case SDL_CONTROLLER_BUTTON_B:
			case SDL_CONTROLLER_BUTTON_BACK:
				handler(E_MenuAbort,   0, arg);	break;

			default: break;
		}

		if(gpad) {
			handler(E_GamepadKeyDown, button, arg);
		}
	} else {
		if(game && key >= 0) {
			handler(E_PlrKeyUp, key, arg);
		}

		if(gpad) {
			handler(E_GamepadKeyUp, button, arg);
		}
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
		case SDL_CONTROLLERAXISMOTION:
			if(event->caxis.which == gamepad.instance) {
				val = event->caxis.value;
				vsign = sign(val);
				val = abs(val);

				if(val < minval) {
					val = 0;
				} else {
					val = vsign * clamp((val - minval) / (1.0 - deadzone), 0, GAMEPAD_AXIS_MAX);
				}

				gamepad_axis(event->caxis.axis, val, handler, flags, arg);
			}
		break;

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			if(event->cbutton.which == gamepad.instance) {
				gamepad_button(event->cbutton.button, event->cbutton.state, handler, flags, arg);
			}
		break;
	}
}

int gamepad_devicecount(void) {
	return gamepad.devices.count;
}

const char* gamepad_devicename(int num) {
	if(num < 0 || num >= gamepad.devices.count) {
		return NULL;
	}

	return SDL_GameControllerNameForIndex(gamepad.devices.id_map[num]);
}

const void gamepad_deviceguid(int num, char *guid_str, size_t guid_str_sz) {
	if(num < 0 || num >= gamepad.devices.count) {
		return;
	}

	SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(gamepad.devices.id_map[num]);
	SDL_JoystickGetGUIDString(guid, guid_str, guid_str_sz);
}

int gamepad_numfromguid(const char *guid_str) {
	for(int i = 0; i < gamepad.devices.count; ++i) {
		char guid[33] = {0};
		gamepad_deviceguid(i, guid, sizeof(guid));

		if(!strcasecmp(guid_str, guid)) {
			return i;
		}
	}

	return -1;
}

int gamepad_currentdevice(void) {
	if(gamepad.initialized) {
		return gamepad.current_devnum;
	}

	return -1;
}

bool gamepad_buttonpressed(SDL_GameControllerButton btn) {
	return SDL_GameControllerGetButton(gamepad.device, btn);
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

const char* gamepad_button_name(SDL_GameControllerButton btn) {
	static const char *const map[] = {
		"A",
		"B",
		"X",
		"Y",
		"Back",
		"Guide",
		"Start",
		"Left Stick",
		"Right Stick",
		"Left Bumper",
		"Right Bumper",
		"Up",
		"Down",
		"Left",
		"Right",
	};

	if(btn > SDL_CONTROLLER_BUTTON_INVALID && btn < SDL_CONTROLLER_BUTTON_MAX) {
		return map[btn];
	}

	return "Unknown";
}

const char* gamepad_axis_name(SDL_GameControllerAxis axis) {
	static const char *const map[] = {
		"Left X",
		"Left Y",
		"Right X",
		"Right Y",
		"Left Trigger",
		"Right Trigger",
	};


	if(axis > SDL_CONTROLLER_AXIS_INVALID && axis < SDL_CONTROLLER_AXIS_MAX) {
		return map[axis];
	}

	return "Unknown";
}
