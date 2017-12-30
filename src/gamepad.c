/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

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

static bool gamepad_event_handler(SDL_Event *event, void *arg);

static int gamepad_load_mappings(const char *vpath, int warn_noexist) {
	char *repr = vfs_repr(vpath, true);
	char *errstr = NULL;
	const char *const_errstr = NULL;

	SDL_RWops *mappings = vfs_open(vpath, VFS_MODE_READ | VFS_MODE_SEEKABLE);
	int num_loaded = -1;
	LogLevel loglvl = LOG_WARN;

	// Yay for gotos!

	if(!mappings) {
		if(!warn_noexist) {
			VFSInfo vinfo = vfs_query(vpath);

			if(!vinfo.error && !vinfo.exists && !vinfo.is_dir) {
				loglvl = LOG_INFO;
				const_errstr = errstr = strfmt("Custom mappings file '%s' does not exist (this is ok)", repr);
				goto cleanup;
			}
		}

		const_errstr = vfs_get_error();
		goto cleanup;
	}

	if((num_loaded = SDL_GameControllerAddMappingsFromRW(mappings, true)) < 0) {
		const_errstr = SDL_GetError();
	}

cleanup:
	if(const_errstr) {
		log_custom(loglvl, "Couldn't load mappings: %s", const_errstr);
	} else if(num_loaded >= 0) {
		log_info("Loaded %i mappings from '%s'", num_loaded, repr);
	}

	free(repr);
	free(errstr);
	return num_loaded;
}

static void gamepad_load_all_mappings(void) {
	gamepad_load_mappings("res/gamecontrollerdb.txt", true);
	gamepad_load_mappings("storage/gamecontrollerdb.txt", false);
}

static const char* gamepad_device_name_unmapped(int idx) {
	const char *name = SDL_GameControllerNameForIndex(idx);

	if(!strcasecmp(name, "Xinput Controller")) {
		// HACK: let's try to get a more descriptive name...
		name = SDL_JoystickNameForIndex(idx);
	}

	return name;
}

const char* gamepad_device_name(int num) {
	if(num < 0 || num >= gamepad.devices.count) {
		return NULL;
	}

	return gamepad_device_name_unmapped(gamepad.devices.id_map[num]);
}


static void gamepad_update_device_list(void) {
	int cnt = SDL_NumJoysticks();
	log_info("Updating gamepad devices list");

	free(gamepad.devices.id_map);
	memset(&gamepad.devices, 0, sizeof(gamepad.devices));

	if(!cnt) {
		log_info("No joysticks attached");
		return;
	}

	int *idmap_ptr = gamepad.devices.id_map = malloc(sizeof(int) * cnt);

	for(int i = 0; i < cnt; ++i) {
		SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(i);
		char guid_str[33] = { 0 };
		SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));

		if(!*guid_str) {
			log_warn("Failed to read GUID of joystick at index %i: %s", i, SDL_GetError());
			continue;
		}

		if(!SDL_IsGameController(i)) {
			log_warn("Joystick at index %i (name: \"%s\"; guid: %s) is not recognized as a game controller by SDL. "
					 "Most likely it just doesn't have a mapping. See https://git.io/vdvdV for solutions",
				i, SDL_JoystickNameForIndex(i), guid_str);
			continue;
		}

		*idmap_ptr = i;
		int num = (int)(uintmax_t)(idmap_ptr - gamepad.devices.id_map);
		++idmap_ptr;
		gamepad.devices.count = (uintptr_t)(idmap_ptr - gamepad.devices.id_map);

		log_info("Found device '%s' (#%i): %s", guid_str, num, gamepad_device_name_unmapped(i));
	}

	if(!gamepad.devices.count) {
		log_info("No usable devices");
		return;
	}
}

static int gamepad_find_device_by_guid(const char *guid_str, char *guid_out, size_t guid_out_sz, int *out_localdevnum) {
	for(int i = 0; i < gamepad.devices.count; ++i) {
		*guid_out = 0;

		SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(gamepad.devices.id_map[i]);
		SDL_JoystickGetGUIDString(guid, guid_out, guid_out_sz);

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

	gamepad_load_all_mappings();
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
	gamepad.axis = calloc(GAMEPAD_AXIS_MAX, 1);

	log_info("Using device '%s' (#%i): %s", guid, dev, gamepad_device_name(dev));
	SDL_GameControllerEventState(SDL_ENABLE);

	config_set_str(CONFIG_GAMEPAD_DEVICE, guid);

	events_register_handler(&(EventHandler){
		.proc = gamepad_event_handler,
		.priority = EPRIO_TRANSLATION,
	});

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
	events_unregister_handler(gamepad_event_handler);
}

void gamepad_restart(void) {
	gamepad_shutdown();
	gamepad_init();
}

static float gamepad_axis_sens(GamepadAxis id) {
	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR))
		return config_get_float(CONFIG_GAMEPAD_AXIS_LR_SENS);

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD))
		return config_get_float(CONFIG_GAMEPAD_AXIS_UD_SENS);

	return 1.0;
}

