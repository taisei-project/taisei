/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "gamepad.h"
#include "config.h"
#include "events.h"
#include "global.h"
#include "hirestime.h"

typedef struct GamepadAxisState {
	int16_t raw;
	int16_t analog;
	int8_t digital;
} GamepadAxisState;

typedef struct GamepadButtonState {
	bool held;
	hrtime_t repeat_time;
} GamepadButtonState;

static struct {
	bool initialized;
	int current_devnum;
	SDL_GameController *device;
	SDL_JoystickID instance;

	GamepadAxisState *axes;
	GamepadButtonState *buttons;

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


static bool gamepad_update_device_list(void) {
	int cnt = SDL_NumJoysticks();
	log_info("Updating gamepad devices list");

	free(gamepad.devices.id_map);
	memset(&gamepad.devices, 0, sizeof(gamepad.devices));

	if(!cnt) {
		log_info("No joysticks attached");
		return false;
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
		return false;
	}

	return true;
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

void gamepad_init(void) {
	if(!config_get_int(CONFIG_GAMEPAD_ENABLED) || gamepad.initialized) {
		return;
	}

	if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
		log_warn("SDL_InitSubSystem() failed: %s", SDL_GetError());
		return;
	}

	gamepad.initialized = true;
	gamepad_load_all_mappings();

	if(!gamepad_update_device_list()) {
		gamepad_shutdown();
		return;
	}

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
	gamepad.axes = calloc(GAMEPAD_AXIS_MAX, sizeof(GamepadAxisState));
	gamepad.buttons = calloc(GAMEPAD_BUTTON_MAX + GAMEPAD_EMULATED_BUTTON_MAX, sizeof(GamepadButtonState));

	log_info("Using device '%s' (#%i): %s", guid, dev, gamepad_device_name(dev));
	SDL_GameControllerEventState(SDL_ENABLE);

	config_set_str(CONFIG_GAMEPAD_DEVICE, guid);

	events_register_handler(&(EventHandler){
		.proc = gamepad_event_handler,
		.priority = EPRIO_TRANSLATION,
	});
}

void gamepad_shutdown(void) {
	if(!gamepad.initialized) {
		return;
	}

	log_info("Disabled the gamepad subsystem");

	if(gamepad.device) {
		SDL_GameControllerClose(gamepad.device);
	}

	free(gamepad.axes);
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

static int gamepad_axis2gameevt(GamepadAxis id) {
	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_LR)) {
		return TE_GAME_AXIS_LR;
	}

	if(id == config_get_int(CONFIG_GAMEPAD_AXIS_UD)) {
		return TE_GAME_AXIS_UD;
	}

	return -1;
}

static int get_axis_abs_limit(int val) {
	if(val < 0) {
		return -GAMEPAD_AXIS_MIN_VALUE;
	}

	return GAMEPAD_AXIS_MAX_VALUE;
}

static int gamepad_axis_process_value_deadzone(int raw) {
	int val, vsign;
	int limit = get_axis_abs_limit(raw);
	float deadzone = clamp(config_get_float(CONFIG_GAMEPAD_AXIS_DEADZONE), 0.01, 0.999);
	int minval = clamp(deadzone, 0, 1) * limit;

	val = raw;
	vsign = sign(val);
	val = abs(val);

	if(val < minval) {
		val = 0;
	} else {
		val = vsign * clamp((val - minval) / (1.0 - deadzone), 0, limit);
	}

	return val;
}

static int gamepad_axis_process_value(GamepadAxis id, int raw) {
	double sens = gamepad_axis_sens(id);
	int sens_sign = sign(sens);

	raw = gamepad_axis_process_value_deadzone(raw);

	int limit = get_axis_abs_limit(sens_sign * raw);
	double x = raw / (double)limit;
	int in_sign = sign(x);

	x = pow(fabs(x), 1.0 / fabs(sens)) * in_sign * sens_sign;
	x = x ? x : 0;
	x = clamp(x * limit, GAMEPAD_AXIS_MIN_VALUE, GAMEPAD_AXIS_MAX_VALUE);

	return (int)x;
}

