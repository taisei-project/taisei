/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <SDL_keycode.h>
#include <stdbool.h>

/*
	Define these macros, then use CONFIGDEFS to expand them all for all config entries, or KEYDEFS for just keybindings.
	Don't forget to undef them afterwards.
	Remember that the id won't have the CONFIG_ prefix, prepend it yourself if you want a ConfigIndex constant.
*/

// #define CONFIGDEF_KEYBINDING(id,entryname,default)
// #define CONFIGDEF_GPKEYBINDING(id,entryname,default)
// #define CONFIGDEF_INT(id,entryname,default)
// #define CONFIGDEF_FLOAT(id,entryname,default)
// #define CONFIGDEF_STRING(id,entryname,default)

/*
 *	DO NOT REORDER KEYBINDINGS.
 *
 *	The KEY_* constants are used by replays. These are formed from KEYDEFS.
 *	Always add new keys at the end of KEYDEFS, never put them directly into CONFIGDEFS.
 *
 *	KEYDEFS can be safely moved around within CONFIGDEFS, however.
 *	Stuff in GPKEYDEFS is safe to reorder.
 */

#define KEYDEFS \
	CONFIGDEF_KEYBINDING(KEY_UP,				"key_up",				SDL_SCANCODE_UP) \
	CONFIGDEF_KEYBINDING(KEY_DOWN,				"key_down",				SDL_SCANCODE_DOWN) \
	CONFIGDEF_KEYBINDING(KEY_LEFT,				"key_left",				SDL_SCANCODE_LEFT) \
	CONFIGDEF_KEYBINDING(KEY_RIGHT,				"key_right",			SDL_SCANCODE_RIGHT) \
	CONFIGDEF_KEYBINDING(KEY_FOCUS,				"key_focus",			SDL_SCANCODE_LSHIFT) \
	CONFIGDEF_KEYBINDING(KEY_SHOT,				"key_shot",				SDL_SCANCODE_Z) \
	CONFIGDEF_KEYBINDING(KEY_BOMB,				"key_bomb",				SDL_SCANCODE_X) \
	CONFIGDEF_KEYBINDING(KEY_FULLSCREEN,		"key_fullscreen",		SDL_SCANCODE_F11) \
	CONFIGDEF_KEYBINDING(KEY_SCREENSHOT,		"key_screenshot",		SDL_SCANCODE_P) \
	CONFIGDEF_KEYBINDING(KEY_SKIP,				"key_skip",				SDL_SCANCODE_LCTRL) \
	CONFIGDEF_KEYBINDING(KEY_IDDQD,				"key_iddqd",			SDL_SCANCODE_Q) \
	CONFIGDEF_KEYBINDING(KEY_HAHAIWIN,			"key_skipstage",		SDL_SCANCODE_E) \
	CONFIGDEF_KEYBINDING(KEY_PAUSE,				"key_pause",			SDL_SCANCODE_PAUSE) \


#define GPKEYDEFS \
	CONFIGDEF_GPKEYBINDING(KEY_UP, 				"gamepad_key_up", 		-1) \
	CONFIGDEF_GPKEYBINDING(KEY_DOWN, 			"gamepad_key_down", 	-1) \
	CONFIGDEF_GPKEYBINDING(KEY_LEFT, 			"gamepad_key_left", 	-1) \
	CONFIGDEF_GPKEYBINDING(KEY_RIGHT, 			"gamepad_key_right", 	-1) \
	CONFIGDEF_GPKEYBINDING(KEY_FOCUS, 			"gamepad_key_focus", 	0) \
	CONFIGDEF_GPKEYBINDING(KEY_SHOT, 			"gamepad_key_shot", 	1) \
	CONFIGDEF_GPKEYBINDING(KEY_BOMB, 			"gamepad_key_bomb", 	2) \
	CONFIGDEF_GPKEYBINDING(KEY_SKIP, 			"gamepad_key_skip", 	3) \
	CONFIGDEF_GPKEYBINDING(KEY_PAUSE, 			"gamepad_key_pause", 	4) \