static int gamepad_axis2gamekey(GamepadAxis id, int val) {
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

static GamepadAxis gamepad_gamekey2axis(KeyIndex key) {
	switch(key) {
		case KEY_UP:	case KEY_DOWN:	return config_get_int(CONFIG_GAMEPAD_AXIS_UD);
		case KEY_LEFT:	case KEY_RIGHT:	return config_get_int(CONFIG_GAMEPAD_AXIS_LR);
		default:						return GAMEPAD_AXIS_INVALID;
	}
}

static int gamepad_gamekey2axisval(KeyIndex key) {
	switch(key) {
		case KEY_UP:	return AXISVAL_UP;
		case KEY_DOWN:	return AXISVAL_DOWN;
		case KEY_LEFT:	return AXISVAL_LEFT;
		case KEY_RIGHT:	return AXISVAL_RIGHT;
		default:		return AXISVAL_NULL;
	}
}

static int gamepad_axis2menuevt(GamepadAxis id, int val) {
	if(id == GAMEPAD_AXIS_LEFT_X || id == GAMEPAD_AXIS_RIGHT_X)
		return val == AXISVAL_LEFT ? TE_MENU_CURSOR_LEFT : TE_MENU_CURSOR_RIGHT;

	if(id == GAMEPAD_AXIS_LEFT_Y || id == GAMEPAD_AXIS_RIGHT_Y)
		return val == AXISVAL_UP   ? TE_MENU_CURSOR_UP : TE_MENU_CURSOR_DOWN;

	return -1;
}

static int gamepad_axis2gameevt(GamepadAxis id) {
	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR))
		return TE_GAME_AXIS_LR;

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD))
		return TE_GAME_AXIS_UD;

	return -1;
}

static int gamepad_axis_process_value_deadzone(int raw) {
	int val, vsign;
	float deadzone = clamp(config_get_float(CONFIG_GAMEPAD_AXIS_DEADZONE), 0.01, 0.999);
	int minval = clamp(deadzone, 0, 1) * GAMEPAD_AXIS_MAX_VALUE;

	val = raw;
	vsign = sign(val);
	val = abs(val);

	if(val < minval) {
		val = 0;
	} else {
		val = vsign * clamp((val - minval) / (1.0 - deadzone), 0, GAMEPAD_AXIS_MAX_VALUE);
	}

	return val;
}

static int gamepad_axis_process_value(GamepadAxis id, int raw) {
	double sens = gamepad_axis_sens(id);
	int sens_sign = sign(sens);

	raw = gamepad_axis_process_value_deadzone(raw);

	double x = raw / (double)GAMEPAD_AXIS_MAX_VALUE;
	int in_sign = sign(x);

	x = pow(fabs(x), 1.0 / fabs(sens)) * in_sign * sens_sign;
	x = x ? x : 0;
	x = clamp(x * GAMEPAD_AXIS_MAX_VALUE, GAMEPAD_AXIS_MIN_VALUE, GAMEPAD_AXIS_MAX_VALUE);

	return (int)x;
}

int gamepad_axis_value(GamepadAxis id) {
	SDL_GameControllerAxis sdl_id = gamepad_axis_to_sdl_axis(id);
	return gamepad_axis_process_value(id, SDL_GameControllerGetAxis(gamepad.device, sdl_id));
}

int gamepad_player_axis_value(GamepadPlrAxis paxis) {
	GamepadAxis id;

	if(!gamepad.initialized) {
		return 0;
	}

	if(paxis == PLRAXIS_LR) {
		id = config_get_int(CONFIG_GAMEPAD_AXIS_LR);
	} else if(paxis == PLRAXIS_UD) {
		id = config_get_int(CONFIG_GAMEPAD_AXIS_UD);
	} else {
		return INT_MAX;
	}

	return gamepad_axis_value(id);
}