int gamepad_axis_value(GamepadAxis id) {
	assert(id > GAMEPAD_AXIS_INVALID);
	assert(id < GAMEPAD_AXIS_MAX);

	return gamepad.axes[id].analog;
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

static void gamepad_button(GamepadButton button, int state, bool is_repeat);
static void gamepad_axis(GamepadAxis id, int raw);

double gamepad_normalize_axis_value(int val) {
	if(val < 0) {
		return -val / (double)GAMEPAD_AXIS_MIN_VALUE;
	} else if(val > 0) {
		return val / (double)GAMEPAD_AXIS_MAX_VALUE;
	} else {
		return 0;
	}
}

int gamepad_denormalize_axis_value(double val) {
	if(val < 0) {
		return -val * GAMEPAD_AXIS_MIN_VALUE;
	} else if(val > 0) {
		return val * GAMEPAD_AXIS_MAX_VALUE;
	} else {
		return 0;
	}
}

static void gamepad_update_game_axis(GamepadAxis axis, int oldval) {
	if(oldval != gamepad.axes[axis].analog) {
		int evt = gamepad_axis2gameevt(axis);

		if(evt >= 0) {
			events_emit(evt, gamepad.axes[axis].analog, NULL, NULL);
		}
	}
}

static void gamepad_restrict_player_axis_vals(GamepadAxis new_axis, int new_val) {
	typedef enum {
		UP = (1 << 0),
		DOWN = (1 << 1),
		RIGHT = (1 << 3),
		LEFT = (1 << 2),
	} MoveDir;

	GamepadAxis axis_x = config_get_int(CONFIG_GAMEPAD_AXIS_LR);
	GamepadAxis axis_y = config_get_int(CONFIG_GAMEPAD_AXIS_UD);

	int old_x = gamepad.axes[axis_x].analog;
	int old_y = gamepad.axes[axis_y].analog;

	gamepad.axes[new_axis].analog = new_val;

	assert(axis_x > GAMEPAD_AXIS_INVALID && axis_x < GAMEPAD_AXIS_MAX);
	assert(axis_y > GAMEPAD_AXIS_INVALID && axis_y < GAMEPAD_AXIS_MAX);

	double x = gamepad_normalize_axis_value(gamepad_player_axis_value(PLRAXIS_LR));
	double y = gamepad_normalize_axis_value(gamepad_player_axis_value(PLRAXIS_UD));

	MoveDir move = 0;

	if(x || y) {
		int d = (int)rint(atan2(-y, x) / (M_PI/4));

		switch(d) {
			case  0:         move =    0 | RIGHT; break;
			case -1:         move =   UP | RIGHT; break;
			case -2:         move =   UP | 0;     break;
			case -3:         move =   UP | LEFT;  break;
			case -4: case 4: move =    0 | LEFT;  break;
			case  3:         move = DOWN | LEFT;  break;
			case  2:         move = DOWN | 0;     break;
			case  1:         move = DOWN | RIGHT; break;
		}
	}

	gamepad.axes[axis_x].analog = (move & LEFT)  ? GAMEPAD_AXIS_MIN_VALUE :
	                              (move & RIGHT) ? GAMEPAD_AXIS_MAX_VALUE : 0;

	gamepad.axes[axis_y].analog = (move & DOWN)  ? GAMEPAD_AXIS_MIN_VALUE :
	                              (move & UP)    ? GAMEPAD_AXIS_MAX_VALUE : 0;

	gamepad_update_game_axis(axis_x, old_x);
	gamepad_update_game_axis(axis_y, old_y);
}

static void gamepad_axis(GamepadAxis id, int raw) {
	int8_t digital = AXISVAL(gamepad_axis_process_value_deadzone(raw));
	int16_t analog = gamepad_axis_process_value(id, raw);

	if(digital * gamepad.axes[id].digital < 0) {
		// axis changed direction without passing the '0' state
		// this can be bad for digital input simulation (aka 'restricted mode')
		// so we insert a fake 0 event inbetween
		gamepad_axis(id, 0);
	}

	events_emit(TE_GAMEPAD_AXIS, id, (void*)(intptr_t)raw, NULL);

	int old_analog = gamepad.axes[id].analog;
	gamepad.axes[id].raw = raw;

	if(config_get_int(CONFIG_GAMEPAD_AXIS_FREE)) {
		gamepad.axes[id].analog = analog;
		gamepad_update_game_axis(id, old_analog);
	} else if(gamepad_axis2gameevt(id) >= 0) {
		gamepad_restrict_player_axis_vals(id, analog);
	}

	if(digital != AXISVAL_NULL) {   // simulate press
		if(!gamepad.axes[id].digital) {
			gamepad.axes[id].digital = digital;

			GamepadButton btn = gamepad_button_from_axis(id, digital);

			if(btn != GAMEPAD_BUTTON_INVALID) {
				gamepad_button(btn, SDL_PRESSED, false);
			}
		}
	} else if(gamepad.axes[id].digital != AXISVAL_NULL) {  // simulate release
		GamepadButton btn = gamepad_button_from_axis(id, gamepad.axes[id].digital);
		gamepad.axes[id].digital = AXISVAL_NULL;

		if(btn != GAMEPAD_BUTTON_INVALID) {
			gamepad_button(btn, SDL_RELEASED, false);
		}
	}
}

static GamepadButtonState* gamepad_button_state(GamepadButton button) {
	if(button > GAMEPAD_BUTTON_INVALID && button < GAMEPAD_BUTTON_MAX) {
		return gamepad.buttons + button;
	}

	if(button & GAMEPAD_BUTTON_EMULATED) {
		GamepadEmulatedButton ebutton = button & ~GAMEPAD_BUTTON_EMULATED;

		if(ebutton > GAMEPAD_EMULATED_BUTTON_INVALID && ebutton < GAMEPAD_EMULATED_BUTTON_MAX) {
			return gamepad.buttons + GAMEPAD_BUTTON_MAX + ebutton;
		}
	}

	log_fatal("Button id %i is invalid", button);
}

static const char* gamepad_button_name_internal(GamepadButton btn);

static struct {
	GamepadButton button;
	KeyIndex game_key;
} gamepad_default_button_mappings[] = {
	{ GAMEPAD_BUTTON_BACK, KEY_SKIP },
};

#define NUM_DEFAULT_BUTTON_MAPPINGS (sizeof(gamepad_default_button_mappings)/sizeof(gamepad_default_button_mappings[0]))

static int gamepad_game_key_for_button(GamepadButton button) {
	int gpkey = config_gamepad_key_from_gamepad_button(button);
	int key = config_key_from_gamepad_key(gpkey);

	if(key >= 0) {
		return key;
	}

	for(int i = 0; i < NUM_DEFAULT_BUTTON_MAPPINGS; ++i) {
		if(gamepad_default_button_mappings[i].button == button) {
			return gamepad_default_button_mappings[i].game_key;
		}
	}

	return -1;
}

static void gamepad_button(GamepadButton button, int state, bool is_repeat) {
	int key = gamepad_game_key_for_button(button);
	void *indev = (void*)(intptr_t)INDEV_GAMEPAD;
	GamepadButtonState *btnstate = gamepad_button_state(button);

	if(state == SDL_PRESSED) {
		if(is_repeat) {
			btnstate->repeat_time = time_get() + config_get_float(CONFIG_GAMEPAD_BTNREPEAT_INTERVAL);
		} else {
			btnstate->repeat_time = time_get() + config_get_float(CONFIG_GAMEPAD_BTNREPEAT_DELAY);
		}

		btnstate->held = true;

		if(gamepad_button_name_internal(button) != NULL) {
			events_emit(TE_GAMEPAD_BUTTON_DOWN, button, indev, NULL);
		}

		if(!is_repeat || transition.state == TRANS_IDLE) {
			static struct eventmap_s {
				GamepadButton button;
				TaiseiEvent events[3];
			} eventmap[] = {
				{ GAMEPAD_BUTTON_START,               { TE_MENU_ACCEPT,       TE_GAME_PAUSE, TE_INVALID } },
				{ GAMEPAD_BUTTON_BACK,                { TE_MENU_ABORT,        TE_INVALID } },
				{ GAMEPAD_BUTTON_DPAD_UP,             { TE_MENU_CURSOR_UP,    TE_INVALID } },
				{ GAMEPAD_BUTTON_DPAD_DOWN,           { TE_MENU_CURSOR_DOWN,  TE_INVALID } },
				{ GAMEPAD_BUTTON_DPAD_LEFT,           { TE_MENU_CURSOR_LEFT,  TE_INVALID } },
				{ GAMEPAD_BUTTON_DPAD_RIGHT,          { TE_MENU_CURSOR_RIGHT, TE_INVALID } },
				{ GAMEPAD_BUTTON_ANALOG_STICK_UP,     { TE_MENU_CURSOR_UP,    TE_INVALID } },
				{ GAMEPAD_BUTTON_ANALOG_STICK_DOWN,   { TE_MENU_CURSOR_DOWN,  TE_INVALID } },
				{ GAMEPAD_BUTTON_ANALOG_STICK_LEFT,   { TE_MENU_CURSOR_LEFT,  TE_INVALID } },
				{ GAMEPAD_BUTTON_ANALOG_STICK_RIGHT,  { TE_MENU_CURSOR_RIGHT, TE_INVALID } },
				{ GAMEPAD_BUTTON_A,                   { TE_MENU_ACCEPT,       TE_INVALID } },
				{ GAMEPAD_BUTTON_B,                   { TE_MENU_ABORT,        TE_INVALID } },
			};

			for(int i = 0; i < sizeof(eventmap)/sizeof(eventmap[0]); ++i) {
				struct eventmap_s *e = eventmap + i;

				if(e->button == button) {
					for(int j = 0; j < sizeof(e->events)/sizeof(e->events[0]) && e->events[j] > TE_INVALID; ++j) {
						if(e->events[j] != TE_MENU_ACCEPT || !is_repeat) {
							events_emit(e->events[j], 0, indev, NULL);
						}
					}

					break;
				}
			}
		}

		if(key >= 0 && !is_repeat) {
			events_emit(TE_GAME_KEY_DOWN, key, indev, NULL);
		}
	} else {
		btnstate->held = false;
		events_emit(TE_GAMEPAD_BUTTON_UP, button, indev, NULL);

		if(key >= 0) {
			events_emit(TE_GAME_KEY_UP, key, indev, NULL);
		}
	}
}

static void gamepad_handle_button_repeat(GamepadButton btn, hrtime_t time) {
	GamepadButtonState *state = gamepad_button_state(btn);

	if(state->held && time >= state->repeat_time) {
		gamepad_button(btn, SDL_PRESSED, true);
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
					gamepad_button(btn, event->cbutton.state, false);
				}
			}
		return true;
	}

	if(event->type == MAKE_TAISEI_EVENT(TE_FRAME)) {
		hrtime_t time = time_get();

		for(GamepadButton btn = 0; btn < GAMEPAD_BUTTON_MAX; ++btn) {
			gamepad_handle_button_repeat(btn, time);
		}

		for(GamepadEmulatedButton btn = 0; btn < GAMEPAD_EMULATED_BUTTON_MAX; ++btn) {
			gamepad_handle_button_repeat(btn | GAMEPAD_BUTTON_EMULATED, time);
		}
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
	if(btn <= GAMEPAD_BUTTON_INVALID) {
		return false;
	}

	if(btn < GAMEPAD_BUTTON_MAX) {
		return gamepad.buttons[btn].held;
	}

	if(btn & GAMEPAD_BUTTON_EMULATED) {
		GamepadEmulatedButton ebtn = btn & ~GAMEPAD_BUTTON_EMULATED;

		if(ebtn > GAMEPAD_EMULATED_BUTTON_INVALID && ebtn < GAMEPAD_EMULATED_BUTTON_MAX) {
			return gamepad.buttons[GAMEPAD_BUTTON_MAX + ebtn].held;
		}
	}

	return false;
}

