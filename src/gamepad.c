/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "gamepad.h"

#include "config.h"
#include "dynarray.h"
#include "events.h"
#include "hirestime.h"
#include "log.h"
#include "transition.h"
#include "util.h"
#include "util/miscmath.h"
#include "util/stringops.h"
#include "vfs/public.h"

typedef struct GamepadAxisState {
	int16_t raw;
	int8_t digital;
} GamepadAxisState;

typedef struct GamepadButtonState {
	bool held;
	hrtime_t repeat_time;
} GamepadButtonState;

typedef struct GamepadDevice {
	SDL_Gamepad *controller;
	SDL_JoystickID joy_instance;
} GamepadDevice;

static struct {
	GamepadAxisState *axes;
	GamepadButtonState *buttons;
	DYNAMIC_ARRAY(GamepadDevice) devices;
	int active_dev_num;
	bool initialized;
	bool update_needed;
} gamepad;

#define DEVNUM(dev) dynarray_indexof(&gamepad.devices, (dev))

#define MIN_DEADZONE (4.0 / (GAMEPAD_AXIS_MAX_VALUE))
#define MAX_DEADZONE (1 - MIN_DEADZONE)

static bool gamepad_event_handler(SDL_Event *event, void *arg);

static int gamepad_load_mappings(const char *vpath, int warn_noexist) {
	char *repr = vfs_repr(vpath, true);
	char *errstr = NULL;
	const char *const_errstr = NULL;

	SDL_IOStream *mappings = vfs_open(vpath, VFS_MODE_READ | VFS_MODE_SEEKABLE);
	int num_loaded = -1;
	LogLevel loglvl = LOG_WARN;

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

	if((num_loaded = SDL_AddGamepadMappingsFromIO(mappings, true)) < 0) {
		const_errstr = SDL_GetError();
	}

cleanup:
	if(const_errstr) {
		log_custom(loglvl, "Couldn't load mappings: %s", const_errstr);
	} else if(num_loaded >= 0) {
		log_info("Loaded %i mappings from '%s'", num_loaded, repr);
	}

	mem_free(repr);
	mem_free(errstr);
	return num_loaded;
}

static void gamepad_load_all_mappings(void) {
	gamepad_load_mappings("res/gamecontrollerdb.txt", true);
	gamepad_load_mappings("storage/gamecontrollerdb.txt", false);
}

static inline GamepadDevice* gamepad_get_device(int num) {
	if(num < 0 || num >= gamepad.devices.num_elements) {
		return NULL;
	}

	return dynarray_get_ptr(&gamepad.devices, num);
}

const char *gamepad_device_name(int num) {
	GamepadDevice *dev = gamepad_get_device(num);
	const char *const default_name = _("Unknown device");
	const char *name = NULL;

	if(!dev) {
		return default_name;
	}

	name = SDL_GetGamepadName(dev->controller);

	if(!name) {
		return default_name;
	}

	if(!strcasecmp(name, "Xinput Controller")) {
		// HACK: let's try to get a more descriptive name…
		const char *jname = SDL_GetJoystickNameForID(dev->joy_instance);

		if(jname != NULL) {
			name = jname;
		}
	}

	return name;
}