static void gamepad_button(GamepadButton button, int state);

static void gamepad_axis(GamepadAxis id, int raw) {
	signed char *a   = gamepad.axis;
	signed char val  = AXISVAL(gamepad_axis_process_value_deadzone(raw));
	bool restricted = !config_get_int(CONFIG_GAMEPAD_AXIS_FREE);

	if(val * a[id] < 0) {
		// axis changed direction without passing the '0' state
		// this can be bad for digital input simulation (aka 'restricted mode')
		// so we insert a fake 0 event inbetween
		gamepad_axis(id, 0);
	}

	events_emit(TE_GAMEPAD_AXIS, id, (void*)(intptr_t)raw, NULL);

	if(!restricted) {
		int evt = gamepad_axis2gameevt(id);
		if(evt >= 0) {
			events_emit(evt, gamepad_axis_process_value(id, raw), NULL, NULL);
		}
	}

	if(val) {	// simulate press
		if(!a[id]) {
			a[id] = val;
			int key = gamepad_axis2gamekey(id, val);

			if(key >= 0) {
				if(restricted) {
					events_emit(TE_GAME_KEY_DOWN, key, (void*)(intptr_t)INDEV_GAMEPAD, NULL);
				}

				int evt = gamepad_axis2menuevt(id, val);
				if(evt >= 0) {
					events_emit(evt, 0, (void*)(intptr_t)INDEV_GAMEPAD, NULL);
				}
			}

			GamepadButton btn = gamepad_button_from_axis(id);

			if(btn != GAMEPAD_BUTTON_INVALID) {
				gamepad_button(btn, SDL_PRESSED);
			}
		}
	} else if(a[id]) {	// simulate release
		if(restricted) {
			int key = gamepad_axis2gamekey(id, a[id]);

			if(key >= 0) {
				events_emit(TE_GAME_KEY_UP, key, (void*)(intptr_t)INDEV_GAMEPAD, NULL);
			}
		}

		a[id] = AXISVAL_NULL;

		GamepadButton btn = gamepad_button_from_axis(id);

		if(btn != GAMEPAD_BUTTON_INVALID) {
			gamepad_button(btn, SDL_RELEASED);
		}
	}
}

static void gamepad_button(GamepadButton button, int state) {
	int gpkey	= config_gamepad_key_from_gamepad_button(button);
	int key		= config_key_from_gamepad_key(gpkey);
	void *indev =  (void*)(intptr_t)INDEV_GAMEPAD;

	if(state == SDL_PRESSED) {
		events_emit(TE_GAMEPAD_BUTTON_DOWN, button, indev, NULL);

		switch(button) {
			case GAMEPAD_BUTTON_START:
				events_emit(TE_MENU_ACCEPT, 0, indev, NULL);
				events_emit(TE_GAME_PAUSE, 0, indev, NULL);
				break;

			case GAMEPAD_BUTTON_BACK:
				events_emit(TE_MENU_ABORT, 0, indev, NULL);
				events_emit(TE_GAME_PAUSE, 0, indev, NULL);
				break;

			case GAMEPAD_BUTTON_DPAD_UP:    events_emit(TE_MENU_CURSOR_UP,    0, indev, NULL); break;
			case GAMEPAD_BUTTON_DPAD_DOWN:  events_emit(TE_MENU_CURSOR_DOWN,  0, indev, NULL); break;
			case GAMEPAD_BUTTON_DPAD_LEFT:  events_emit(TE_MENU_CURSOR_LEFT,  0, indev, NULL); break;
			case GAMEPAD_BUTTON_DPAD_RIGHT: events_emit(TE_MENU_CURSOR_RIGHT, 0, indev, NULL); break;

			case GAMEPAD_BUTTON_A:
				events_emit(TE_MENU_ACCEPT, 0, indev, NULL);
				break;

			case GAMEPAD_BUTTON_B:
				events_emit(TE_MENU_ABORT, 0, indev, NULL);
				break;

			default:
				break;
		}

		if(key >= 0) {
			events_emit(TE_GAME_KEY_DOWN, key, indev, NULL);
		}
	} else {
		events_emit(TE_GAMEPAD_BUTTON_UP, button, indev, NULL);

		if(key >= 0) {
			events_emit(TE_GAME_KEY_UP, key, indev, NULL);
		}
	}
}

