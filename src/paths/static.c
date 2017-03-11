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
	conf_path = strjoin(getenv("HOME"), CFG_DIR, NULL);
	scr_path = strjoin(conf_path, SCR_DIR, NULL);
	rpy_path = strjoin(conf_path, RPY_DIR, NULL);
}
