/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef FONT_H
#define FONT_H

#include <SDL_ttf.h>
#include "texture.h"

typedef enum {
	AL_Center,
	AL_Left,
	AL_Right
} Alignment;


// Size of the buffer used by the font renderer. No text larger than this can be drawn.
enum {
	FONTREN_MAXW = 1024,
	FONTREN_MAXH = 512
};

typedef struct FontRenderer FontRenderer;
struct FontRenderer {
	Texture tex;
	GLuint pbo;
};

void fontrenderer_init(FontRenderer *f);
void fontrenderer_free(FontRenderer *f);
void fontrenderer_draw(FontRenderer *f, const char *text,TTF_Font *font);

Texture *load_text(const char *text, TTF_Font *font);
void draw_text(Alignment align, float x, float y, const char *text, TTF_Font *font);
void init_fonts(void);
void free_fonts(void);

int stringwidth(char *s, TTF_Font *font);
int stringheight(char *s, TTF_Font *font);
int charwidth(char c, TTF_Font *font);

struct Fonts {
	TTF_Font *standard;
	TTF_Font *mainmenu;
};

extern struct Fonts _fonts;

#endif