static bool gamepad_event_handler(SDL_Event *event, void *arg) {
	assert(gamepad.initialized);

	switch(event->type) {
		case SDL_CONTROLLERAXISMOTION:
			if(event->caxis.which == gamepad.instance) {
				GamepadAxis axis = gamepad_axis_from_sdl_axis(event->caxis.axis);

				if(axis != GAMEPAD_AXIS_INVALID) {
					gamepad_axis(axis, event->caxis.value);
				}
			}
		return true;

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			if(event->cbutton.which == gamepad.instance) {
				GamepadButton btn = gamepad_button_from_sdl_button(event->cbutton.button);

				if(btn != GAMEPAD_BUTTON_INVALID) {
					gamepad_button(btn, event->cbutton.state);
				}
			}
		return true;
	}

	return false;
}

int gamepad_device_count(void) {
	return gamepad.devices.count;
}

void gamepad_device_guid(int num, char *guid_str, size_t guid_str_sz) {
	if(num < 0 || num >= gamepad.devices.count) {
		return;
	}

	SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(gamepad.devices.id_map[num]);
	SDL_JoystickGetGUIDString(guid, guid_str, guid_str_sz);
}

int gamepad_device_num_from_guid(const char *guid_str) {
	for(int i = 0; i < gamepad.devices.count; ++i) {
		char guid[33] = {0};
		gamepad_device_guid(i, guid, sizeof(guid));

		if(!strcasecmp(guid_str, guid)) {
			return i;
		}
	}

	return -1;
}

int gamepad_current_device_num(void) {
	if(gamepad.initialized) {
		return gamepad.current_devnum;
	}

	return -1;
}

bool gamepad_button_pressed(GamepadButton btn) {
	if(btn > GAMEPAD_BUTTON_INVALID && btn < GAMEPAD_BUTTON_MAX) {
		return SDL_GameControllerGetButton(gamepad.device, (SDL_GameControllerButton)btn);
	}

	if(btn & GAMEPAD_BUTTON_EMULATED) {
		GamepadEmulatedButton ebtn = btn & ~GAMEPAD_BUTTON_EMULATED;

		if(ebtn > GAMEPAD_EMULATED_BUTTON_INVALID && ebtn < GAMEPAD_EMULATED_BUTTON_MAX) {
			static GamepadAxis ebtn_to_axis[] = {
				[GAMEPAD_EMULATED_BUTTON_TRIGGER_LEFT] = GAMEPAD_AXIS_TRIGGER_LEFT,
				[GAMEPAD_EMULATED_BUTTON_TRIGGER_RIGHT] = GAMEPAD_AXIS_TRIGGER_RIGHT,
			};

			return gamepad.axis[ebtn_to_axis[ebtn]];
		}
	}

	return false;
}

bool gamepad_game_key_pressed(KeyIndex key) {
	if(!gamepad.initialized)
		return false;

	if(!config_get_int(CONFIG_GAMEPAD_AXIS_FREE)) {
		GamepadAxis axis = gamepad_gamekey2axis(key);

		if(axis != GAMEPAD_AXIS_INVALID && gamepad.axis[axis] == gamepad_gamekey2axisval(key)) {
			return true;
		}
	}

	int gpkey = config_gamepad_key_from_key(key);

	if(gpkey < 0) {
		return false;
	}

	int cfgidx = GPKEYIDX_TO_CFGIDX(gpkey);
	int button = config_get_int(cfgidx);

	return gamepad_button_pressed(button);
}

static const char *const gamepad_button_names[] = {
	[GAMEPAD_BUTTON_A] = "A",
	[GAMEPAD_BUTTON_B] = "B",
	[GAMEPAD_BUTTON_X] = "X",
	[GAMEPAD_BUTTON_Y] = "Y",
	[GAMEPAD_BUTTON_BACK] = "Back",
	[GAMEPAD_BUTTON_GUIDE] = "Guide",
	[GAMEPAD_BUTTON_START] = "Start",
	[GAMEPAD_BUTTON_STICK_LEFT] = "Left Stick",
	[GAMEPAD_BUTTON_STICK_RIGHT] = "Right Stick",
	[GAMEPAD_BUTTON_SHOULDER_LEFT] = "Left Bumper",
	[GAMEPAD_BUTTON_SHOULDER_RIGHT] = "Right Bumper",
	[GAMEPAD_BUTTON_DPAD_UP] = "Up",
	[GAMEPAD_BUTTON_DPAD_DOWN] = "Down",
	[GAMEPAD_BUTTON_DPAD_LEFT] = "Left",
	[GAMEPAD_BUTTON_DPAD_RIGHT] = "Right",
};

