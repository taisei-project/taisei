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
	AL_Center,
	AL_Left,
	AL_Right
} Alignment;

Texture *load_text(const char *text, TTF_Font *font);
void draw_text(Alignment align, float x, float y, const char *text, TTF_Font *font);
void init_fonts(void);
int stringwidth(char *s, TTF_Font *font);
int stringheight(char *s, TTF_Font *font);
int charwidth(char c, TTF_Font *font);

struct Fonts {
	TTF_Font *standard;
	TTF_Font *mainmenu;
};

extern struct Fonts _fonts;

#endif
