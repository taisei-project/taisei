/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL_keycode.h>

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
 *  DO NOT REORDER KEYBINDINGS.
 *
 *  The KEY_* constants are used by replays. These are formed from KEYDEFS.
 *  Always add new keys at the end of KEYDEFS, never put them directly into CONFIGDEFS.
 *
 *  KEYDEFS can be safely moved around within CONFIGDEFS, however.
 *  Stuff in GPKEYDEFS is safe to reorder.
 */

#define KEYDEFS \
	CONFIGDEF_KEYBINDING(KEY_UP,                "key_up",               SDL_SCANCODE_UP) \
	CONFIGDEF_KEYBINDING(KEY_DOWN,              "key_down",             SDL_SCANCODE_DOWN) \
	CONFIGDEF_KEYBINDING(KEY_LEFT,              "key_left",             SDL_SCANCODE_LEFT) \
	CONFIGDEF_KEYBINDING(KEY_RIGHT,             "key_right",            SDL_SCANCODE_RIGHT) \
	CONFIGDEF_KEYBINDING(KEY_FOCUS,             "key_focus",            SDL_SCANCODE_LSHIFT) \
	CONFIGDEF_KEYBINDING(KEY_SHOT,              "key_shot",             SDL_SCANCODE_Z) \
	CONFIGDEF_KEYBINDING(KEY_BOMB,              "key_bomb",             SDL_SCANCODE_X) \
	CONFIGDEF_KEYBINDING(KEY_SPECIAL,           "key_special",          SDL_SCANCODE_C) \
	CONFIGDEF_KEYBINDING(KEY_SKIP,              "key_skip",             SDL_SCANCODE_LCTRL) \
	CONFIGDEF_KEYBINDING(KEY_FULLSCREEN,        "key_fullscreen",       SDL_SCANCODE_F11) \
	CONFIGDEF_KEYBINDING(KEY_SCREENSHOT,        "key_screenshot",       SDL_SCANCODE_P) \
	CONFIGDEF_KEYBINDING(KEY_IDDQD,             "key_iddqd",            SDL_SCANCODE_Q) \
	CONFIGDEF_KEYBINDING(KEY_HAHAIWIN,          "key_skipstage",        SDL_SCANCODE_E) \
	CONFIGDEF_KEYBINDING(KEY_PAUSE,             "key_pause",            SDL_SCANCODE_PAUSE) \
	CONFIGDEF_KEYBINDING(KEY_NOBACKGROUND,      "key_nobackground",     SDL_SCANCODE_LALT) \
	CONFIGDEF_KEYBINDING(KEY_POWERUP,           "key_powerup",          SDL_SCANCODE_2) \
	CONFIGDEF_KEYBINDING(KEY_POWERDOWN,         "key_powerdown",        SDL_SCANCODE_1) \
	CONFIGDEF_KEYBINDING(KEY_FPSLIMIT_OFF,      "key_fpslimit_off",     SDL_SCANCODE_RSHIFT) \
	CONFIGDEF_KEYBINDING(KEY_STOP,              "key_stop",             SDL_SCANCODE_F1) \
	CONFIGDEF_KEYBINDING(KEY_RESTART,           "key_restart",          SDL_SCANCODE_F2) \
	CONFIGDEF_KEYBINDING(KEY_HITAREAS,          "key_hitareas",         SDL_SCANCODE_H) \
	CONFIGDEF_KEYBINDING(KEY_TOGGLE_AUDIO,      "key_toggle_audio",     SDL_SCANCODE_M) \
	CONFIGDEF_KEYBINDING(KEY_RELOAD_RESOURCES,  "key_reload_resources", SDL_SCANCODE_F5) \
	CONFIGDEF_KEYBINDING(KEY_QUICKSAVE,         "key_quicksave",        SDL_SCANCODE_F4) \
	CONFIGDEF_KEYBINDING(KEY_QUICKLOAD,         "key_quickload",        SDL_SCANCODE_F3) \