static const char *const gamepad_emulated_button_names[] = {
	[GAMEPAD_EMULATED_BUTTON_TRIGGER_LEFT] = "Left Trigger",
	[GAMEPAD_EMULATED_BUTTON_TRIGGER_RIGHT] = "Right Trigger",
};

static const char *const gamepad_axis_names[] = {
	[GAMEPAD_AXIS_LEFT_X] = "Left X",
	[GAMEPAD_AXIS_LEFT_Y] = "Left Y",
	[GAMEPAD_AXIS_RIGHT_X] = "Right X",
	[GAMEPAD_AXIS_RIGHT_Y] = "Right Y",
	[GAMEPAD_AXIS_TRIGGER_LEFT] = "Left Trigger",
	[GAMEPAD_AXIS_TRIGGER_RIGHT] = "Right Trigger",
};

const char* gamepad_button_name(GamepadButton btn) {

	if(btn > GAMEPAD_BUTTON_INVALID && btn < GAMEPAD_BUTTON_MAX) {
		return gamepad_button_names[btn];
	}

	if(btn & GAMEPAD_BUTTON_EMULATED) {
		return gamepad_emulated_button_names[btn & ~GAMEPAD_BUTTON_EMULATED];
	}

	return "Unknown";
}

const char* gamepad_axis_name(GamepadAxis axis) {

	if(axis > GAMEPAD_AXIS_INVALID && axis < GAMEPAD_AXIS_MAX) {
		return gamepad_axis_names[axis];
	}

	return "Unknown";
}

GamepadButton gamepad_button_from_name(const char *name) {
	for(int i = 0; i < GAMEPAD_BUTTON_MAX; ++i) {
		if(!strcasecmp(gamepad_button_names[i], name)) {
			return (GamepadButton)i;
		}
	}

	for(int i = 0; i < GAMEPAD_EMULATED_BUTTON_MAX; ++i) {
		if(!strcasecmp(gamepad_emulated_button_names[i], name)) {
			return (GamepadButton)(i | GAMEPAD_BUTTON_EMULATED);
		}
	}

	// for compatibility
	return (GamepadButton)SDL_GameControllerGetButtonFromString(name);
}

GamepadAxis gamepad_axis_from_name(const char *name) {
	for(int i = 0; i < GAMEPAD_AXIS_MAX; ++i) {
		if(!strcasecmp(gamepad_axis_names[i], name)) {
			return (GamepadAxis)i;
		}
	}

	// for compatibility
	return (GamepadAxis)SDL_GameControllerGetAxisFromString(name);
}

GamepadButton gamepad_button_from_sdl_button(SDL_GameControllerButton btn) {
	if(btn <= SDL_CONTROLLER_BUTTON_INVALID || btn >= SDL_CONTROLLER_BUTTON_MAX) {
		return GAMEPAD_BUTTON_INVALID;
	}

	return (GamepadButton)btn;
}

GamepadButton gamepad_button_from_axis(GamepadAxis axis) {
	switch(axis) {
		case GAMEPAD_AXIS_TRIGGER_LEFT:
			return GAMEPAD_BUTTON_TRIGGER_LEFT;

		case GAMEPAD_AXIS_TRIGGER_RIGHT:
			return GAMEPAD_BUTTON_TRIGGER_RIGHT;

		default:
			return GAMEPAD_BUTTON_INVALID;
	}
}

SDL_GameControllerButton gamepad_button_to_sdl_button(GamepadButton btn) {
	if(btn <= GAMEPAD_BUTTON_INVALID || btn >= GAMEPAD_BUTTON_MAX) {
		return SDL_CONTROLLER_BUTTON_INVALID;
	}

	return (SDL_GameControllerButton)btn;
}

GamepadAxis gamepad_axis_from_sdl_axis(SDL_GameControllerAxis axis) {
	if(axis <= SDL_CONTROLLER_AXIS_INVALID || axis >= SDL_CONTROLLER_AXIS_MAX) {
		return GAMEPAD_AXIS_INVALID;
	}

	return (GamepadAxis)axis;
}

SDL_GameControllerAxis gamepad_axis_to_sdl_axis(GamepadAxis axis) {
	if(axis <= GAMEPAD_AXIS_INVALID || axis >= GAMEPAD_AXIS_MAX) {
		return SDL_CONTROLLER_AXIS_INVALID;
	}

	return (SDL_GameControllerAxis)axis;
}
