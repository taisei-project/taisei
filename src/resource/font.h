/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef FONT_H
#define FONT_H

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "texture.h"

typedef enum {
	AlCenter,
	AlLeft,
	AlRight
} Alignment;

Texture *load_text(const char *text, TTF_Font *font);
void draw_text(Alignment align, float x, float y, const char *text, TTF_Font *font);
void init_fonts();

struct Fonts {
	TTF_Font *standard;
	TTF_Font *mainmenu;
};

extern struct Fonts _fonts;

#endif