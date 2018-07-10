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
#include "util.h"
#include "resource.h"

typedef struct Texture Texture;

char* texture_path(const char *name);
bool check_texture_path(const char *path);

void begin_draw_texture(FloatRect dest, FloatRect frag, Texture *tex);
void end_draw_texture(void);

void draw_texture_with_size(float x, float y, float w, float h, const char *name);
void draw_texture_with_size_p(float x, float y, float w, float h, Texture *tex);

void fill_viewport(float xoff, float yoff, float ratio, const char *name);
void fill_viewport_p(float xoff, float yoff, float ratio, float aspect, float angle, Texture *tex);

void fill_screen(const char *name);
void fill_screen_p(Texture *tex);

void loop_tex_line_p(complex a, complex b, float w, float t, Texture *texture);
void loop_tex_line(complex a, complex b, float w, float t, const char *texture);

Texture* get_tex(const char *name);
Texture* prefix_get_tex(const char *name, const char *prefix);

extern ResourceHandler texture_res_handler;

#define TEX_PATH_PREFIX "res/gfx/"
#define TEX_EXTENSION ".tex"
