/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_resource_texture_h
#define IGUARD_resource_texture_h

#include "taisei.h"

#include "util.h"
#include "resource.h"

typedef struct Texture Texture;

char *texture_path(const char *name);
bool check_texture_path(const char *path);

void begin_draw_texture(FloatRect dest, FloatRect frag, Texture *tex);
void end_draw_texture(void);

void fill_viewport(float xoff, float yoff, float ratio, const char *name);
void fill_viewport_p(float xoff, float yoff, float ratio, float aspect, float angle, Texture *tex);

void fill_screen(const char *name);
void fill_screen_p(Texture *tex);

void loop_tex_line_p(cmplx a, cmplx b, float w, float t, Texture *texture);
void loop_tex_line(cmplx a, cmplx b, float w, float t, const char *texture);

Texture *get_tex(const char *name);
Texture *prefix_get_tex(const char *name, const char *prefix);

extern ResourceHandler texture_res_handler;

#define TEX_PATH_PREFIX "res/gfx/"
#define TEX_EXTENSION ".tex"

#endif // IGUARD_resource_texture_h
