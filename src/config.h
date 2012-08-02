/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
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

enum {
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
};

void parse_config(char *filename);
void config_preset();

#define CONFIG_KEY_FIRST KEY_UP
#define CONFIG_KEY_LAST KEY_SKIP

int config_sym2key(int sym);

#endif
