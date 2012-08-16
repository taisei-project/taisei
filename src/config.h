/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */
 
#ifndef CONFIG_H
#define CONFIG_H

#include <SDL/SDL_keysym.h>

typedef struct Config {
	int 	intval[64];
	char* 	strval[64];
	float 	fltval[64];
} Config;

extern Config tconfig;

/*
 * <Akari> IMPORTANT: When adding new controls, ALWAYS add them RIGHT AFTER the last KEY_* constant.
 * Not doing so will likely break replays! And don't forget to update CONFIG_KEY_LAST below.
 * Same goes for GP_*.
 */

typedef enum ConfigKey {
	KEY_UP = 0,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_FOCUS,
	KEY_SHOT,
	KEY_BOMB,
	
	KEY_FULLSCREEN,
	KEY_SCREENSHOT,
	KEY_SKIP,
	
	FULLSCREEN,
	
	NO_SHADER,
	NO_AUDIO,
	
	NO_STAGEBG,
	NO_STAGEBG_FPSLIMIT,
	
	SAVE_RPY,
	
	VID_WIDTH,
	VID_HEIGHT,
	
	PLAYERNAME,
	
	GAMEPAD_ENABLED,
	GAMEPAD_DEVICE,
	GAMEPAD_AXIS_UD,
	GAMEPAD_AXIS_LR,
	GAMEPAD_AXIS_UD_SENS,
	GAMEPAD_AXIS_LR_SENS,
	GAMEPAD_AXIS_DEADZONE,
	GAMEPAD_AXIS_FREE,
	
	// gamepad controls
	// The UDLR ones should work without adjusting - but you can assign custom buttons to them if you really need to
	GP_UP,
	GP_DOWN,
	GP_LEFT,
	GP_RIGHT,
	GP_FOCUS,
	GP_SHOT,
	GP_BOMB,
	GP_SKIP,
	GP_PAUSE
} ConfigKey;

typedef enum ConfigKeyType {
	CFGT_INT,
	CFGT_STRING,
	CFGT_KEYBINDING,
	CFGT_FLOAT
} ConfigKeyType;

typedef struct ConfigEntry {
	ConfigKeyType type;
	ConfigKey key;
	char *name;
} ConfigEntry;

extern ConfigEntry configdefs[];
Config tconfig;

#define CONFIG_KEY_FIRST KEY_UP
#define CONFIG_KEY_LAST KEY_SKIP

#define CONFIG_GPKEY_FIRST GP_UP
#define CONFIG_GPKEY_LAST GP_PAUSE

#define CONFIG_LOAD_BUFSIZE 256

int config_sym2key(int sym);
int config_button2gpkey(int btn);
int config_gpkey2key(int btn);
int config_button2key(int btn);

void config_preset(void);
void config_load(char *filename);
void config_save(char *filename);
ConfigEntry* config_findentry(char *name);

int config_intval(char*);
int config_intval_p(ConfigEntry*);
char* config_strval(char*);
char* config_strval_p(ConfigEntry*);
float config_fltval(char*);
float config_fltval_p(ConfigEntry*);

#endif
