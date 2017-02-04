/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "native.h"
#include "global.h"
#include <stdlib.h>
#include <string.h>

#define DATA_DIR "data/"

#define SCR_DIR "screenshots"
#define RPY_DIR "replays"

char *content_path;
char *conf_path;
char *scr_path;
char *rpy_path;

const char *get_prefix(void) {
	return content_path;
}

const char *get_config_path(void) {
	return conf_path;
}

const char *get_screenshots_path(void) {
	return scr_path;
}

const char *get_replays_path(void) {
	return rpy_path;
}

void init_paths(void) {
#ifdef RELATIVE
	char *basedir = SDL_GetBasePath();
	content_path = malloc(strlen(basedir) + strlen(DATA_DIR) + 1);
	strcpy(content_path, basedir);
	strcat(content_path, DATA_DIR);
	free(basedir);
#else
	content_path = FILE_PREFIX;
#endif

	conf_path = SDL_GetPrefPath("", "taisei");

	scr_path = malloc(strlen(SCR_DIR) + strlen(get_config_path()) + 1);
	strcpy(scr_path, get_config_path());
	strcat(scr_path, SCR_DIR);

	rpy_path = malloc(strlen(RPY_DIR) + strlen(get_config_path()) + 1);
	strcpy(rpy_path, get_config_path());
	strcat(rpy_path, RPY_DIR);
}