static bool gamepad_update_device_list(void) {
	log_info("Updating gamepad devices list");

	int num_joysticks;
	SDL_JoystickID *joysticks = SDL_GetJoysticks(&num_joysticks);

	if(!joysticks) {
		log_sdl_error(LOG_ERROR, "SDL_GetJoysticks");
		return false;
	}

	dynarray_foreach_elem(&gamepad.devices, auto dev, {
		SDL_CloseGamepad(dev->controller);
	});

	gamepad.devices.num_elements = 0;

	if(num_joysticks < 1) {
		dynarray_free_data(&gamepad.devices);
		log_info("No joysticks attached");
		SDL_free(joysticks);
		return false;
	}

	dynarray_ensure_capacity(&gamepad.devices, num_joysticks);

	for(int i = 0; i < num_joysticks; ++i) {
		SDL_GUID guid = SDL_GetJoystickGUIDForID(joysticks[i]);
		char guid_str[33] = { 0 };
		SDL_GUIDToString(guid, guid_str, sizeof(guid_str));

		if(!*guid_str) {
			log_warn("Failed to read GUID of joystick at index %i: %s", i, SDL_GetError());
			continue;
		}

		if(!	SDL_IsGamepad(joysticks[i])) {
			log_warn(
				"Joystick at index %i (name: \"%s\"; guid: %s) is not recognized as a game controller by SDL. "
				"Most likely it just doesn't have a mapping. See https://git.io/vdvdV for solutions",
				i, SDL_GetJoystickNameForID(joysticks[i]), guid_str
			);
			continue;
		}

		auto dev = dynarray_append(&gamepad.devices, {
			.joy_instance = joysticks[i],
			.controller = SDL_OpenGamepad(joysticks[i]),
		});

		if(dev->controller == NULL) {
			log_sdl_error(LOG_WARN, "SDL_OpenGamepad");
			--gamepad.devices.num_elements;
			continue;
		}

		int n = DEVNUM(dev);
		log_info("Found device '%s' (#%i): %s", guid_str, n, gamepad_device_name(n));
	}

	SDL_free(joysticks);

	if(!gamepad.devices.num_elements) {
		log_info("No usable devices");
		return false;
	}

	return true;
}

static GamepadDevice *gamepad_find_device_by_guid(const char *guid_str, char *guid_out, size_t guid_out_sz, int *out_localdevnum) {
	if(gamepad.devices.num_elements < 1) {
		*out_localdevnum = GAMEPAD_DEVNUM_INVALID;
		return NULL;
	}

	if(!strcasecmp(guid_str, "any")) {
		*out_localdevnum = GAMEPAD_DEVNUM_ANY;
		strlcpy(guid_out, "any", guid_out_sz);
		return NULL;
	}

	dynarray_foreach(&gamepad.devices, int i, auto dev, {
		*guid_out = 0;

		SDL_GUID guid = SDL_GetJoystickGUIDForID(dev->joy_instance);
		SDL_GUIDToString(guid, guid_out, guid_out_sz);

		if(!strcasecmp(guid_str, guid_out) || !strcasecmp(guid_str, "default")) {
			*out_localdevnum = i;
			return dev;
		}
	});

	*out_localdevnum = GAMEPAD_DEVNUM_INVALID;
	return NULL;
}

static GamepadDevice *gamepad_find_device(char *guid_out, size_t guid_out_sz, int *out_localdevnum) {
	GamepadDevice *dev;
	const char *want_guid = config_get_str(CONFIG_GAMEPAD_DEVICE);

	dev = gamepad_find_device_by_guid(want_guid, guid_out, guid_out_sz, out_localdevnum);

	if(dev || *out_localdevnum == GAMEPAD_DEVNUM_ANY) {
		return dev;
	}

	if(strcasecmp(want_guid, "default")) {
		log_warn("Requested device '%s' is not available", want_guid);
		return gamepad_find_device_by_guid("any", guid_out, guid_out_sz, out_localdevnum);
	}

	*out_localdevnum = GAMEPAD_DEVNUM_INVALID;
	strcpy(guid_out, want_guid);

	return NULL;
}

int gamepad_get_active_device(void) {
	if(gamepad.initialized) {
		return gamepad.active_dev_num;
	}

	return GAMEPAD_DEVNUM_INVALID;
}

bool gamepad_update_devices(void) {
	gamepad.update_needed = false;

	if(!gamepad_update_device_list()) {
		return false;
	}

	char guid[33];
	GamepadDevice *dev = gamepad_find_device(guid, sizeof(guid), &gamepad.active_dev_num);

	if(gamepad.active_dev_num == GAMEPAD_DEVNUM_ANY) {
		log_info("Using all available devices (%u)", gamepad.devices.num_elements);
		return true;
	}

	if(dev == NULL) {
		log_info("No devices available");
		return false;
	}

	log_info("Using device '%s' (#%i): %s", guid, gamepad.active_dev_num, gamepad_device_name(gamepad.active_dev_num));
	config_set_str(CONFIG_GAMEPAD_DEVICE, guid);
	return true;
}

