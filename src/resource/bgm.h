/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef BGM_H
#define BGM_H

#include "audio.h"

struct current_bgm_t {
	char *name;
	char *title;
	int isboss;
	Sound *data;
	int started_at;
};

extern struct current_bgm_t current_bgm;

Sound *load_bgm(const char *filename);

void start_bgm(const char *name);
void stop_bgm(void);
void continue_bgm(void);
void save_bgm(void);
void restore_bgm(void);

int init_bgm(void);
void shutdown_bgm(void);

void load_bgm_descriptions(const char *path);

void delete_music(void);

void set_bgm_volume(float gain);

#endif
