/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "native.h"

const char *get_prefix(void) {
	return "./data/";
}

const char *get_config_path(void) {
	return ".";
}

const char *get_screenshots_path(void) {
	return "./screenshots";
}

const char *get_replays_path(void) {
	return "./replays";
}

void init_paths(void) {
}