#define GPKEYDEFS \
	CONFIGDEF_GPKEYBINDING(KEY_UP,              "gamepad_key_up",       GAMEPAD_BUTTON_DPAD_UP) \
	CONFIGDEF_GPKEYBINDING(KEY_DOWN,            "gamepad_key_down",     GAMEPAD_BUTTON_DPAD_DOWN) \
	CONFIGDEF_GPKEYBINDING(KEY_LEFT,            "gamepad_key_left",     GAMEPAD_BUTTON_DPAD_LEFT) \
	CONFIGDEF_GPKEYBINDING(KEY_RIGHT,           "gamepad_key_right",    GAMEPAD_BUTTON_DPAD_RIGHT) \
	CONFIGDEF_GPKEYBINDING(KEY_FOCUS,           "gamepad_key_focus",    GAMEPAD_BUTTON_X) \
	CONFIGDEF_GPKEYBINDING(KEY_SHOT,            "gamepad_key_shot",     GAMEPAD_BUTTON_A) \
	CONFIGDEF_GPKEYBINDING(KEY_BOMB,            "gamepad_key_bomb",     GAMEPAD_BUTTON_TRIGGER_LEFT) \
	CONFIGDEF_GPKEYBINDING(KEY_SPECIAL,         "gamepad_key_special",  GAMEPAD_BUTTON_TRIGGER_RIGHT) \
	CONFIGDEF_GPKEYBINDING(KEY_SKIP,            "gamepad_key_skip",     GAMEPAD_BUTTON_B) \


#ifdef __EMSCRIPTEN__
	#define CONFIG_VSYNC_DEFAULT 1
	#define CONFIG_FXAA_DEFAULT 0
#else
	#define CONFIG_VSYNC_DEFAULT 0
	#define CONFIG_FXAA_DEFAULT 1
#endif

#define CONFIG_CHUNKSIZE_DEFAULT 512

#define CONFIG_GAMEPAD_BTNREPEAT_DELAY_DEFAULT 0.5
#define CONFIG_GAMEPAD_BTNREPEAT_INTERVAL_DEFAULT 0.04

