/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include <SDL.h>
#include <stdbool.h>
#include "taiseigl.h"
#include "util.h"

typedef struct Texture Texture;

struct Texture {
	int w, h;
	int truew, trueh;
	GLuint gltex;
};

char* texture_path(const char *name);
void* load_texture_begin(const char *path, unsigned int flags);
void* load_texture_end(void *opaque, const char *path, unsigned int flags);
bool check_texture_path(const char *path);

void load_sdl_surf(SDL_Surface *surface, Texture *texture);
void free_texture(Texture *tex);

void draw_texture(float x, float y, const char *name);
void draw_texture_p(float x, float y, Texture *tex);

void begin_draw_texture(FloatRect dest, FloatRect frag, Texture *tex);
void end_draw_texture(void);

void draw_texture_with_size(float x, float y, float w, float h, const char *name);
void draw_texture_with_size_p(float x, float y, float w, float h, Texture *tex);

void fill_screen(float xoff, float yoff, float ratio, const char *name);
void fill_screen_p(float xoff, float yoff, float ratio, float aspect, float angle, Texture *tex);

void loop_tex_line_p(complex a, complex b, float w, float t, Texture *texture);
void loop_tex_line(complex a, complex b, float w, float t, const char *texture);

Texture* get_tex(const char *name);
Texture* prefix_get_tex(const char *name, const char *prefix);

#define TEX_PATH_PREFIX "res/gfx/"
#define TEX_EXTENSION ".png"
