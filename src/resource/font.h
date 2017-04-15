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


// Size of the buffer used by the font renderer at quality == 1.0.
// No text larger than this can be drawn.
enum {
	FONTREN_MAXW = 1024,	// must be a power of two that is >= SCREEN_W
	FONTREN_MAXH = 64,		// must be a power of two that is > largest font size
};

typedef struct FontRenderer FontRenderer;
struct FontRenderer {
	Texture tex;
	GLuint pbo;
	float quality;
};

void fontrenderer_init(FontRenderer *f, float quality);
void fontrenderer_free(FontRenderer *f);
void fontrenderer_draw(FontRenderer *f, const char *text, TTF_Font *font);
void fontrenderer_draw_prerendered(FontRenderer *f, SDL_Surface *surf);
SDL_Surface* fontrender_render(FontRenderer *f, const char *text, TTF_Font *font);

Texture *load_text(const char *text, TTF_Font *font);
void draw_text(Alignment align, float x, float y, const char *text, TTF_Font *font);
void draw_text_prerendered(Alignment align, float x, float y, SDL_Surface *surf);

int stringwidth(char *s, TTF_Font *font);
int stringheight(char *s, TTF_Font *font);
int charwidth(char c, TTF_Font *font);

void init_fonts(void);
void uninit_fonts(void);
void load_fonts(float quality);
void reload_fonts(float quality);
void free_fonts(void);

struct Fonts {
	TTF_Font *standard;
	TTF_Font *mainmenu;
	TTF_Font *small;
};

extern struct Fonts _fonts;

#endif