static inline void set_events_state(bool state) {
	SDL_SetJoystickEventsEnabled(state);
	SDL_SetGamepadEventsEnabled(state);
}

void gamepad_init(void) {
	if(gamepad.initialized) {
		return;
	}

	set_events_state(false);

	if(!config_get_int(CONFIG_GAMEPAD_ENABLED)) {
		return;
	}

	if(!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		log_sdl_error(LOG_ERROR, "SDL_InitSubSystem");
		return;
	}

	gamepad.initialized = true;
	gamepad.update_needed = true;
	gamepad.axes = ALLOC_ARRAY(GAMEPAD_AXIS_MAX, typeof(*gamepad.axes));
	gamepad.buttons = ALLOC_ARRAY(GAMEPAD_BUTTON_MAX + GAMEPAD_EMULATED_BUTTON_MAX, typeof(*gamepad.buttons));
	gamepad.active_dev_num = GAMEPAD_DEVNUM_INVALID;
	gamepad_load_all_mappings();

	events_register_handler(&(EventHandler){
		.proc = gamepad_event_handler,
		.priority = EPRIO_TRANSLATION,
	});

	set_events_state(true);
}

void gamepad_shutdown(void) {
	if(!gamepad.initialized) {
		return;
	}

	log_info("Disabled the gamepad subsystem");

	mem_free(gamepad.axes);
	mem_free(gamepad.buttons);

	dynarray_foreach_elem(&gamepad.devices, auto dev, {
		if(dev->controller) {
			SDL_CloseGamepad(dev->controller);
		}
	});

	dynarray_free_data(&gamepad.devices);

	set_events_state(false);
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);

	memset(&gamepad, 0, sizeof(gamepad));
	events_unregister_handler(gamepad_event_handler);
}

