/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef NATIVE_H
#define NATIVE_H

const char *get_prefix(void);
const char *get_config_path(void);
const char *get_screenshots_path(void);
const char *get_replays_path(void);

void init_paths(void);

#endif
