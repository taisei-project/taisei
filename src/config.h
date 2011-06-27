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
	
	NO_SHADER,
	NO_AUDIO
};

void parse_config();
void config_preset();

#endif