bool gamepad_initialized(void) {
	return gamepad.initialized;
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

double gamepad_get_normalized_deadzone(void) {
	return clamp(config_get_float(CONFIG_GAMEPAD_AXIS_DEADZONE), MIN_DEADZONE, MAX_DEADZONE);
}

double gamepad_get_normalized_maxzone(void) {
	return clamp(config_get_float(CONFIG_GAMEPAD_AXIS_MAXZONE), 0, 1);
}

int gamepad_axis_value(GamepadAxis id) {
	assert(id > GAMEPAD_AXIS_INVALID);
	assert(id < GAMEPAD_AXIS_MAX);

	return gamepad.axes[id].raw;
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

static void gamepad_button(GamepadButton button, bool pressed, bool is_repeat);
static void gamepad_axis(GamepadAxis id, int raw);

double gamepad_normalize_axis_value(int val) {
	if(val < 0) {
		return val / (double)-GAMEPAD_AXIS_MIN_VALUE;
	} else if(val > 0) {
		return val / (double)GAMEPAD_AXIS_MAX_VALUE;
	} else {
		return 0;
	}
}

int gamepad_denormalize_axis_value(double val) {
	if(val < 0) {
		return max(GAMEPAD_AXIS_MIN_VALUE, (int)(val * -GAMEPAD_AXIS_MIN_VALUE));
	} else if(val > 0) {
		return min(GAMEPAD_AXIS_MAX_VALUE, (int)(val * GAMEPAD_AXIS_MAX_VALUE));
	} else {
		return 0;
	}
}

static void gamepad_update_game_axis(GamepadAxis axis, int oldval) {
	if(oldval != gamepad.axes[axis].raw) {
		int evt = gamepad_axis2gameevt(axis);

		if(evt >= 0) {
			events_emit(evt, gamepad.axes[axis].raw, NULL, NULL);
		}
	}
}

static double gamepad_apply_sensitivity(double p, double sens) {
	if(sens == 0) {
		return p;
	}

	double t = exp2(sens);
	double a = 1 - pow(1 - fabs(p), t);

	if(isnan(a)) {
		return p;
	}

	return copysign(pow(a, 1 / t), p);
}

static cmplx square_to_circle(cmplx z) {
	double u = re(z) * sqrt(1.0 - im(z) * im(z) / 2.0);
	double v = im(z) * sqrt(1.0 - re(z) * re(z) / 2.0);

	return CMPLX(u, v);
}

static cmplx gamepad_snap_analog_direction(cmplx z) {
	double snap = clamp(config_get_float(CONFIG_GAMEPAD_AXIS_SNAP), 0, 1);

	if(snap == 0) {
		return z;
	}

	double diagonal_bias = clamp(config_get_float(CONFIG_GAMEPAD_AXIS_SNAP_DIAG_BIAS), -1, 1);
	double w_cardinal = 1 - diagonal_bias;
	double w_diagonal = 1 + diagonal_bias;

	#define D 0.7071067811865475
	struct { cmplx v; double weight; } directions[] = {
		{ CMPLX( 1,  0), w_cardinal },
		{ CMPLX( 0,  1), w_cardinal },
		{ CMPLX(-1,  0), w_cardinal },
		{ CMPLX( 0, -1), w_cardinal },
		{ CMPLX( D,  D), w_diagonal },
		{ CMPLX( D, -D), w_diagonal },
		{ CMPLX(-D, -D), w_diagonal },
		{ CMPLX(-D,  D), w_diagonal },
	};
	#undef D

	double m = cabs(z);
	double thres = snap * M_PI/ARRAY_SIZE(directions);

	for(int i = 0; i < ARRAY_SIZE(directions); ++i) {
		cmplx q = directions[i].v;
		double delta_angle = acos(cdot(q, z) / m);

		if(delta_angle < thres * directions[i].weight) {
			z = q * m;
			break;
		}

	}

	return z;
}

void gamepad_get_player_analog_input(int *xaxis, int *yaxis) {
	int xraw = gamepad_player_axis_value(PLRAXIS_LR);
	int yraw = gamepad_player_axis_value(PLRAXIS_UD);

	*xaxis = 0;
	*yaxis = 0;

	double deadzone = gamepad_get_normalized_deadzone();
	double maxzone = gamepad_get_normalized_maxzone();

	cmplx z = CMPLX(
		gamepad_normalize_axis_value(xraw),
		gamepad_normalize_axis_value(yraw)
	);

	if(config_get_int(CONFIG_GAMEPAD_AXIS_SQUARECIRCLE)) {
		z = square_to_circle(z);
	}

	double raw_abs2 = cabs2(z);

	if(raw_abs2 < deadzone * deadzone) {
		return;
	}

	double raw_abs = sqrt(raw_abs2);
	assert(raw_abs > 0);

	if(deadzone < maxzone) {
		double new_abs;
		if(raw_abs >= maxzone) {
			new_abs = max(raw_abs, 1);
		} else {
			new_abs = (min(raw_abs, maxzone) - deadzone) / (maxzone - deadzone);
			new_abs = gamepad_apply_sensitivity(new_abs, config_get_float(CONFIG_GAMEPAD_AXIS_SENS));
		}
		z *= new_abs / raw_abs;
	} else {
		z /= raw_abs;
	}

	z = CMPLX(
		gamepad_apply_sensitivity(re(z), config_get_float(CONFIG_GAMEPAD_AXIS_LR_SENS)),
		gamepad_apply_sensitivity(im(z), config_get_float(CONFIG_GAMEPAD_AXIS_UD_SENS))
	);

	z = gamepad_snap_analog_direction(z);

	*xaxis = gamepad_denormalize_axis_value(re(z));
	*yaxis = gamepad_denormalize_axis_value(im(z));
}

static int gamepad_axis_get_digital_value(int raw) {
	double deadzone = gamepad_get_normalized_deadzone();
	deadzone = max(deadzone, 0.5);
	int threshold = GAMEPAD_AXIS_MAX_VALUE * deadzone;

	if(abs(raw) < threshold) {
		return 0;
	}

	return raw < 0 ? -1 : 1;
}

static void gamepad_axis(GamepadAxis id, int raw) {
	int8_t digital = gamepad_axis_get_digital_value(raw);

	if(digital * gamepad.axes[id].digital < 0) {
		// axis changed direction without passing the '0' state
		// this can be bad for digital input simulation (aka 'restricted mode')
		// so we insert a fake 0 event in between
		gamepad_axis(id, 0);
	}

	events_emit(TE_GAMEPAD_AXIS, id, (void*)(intptr_t)raw, NULL);

	int old_raw = gamepad.axes[id].raw;
	gamepad.axes[id].raw = raw;
	gamepad_update_game_axis(id, old_raw);

	if(digital != AXISVAL_NULL) {   // simulate press
		if(!gamepad.axes[id].digital) {
			gamepad.axes[id].digital = digital;
			events_emit(TE_GAMEPAD_AXIS_DIGITAL, id, (void*)(intptr_t)digital, NULL);

			GamepadButton btn = gamepad_button_from_axis(id, digital);

			if(btn != GAMEPAD_BUTTON_INVALID) {
				gamepad_button(btn, true, false);
			}
		}
	} else if(gamepad.axes[id].digital != AXISVAL_NULL) {  // simulate release
		GamepadButton btn = gamepad_button_from_axis(id, gamepad.axes[id].digital);
		gamepad.axes[id].digital = AXISVAL_NULL;
		events_emit(TE_GAMEPAD_AXIS_DIGITAL, id, (void*)(intptr_t)digital, NULL);

		if(btn != GAMEPAD_BUTTON_INVALID) {
			gamepad_button(btn, false, false);
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

	log_warn("Unknown button id %i ignored", button);
	return NULL;
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

static inline hrtime_t gamepad_repeat_interval(ConfigIndex opt) {
	return HRTIME_RESOLUTION * (double)config_get_float(opt);
}

static void gamepad_button(GamepadButton button, bool pressed, bool is_repeat) {
	int key = gamepad_game_key_for_button(button);
	void *indev = (void*)(intptr_t)INDEV_GAMEPAD;
	GamepadButtonState *btnstate = gamepad_button_state(button);

	if(UNLIKELY(btnstate == NULL)) {
		return;
	}

	if(pressed) {
		if(is_repeat) {
			btnstate->repeat_time = time_get() + gamepad_repeat_interval(CONFIG_GAMEPAD_BTNREPEAT_INTERVAL);
		} else {
			btnstate->repeat_time = time_get() + gamepad_repeat_interval(CONFIG_GAMEPAD_BTNREPEAT_DELAY);
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

	if(UNLIKELY(state == NULL)) {
		return;
	}

	if(state->held && time >= state->repeat_time) {
		gamepad_button(btn, true, true);
	}
}

#define INSTANCE_IS_VALID(inst) \
	( \
		gamepad.active_dev_num == GAMEPAD_DEVNUM_ANY || \
		( \
			gamepad.active_dev_num >= 0 && \
			(inst) == dynarray_get(&gamepad.devices, gamepad.active_dev_num).joy_instance \
		) \
	)

static bool gamepad_event_handler(SDL_Event *event, void *arg) {
	assert(gamepad.initialized);

	switch(event->type) {
		case SDL_EVENT_GAMEPAD_AXIS_MOTION :
			if(INSTANCE_IS_VALID(event->gaxis.which)) {
				GamepadAxis axis = gamepad_axis_from_sdl_axis(event->gaxis.axis);

				if(axis != GAMEPAD_AXIS_INVALID) {
					gamepad_axis(axis, event->gaxis.value);
				}
			}
		return true;

		case SDL_EVENT_GAMEPAD_BUTTON_DOWN :
		case SDL_EVENT_GAMEPAD_BUTTON_UP :
			if(INSTANCE_IS_VALID(event->gbutton.which)) {
				GamepadButton btn = gamepad_button_from_sdl_button(event->gbutton.button);

				if(btn != GAMEPAD_BUTTON_INVALID) {
					gamepad_button(btn, event->gbutton.down, false);
				}
			}
		return true;

		case SDL_EVENT_GAMEPAD_ADDED :
		case SDL_EVENT_GAMEPAD_REMOVED :
		case SDL_EVENT_GAMEPAD_REMAPPED :
			gamepad.update_needed = true;
		return true;
	}

	if(event->type == MAKE_TAISEI_EVENT(TE_FRAME)) {
		if(gamepad.update_needed) {
			gamepad_update_devices();
		}

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
	return gamepad.devices.num_elements;
}

void gamepad_device_guid(int num, char *guid_str, size_t guid_str_sz) {
	if(num == GAMEPAD_DEVNUM_ANY) {
		strlcpy(guid_str, "any", guid_str_sz);
		return;
	}

	GamepadDevice *dev = gamepad_get_device(num);

	if(dev) {
		SDL_GUID guid = SDL_GetJoystickGUIDForID(dev->joy_instance);
		SDL_GUIDToString(guid, guid_str, guid_str_sz);
	}
}

int gamepad_device_num_from_guid(const char *guid_str) {
	if(strcasecmp(guid_str, "any")) {
		return GAMEPAD_DEVNUM_ANY;
	}

	dynarray_foreach_idx(&gamepad.devices, int i, {
		char guid[33] = {0};
		gamepad_device_guid(i, guid, sizeof(guid));

		if(!strcasecmp(guid_str, guid)) {
			return i;
		}
	});

	return GAMEPAD_DEVNUM_INVALID;
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
	[GAMEPAD_BUTTON_A] = N_("A"),
	[GAMEPAD_BUTTON_B] = N_("B"),
	[GAMEPAD_BUTTON_X] = N_("X"),
	[GAMEPAD_BUTTON_Y] = N_("Y"),
	[GAMEPAD_BUTTON_BACK] = N_("Back"),
	[GAMEPAD_BUTTON_GUIDE] = N_("Guide"),
	[GAMEPAD_BUTTON_START] = N_("Start"),
	[GAMEPAD_BUTTON_STICK_LEFT] = N_("Left Stick"),
	[GAMEPAD_BUTTON_STICK_RIGHT] = N_("Right Stick"),
	[GAMEPAD_BUTTON_SHOULDER_LEFT] = N_("Left Bumper"),
	[GAMEPAD_BUTTON_SHOULDER_RIGHT] = N_("Right Bumper"),
	[GAMEPAD_BUTTON_DPAD_UP] = N_("Up"),
	[GAMEPAD_BUTTON_DPAD_DOWN] = N_("Down"),
	[GAMEPAD_BUTTON_DPAD_LEFT] = N_("Left"),
	[GAMEPAD_BUTTON_DPAD_RIGHT] = N_("Right"),
	[GAMEPAD_BUTTON_MISC1] = N_("Misc"),
	[GAMEPAD_BUTTON_P1] = N_("P1"),
	[GAMEPAD_BUTTON_P2] = N_("P2"),
	[GAMEPAD_BUTTON_P3] = N_("P3"),
	[GAMEPAD_BUTTON_P4] = N_("P4"),
	[GAMEPAD_BUTTON_TOUCHPAD] = N_("Touchpad"),
	[GAMEPAD_BUTTON_MISC2] = N_("Misc 2"),
	[GAMEPAD_BUTTON_MISC3] = N_("Misc 3"),
	[GAMEPAD_BUTTON_MISC4] = N_("Misc 4"),
	[GAMEPAD_BUTTON_MISC5] = N_("Misc 5"),
	[GAMEPAD_BUTTON_MISC6] = N_("Misc 6"),
};

static_assert(ARRAY_SIZE(gamepad_button_names) == GAMEPAD_BUTTON_MAX);

static const char *const gamepad_emulated_button_names[] = {
	[GAMEPAD_EMULATED_BUTTON_TRIGGER_LEFT] = N_("Left Trigger"),
	[GAMEPAD_EMULATED_BUTTON_TRIGGER_RIGHT] = N_("Right Trigger"),
	[GAMEPAD_EMULATED_BUTTON_ANALOG_STICK_UP] = N_("Stick Up"),
	[GAMEPAD_EMULATED_BUTTON_ANALOG_STICK_DOWN] = N_("Stick Down"),
	[GAMEPAD_EMULATED_BUTTON_ANALOG_STICK_LEFT] = N_("Stick Left"),
	[GAMEPAD_EMULATED_BUTTON_ANALOG_STICK_RIGHT] = N_("Stick Right"),
};

static_assert(ARRAY_SIZE(gamepad_emulated_button_names) == GAMEPAD_EMULATED_BUTTON_MAX);

static const char *const gamepad_axis_names[GAMEPAD_AXIS_MAX] = {
	[GAMEPAD_AXIS_LEFT_X] = N_("Left X"),
	[GAMEPAD_AXIS_LEFT_Y] = N_("Left Y"),
	[GAMEPAD_AXIS_RIGHT_X] = N_("Right X"),
	[GAMEPAD_AXIS_RIGHT_Y] = N_("Right Y"),
	[GAMEPAD_AXIS_TRIGGER_LEFT] = N_("Left Trigger"),
	[GAMEPAD_AXIS_TRIGGER_RIGHT] = N_("Right Trigger"),
};

static_assert(ARRAY_SIZE(gamepad_axis_names) == GAMEPAD_AXIS_MAX);

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
		return N_("Unknown");
	}

	return name;
}

const char* gamepad_axis_name(GamepadAxis axis) {
	if(axis > GAMEPAD_AXIS_INVALID && axis < GAMEPAD_AXIS_MAX) {
		return gamepad_axis_names[axis];
	}

	return N_("Unknown");
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
	return (GamepadButton) SDL_GetGamepadButtonFromString(name);
}

GamepadAxis gamepad_axis_from_name(const char *name) {
	for(int i = 0; i < GAMEPAD_AXIS_MAX; ++i) {
		if(!strcasecmp(gamepad_axis_names[i], name)) {
			return (GamepadAxis)i;
		}
	}

	// for compatibility
	return (GamepadAxis) SDL_GetGamepadAxisFromString(name);
}

GamepadButton gamepad_button_from_sdl_button(SDL_GamepadButton btn) {
	if(btn <= SDL_GAMEPAD_BUTTON_INVALID || btn >= SDL_GAMEPAD_BUTTON_COUNT) {
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

SDL_GamepadButton gamepad_button_to_sdl_button(GamepadButton btn) {
	if(btn <= GAMEPAD_BUTTON_INVALID || btn >= GAMEPAD_BUTTON_MAX) {
		return SDL_GAMEPAD_BUTTON_INVALID;
	}

	return (SDL_GamepadButton)btn;
}

GamepadAxis gamepad_axis_from_sdl_axis(SDL_GamepadAxis axis) {
	if(axis <= SDL_GAMEPAD_AXIS_INVALID || axis >= SDL_GAMEPAD_AXIS_COUNT) {
		return GAMEPAD_AXIS_INVALID;
	}

	return (GamepadAxis)axis;
}

SDL_GamepadAxis gamepad_axis_to_sdl_axis(GamepadAxis axis) {
	if(axis <= GAMEPAD_AXIS_INVALID || axis >= GAMEPAD_AXIS_MAX) {
		return SDL_GAMEPAD_AXIS_INVALID;
	}

	return (SDL_GamepadAxis)axis;
}
