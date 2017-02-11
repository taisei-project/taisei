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

#define CFG_DIR "/.taisei"
#define SCR_DIR "/screenshots"
#define RPY_DIR "/replays"

char *conf_path;
char *scr_path;
char *rpy_path;

const char *get_prefix(void) {
	return FILE_PREFIX;
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
	conf_path = malloc(strlen(CFG_DIR) + strlen(getenv("HOME")) + 1);
	strcpy(conf_path, getenv("HOME"));
	strcat(conf_path, CFG_DIR);

	scr_path = malloc(strlen(SCR_DIR) + strlen(get_config_path()) + 1);
	strcpy(scr_path, get_config_path());
	strcat(scr_path, SCR_DIR);

	rpy_path = malloc(strlen(RPY_DIR) + strlen(get_config_path()) + 1);
	strcpy(rpy_path, get_config_path());
	strcat(rpy_path, RPY_DIR);
}