bool gamepad_game_key_pressed(KeyIndex key) {
	if(!gamepad.initialized) {
		return false;
	}

	bool pressed;
	int gpkey = config_gamepad_key_from_key(key);

	if(gpkey < 0) {
		pressed = false;
	} else {
		int cfgidx = GPKEYIDX_TO_CFGIDX(gpkey);
		int button = config_get_int(cfgidx);
		pressed = gamepad_button_pressed(button);
	}

	if(!pressed) {
		for(int i = 0; i < NUM_DEFAULT_BUTTON_MAPPINGS; ++i) {
			if(gamepad_default_button_mappings[i].game_key == key) {
				pressed = gamepad_button_pressed(gamepad_default_button_mappings[i].button);

				if(pressed) {
					return pressed;
				}
			}
		}
	}

	return pressed;
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
	[GAMEPAD_BUTTON_ANALOG_STICK_UP] = NULL,
	[GAMEPAD_BUTTON_ANALOG_STICK_DOWN] = NULL,
	[GAMEPAD_BUTTON_ANALOG_STICK_LEFT] = NULL,
	[GAMEPAD_BUTTON_ANALOG_STICK_RIGHT] = NULL,
};

static const char *const gamepad_axis_names[] = {
	[GAMEPAD_AXIS_LEFT_X] = "Left X",
	[GAMEPAD_AXIS_LEFT_Y] = "Left Y",
	[GAMEPAD_AXIS_RIGHT_X] = "Right X",
	[GAMEPAD_AXIS_RIGHT_Y] = "Right Y",
	[GAMEPAD_AXIS_TRIGGER_LEFT] = "Left Trigger",
	[GAMEPAD_AXIS_TRIGGER_RIGHT] = "Right Trigger",
};