#define CONFIGDEFS \
	CONFIGDEF_STRING	(PLAYERNAME, 				"playername", 							"Player") \
	CONFIGDEF_INT		(FULLSCREEN, 				"fullscreen", 							0) \
	CONFIGDEF_INT		(VID_WIDTH, 				"vid_width", 							RESX) \
	CONFIGDEF_INT		(VID_HEIGHT, 				"vid_height", 							RESY) \
	CONFIGDEF_INT		(VSYNC, 					"vsync", 								1) \
	CONFIGDEF_INT		(NO_SHADER, 				"disable_shader", 						0) \
	CONFIGDEF_INT		(NO_AUDIO,					"disable_audio", 						0) \
	CONFIGDEF_INT		(NO_MUSIC,					"disable_bgm", 							0) \
	CONFIGDEF_INT		(MIXER_CHUNKSIZE,			"mixer_chunksize",						1024) \
	CONFIGDEF_FLOAT		(SFX_VOLUME, 				"sfx_volume", 							1.0) \
	CONFIGDEF_FLOAT		(BGM_VOLUME, 				"bgm_volume", 							1.0) \
	CONFIGDEF_INT		(NO_STAGEBG, 				"disable_stagebg", 						0) \
	CONFIGDEF_INT		(NO_STAGEBG_FPSLIMIT, 		"disable_stagebg_auto_fpslimit", 		40) \
	CONFIGDEF_INT		(SAVE_RPY, 					"save_rpy", 							2) \
	KEYDEFS \
	CONFIGDEF_INT		(GAMEPAD_ENABLED, 			"gamepad_enabled", 						0) \
	CONFIGDEF_INT		(GAMEPAD_DEVICE, 			"gamepad_device", 						0) \
	CONFIGDEF_INT		(GAMEPAD_AXIS_UD, 			"gamepad_axis_ud", 						1) \
	CONFIGDEF_INT		(GAMEPAD_AXIS_LR, 			"gamepad_axis_lr", 						0) \
	CONFIGDEF_INT		(GAMEPAD_AXIS_FREE, 		"gamepad_axis_free", 					1) \
	CONFIGDEF_FLOAT		(GAMEPAD_AXIS_UD_SENS, 		"gamepad_axis_ud_free_sensitivity", 	1.0) \
	CONFIGDEF_FLOAT		(GAMEPAD_AXIS_LR_SENS, 		"gamepad_axis_lr_free_sensitivity", 	1.0) \
	CONFIGDEF_FLOAT		(GAMEPAD_AXIS_DEADZONE, 	"gamepad_axis_deadzone", 				0.0) \
	GPKEYDEFS \


typedef enum ConfigIndex {
	#define CONFIGDEF(id) CONFIG_##id,
	#define CONFIGDEF_KEYBINDING(id,entryname,default) CONFIGDEF(id)
	#define CONFIGDEF_GPKEYBINDING(id,entryname,default) CONFIGDEF(GAMEPAD_##id)
	#define CONFIGDEF_INT(id,entryname,default) CONFIGDEF(id)
	#define CONFIGDEF_FLOAT(id,entryname,default) CONFIGDEF(id)
	#define CONFIGDEF_STRING(id,entryname,default) CONFIGDEF(id)

	CONFIGDEFS
	CONFIGIDX_NUM

	#undef CONFIGDEF
	#undef CONFIGDEF_KEYBINDING
	#undef CONFIGDEF_GPKEYBINDING
	#undef CONFIGDEF_INT
	#undef CONFIGDEF_FLOAT
	#undef CONFIGDEF_STRING
} ConfigIndex;

#define CONFIGIDX_FIRST 0
#define CONFIGIDX_LAST (CONFIGIDX_NUM - 1)

