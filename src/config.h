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
	int intval[64];
} Config;

extern Config tconfig;

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
	
	FULLSCREEN,
	
	NO_SHADER,
	NO_AUDIO,
	
	NO_STAGEBG,
	NO_STAGEBG_FPSLIMIT,
	
	SAVE_RPY,
	
	VID_WIDTH,
	VID_HEIGHT,
};

void parse_config(char *filename);
void config_preset();

#define CONFIG_KEY_FIRST KEY_UP
#define CONFIG_KEY_LAST KEY_SCREENSHOT
int config_sym2key(int sym);

#endif
