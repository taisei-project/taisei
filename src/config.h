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
#include "parser.h"

typedef struct Config {
	int 	intval[64];
	char* 	strval[64];
} Config;

extern Config tconfig;

/*
 * <Akari> IMPORTANT: When adding new controls, ALWAYS add them RIGHT AFTER the last KEY_* constant.
 * Not doing so will likely break replays! And don't forget to update CONFIG_KEY_LAST below.
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
	
	PLAYERNAME
} ConfigKey;

typedef enum ConfigKeyType {
	CFGT_INT,
	CFGT_STRING,
	CFGT_KEYBINDING
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
#define CONFIG_LOAD_BUFSIZE 256

int config_sym2key(int sym);
void config_preset();
void config_load(char *filename);
void config_save(char *filename);
ConfigEntry* config_findentry(char *name);

inline int config_intval(char*);
inline int config_intval_p(ConfigEntry*);
inline int config_strval(char*);
inline int config_strval_p(ConfigEntry*)

#endif