typedef enum KeyIndex {
	#define CONFIGDEF_KEYBINDING(id,entryname,default) id,

	KEYDEFS
	KEYIDX_NUM

	#undef CONFIGDEF_KEYBINDING
} KeyIndex;

#define KEYIDX_FIRST 0
#define KEYIDX_LAST (KEYIDX_NUM - 1)
#define CFGIDX_TO_KEYIDX(idx) (idx - CONFIG_KEY_FIRST + KEYIDX_FIRST)
#define KEYIDX_TO_CFGIDX(idx) (idx + CONFIG_KEY_FIRST - KEYIDX_FIRST)
#define CONFIG_KEY_FIRST CONFIG_KEY_UP
#define CONFIG_KEY_LAST (CONFIG_KEY_FIRST + KEYIDX_LAST - KEYIDX_FIRST)

typedef enum GamepadKeyIndex {
	#define CONFIGDEF_GPKEYBINDING(id,entryname,default) GAMEPAD_##id,

	GPKEYDEFS
	GAMEPAD_KEYIDX_NUM

	#undef CONFIGDEF_GPKEYBINDING
} GamepadKeyIndex;

#define GAMEPAD_KEYIDX_FIRST 0
#define GAMEPAD_KEYIDX_LAST (GAMEPAD_KEYIDX_NUM - 1)
#define CFGIDX_TO_GPKEYIDX(idx) (idx - CONFIG_GAMEPAD_KEY_FIRST + GAMEPAD_KEYIDX_FIRST)
#define GPKEYIDX_TO_CFGIDX(idx) (idx + CONFIG_GAMEPAD_KEY_FIRST - GAMEPAD_KEYIDX_FIRST)
#define CONFIG_GAMEPAD_KEY_FIRST CONFIG_GAMEPAD_KEY_UP
#define CONFIG_GAMEPAD_KEY_LAST (CONFIG_GAMEPAD_KEY_FIRST + GAMEPAD_KEYIDX_LAST - GAMEPAD_KEYIDX_FIRST)

typedef enum ConfigEntryType {
	CONFIG_TYPE_INT,
	CONFIG_TYPE_STRING,
	CONFIG_TYPE_KEYBINDING,
	CONFIG_TYPE_FLOAT
} ConfigEntryType;

typedef union ConfigValue {
	int i;
	double f;
	char *s;
} ConfigValue;

typedef void (*ConfigCallback)(ConfigIndex, ConfigValue);

typedef struct ConfigEntry {
	ConfigEntryType type;
	char *name;
	ConfigValue val;
	ConfigCallback callback;
} ConfigEntry;

#define CONFIG_LOAD_BUFSIZE 256

KeyIndex config_key_from_scancode(int scan);
GamepadKeyIndex config_gamepad_key_from_gamepad_button(int btn);
KeyIndex config_key_from_gamepad_key(GamepadKeyIndex gpkey);
GamepadKeyIndex config_gamepad_key_from_key(KeyIndex key);
KeyIndex config_key_from_gamepad_button(int btn);

void config_reset(void);
void config_init(void);
void config_uninit(void);
void config_load(char *filename);
void config_save(char *filename);
void config_set_callback(ConfigIndex idx, ConfigCallback callback);

#ifndef DEBUG
	#define CONFIG_RAWACCESS
#endif

#ifdef CONFIG_RAWACCESS
	#define CONFIGDEFS_EXPORT
	extern ConfigEntry configdefs[];
	#define config_get(idx) (configdefs + (idx))
#else
	#define CONFIGDEFS_EXPORT static
	ConfigEntry* config_get(ConfigIndex idx);
#endif

#define config_get_int(idx) (config_get(idx)->val.i)
#define config_get_float(idx) (config_get(idx)->val.f)
#define config_get_str(idx) (config_get(idx)->val.s)

int config_set_int(ConfigIndex idx, int val);
double config_set_float(ConfigIndex idx, double val);
char* config_set_str(ConfigIndex idx, const char *val);

#endif