#define CONFIGDEFS \
	 /* @version must be on top. don't change its default value here, it does nothing. */ \
	CONFIGDEF_INT       (VERSION,                   "@version",                             0) \
	\
	CONFIGDEF_STRING    (PLAYERNAME,                "playername",                           "Player") \
	CONFIGDEF_STRING    (LOCALE,                    "locale",                               "system") \
	CONFIGDEF_INT       (FULLSCREEN,                "fullscreen",                           1) \
	CONFIGDEF_INT       (VID_WIDTH,                 "vid_width",                            RESX) \
	CONFIGDEF_INT       (VID_HEIGHT,                "vid_height",                           RESY) \
	CONFIGDEF_INT       (VID_DISPLAY,               "vid_display",                          0) \
	CONFIGDEF_INT       (VID_RESIZABLE,             "vid_resizable",                        0) \
	CONFIGDEF_INT       (VID_FRAMESKIP,             "vid_frameskip",                        1) \
	CONFIGDEF_INT       (VSYNC,                     "vsync",                                CONFIG_VSYNC_DEFAULT) \
	CONFIGDEF_INT       (MIXER_CHUNKSIZE,           "mixer_chunksize",                      CONFIG_CHUNKSIZE_DEFAULT) \
	CONFIGDEF_FLOAT     (SFX_VOLUME,                "sfx_volume",                           0.5) \
	CONFIGDEF_FLOAT     (BGM_VOLUME,                "bgm_volume",                           1.0) \
	CONFIGDEF_INT       (MUTE_AUDIO,                "mute_audio",                           0) \
	CONFIGDEF_INT       (NO_STAGEBG,                "disable_stagebg",                      0) \
	CONFIGDEF_INT       (SAVE_RPY,                  "save_rpy",                             2) \
	CONFIGDEF_INT       (SPELLSTAGE_AUTORESTART,    "spellpractice_restart_on_fail",        0) \
	CONFIGDEF_INT       (PRACTICE_POWER,            "practice_power",                       400) \
	CONFIGDEF_FLOAT     (TEXT_QUALITY,              "text_quality",                         1.0) \
	CONFIGDEF_FLOAT     (FG_QUALITY,                "fg_quality",                           1.0) \
	CONFIGDEF_FLOAT     (BG_QUALITY,                "bg_quality",                           1.0) \
	CONFIGDEF_INT       (SHOT_INVERTED,             "shot_inverted",                        0) \
	CONFIGDEF_INT       (FOCUS_LOSS_PAUSE,          "focus_loss_pause",                     1) \
	CONFIGDEF_INT       (PARTICLES,                 "particles",                            1) \
	CONFIGDEF_INT       (FXAA,                      "fxaa",                                 CONFIG_FXAA_DEFAULT) \
	CONFIGDEF_INT       (POSTPROCESS,               "postprocess",                          2) \
	CONFIGDEF_INT       (HEALTHBAR_STYLE,           "healthbar_style",                      1) \
	CONFIGDEF_INT       (SKIP_SPEED,                "skip_speed",                           10) \
	CONFIGDEF_FLOAT     (SCORETEXT_ALPHA,           "scoretext_alpha",                      1) \
	CONFIGDEF_INT       (AUTO_SURGE,                "auto_surge",                           1) \
	KEYDEFS \
	CONFIGDEF_INT       (GAMEPAD_ENABLED,           "gamepad_enabled",                      1) \
	CONFIGDEF_STRING    (GAMEPAD_DEVICE,            "gamepad_device",                       "any") \
	CONFIGDEF_INT       (GAMEPAD_AXIS_UD,           "gamepad_axis_ud",                      1) \
	CONFIGDEF_INT       (GAMEPAD_AXIS_LR,           "gamepad_axis_lr",                      0) \
	CONFIGDEF_INT       (GAMEPAD_AXIS_SQUARECIRCLE, "gamepad_axis_square_to_circle",        0) \
	CONFIGDEF_FLOAT     (GAMEPAD_AXIS_SENS,         "gamepad_axis_sensitivity",             0.0) \
	CONFIGDEF_FLOAT     (GAMEPAD_AXIS_UD_SENS,      "gamepad_axis_ud_sensitivity",          0.0) \
	CONFIGDEF_FLOAT     (GAMEPAD_AXIS_LR_SENS,      "gamepad_axis_lr_sensitivity",          0.0) \
	CONFIGDEF_FLOAT     (GAMEPAD_AXIS_DEADZONE,     "gamepad_axis_deadzone",                0.1) \
	CONFIGDEF_FLOAT     (GAMEPAD_AXIS_MAXZONE,      "gamepad_axis_maxzone",                 0.9) \
	CONFIGDEF_FLOAT     (GAMEPAD_AXIS_SNAP,         "gamepad_axis_snap",                    0.5) \
	CONFIGDEF_FLOAT     (GAMEPAD_AXIS_SNAP_DIAG_BIAS, "gamepad_axis_snap_diagonal_bias",    0.0) \
	CONFIGDEF_FLOAT     (GAMEPAD_BTNREPEAT_DELAY,   "gamepad_button_repeat_delay",          CONFIG_GAMEPAD_BTNREPEAT_DELAY_DEFAULT) \
	CONFIGDEF_FLOAT     (GAMEPAD_BTNREPEAT_INTERVAL,"gamepad_button_repeat_interval",       CONFIG_GAMEPAD_BTNREPEAT_INTERVAL_DEFAULT) \
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
	CONFIG_TYPE_GPKEYBINDING,
	CONFIG_TYPE_FLOAT
} ConfigEntryType;

typedef union ConfigValue {
	char *s;
	double f;
	int i;
} ConfigValue;

typedef struct ConfigEntry {
	char *name;
	ConfigValue val;
	ConfigEntryType type;
} ConfigEntry;

KeyIndex config_key_from_scancode(int scan);
GamepadKeyIndex config_gamepad_key_from_gamepad_button(int btn);
KeyIndex config_key_from_gamepad_key(GamepadKeyIndex gpkey);
GamepadKeyIndex config_gamepad_key_from_key(KeyIndex key);
KeyIndex config_key_from_gamepad_button(int btn);

void config_load(void);
void config_save(void);
void config_reset(void);
void config_shutdown(void);

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
char* config_set_str(ConfigIndex idx, const char *val) attr_nonnull(2) attr_returns_nonnull;