static const char* gamepad_button_name_internal(GamepadButton btn) {
	if(btn > GAMEPAD_BUTTON_INVALID && btn < GAMEPAD_BUTTON_MAX) {
		return gamepad_button_names[btn];
	}

	if(btn & GAMEPAD_BUTTON_EMULATED) {
		return gamepad_emulated_button_names[btn & ~GAMEPAD_BUTTON_EMULATED];
	}

	return NULL;
}

const char* gamepad_button_name(GamepadButton btn) {
	const char *name = gamepad_button_name_internal(btn);

	if(name == NULL) {
		return "Unknown";
	}

	return name;
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
		if(gamepad_emulated_button_names[i] && !strcasecmp(gamepad_emulated_button_names[i], name)) {
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

GamepadButton gamepad_button_from_axis(GamepadAxis axis, GamepadAxisDigitalValue dval) {
	switch(axis) {
		case GAMEPAD_AXIS_TRIGGER_LEFT:
			return GAMEPAD_BUTTON_TRIGGER_LEFT;

		case GAMEPAD_AXIS_TRIGGER_RIGHT:
			return GAMEPAD_BUTTON_TRIGGER_RIGHT;

		default:
			break;
	}

	if(axis == GAMEPAD_AXIS_LEFT_Y || axis == GAMEPAD_AXIS_RIGHT_Y) {
		switch(dval) {
			case AXISVAL_UP:
				return GAMEPAD_BUTTON_ANALOG_STICK_UP;

			case AXISVAL_DOWN:
				return GAMEPAD_BUTTON_ANALOG_STICK_DOWN;

			default:
				return GAMEPAD_BUTTON_INVALID;
		}
	}

	if(axis == GAMEPAD_AXIS_LEFT_X || axis == GAMEPAD_AXIS_RIGHT_X) {
		switch(dval) {
			case AXISVAL_LEFT:
				return GAMEPAD_BUTTON_ANALOG_STICK_LEFT;

			case AXISVAL_RIGHT:
				return GAMEPAD_BUTTON_ANALOG_STICK_RIGHT;

			default:
				return GAMEPAD_BUTTON_INVALID;
		}
	}

	return GAMEPAD_BUTTON_INVALID;
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
