/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef BGM_H
#define BGM_H

#include "audio.h"

typedef struct Bgm_desc {
	struct Bgm_desc *next;
	struct Bgm_desc *prev;
	
	char *name;
	char *value;
} Bgm_desc;

struct current_bgm_t {
	char *name;
	char *title;
	Sound *data;
	int started_at;
};

Sound *load_bgm(char *filename, const char *type);

extern struct current_bgm_t current_bgm;

void start_bgm(char *name);
void stop_bgm(void);
void continue_bgm(void);
void save_bgm(void);
void restore_bgm(void);

int init_bgm(int *argc, char *argv[]);
void shutdown_bgm(void);

void load_bgm_descriptions(const char *path);

void delete_music(void);

void set_bgm_volume(int value);

#endif
