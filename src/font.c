/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "font.h"
#include "global.h"
#include <assert.h>

struct Fonts _fonts;

void init_fonts() {
	TTF_Init();
	_fonts.biolinum = TTF_OpenFont(FILE_PREFIX "gfx/LinBiolinum.ttf", 20);
	assert(_fonts.biolinum);
}

Texture *load_text(const char *text, TTF_Font *font) {
	Texture *tex = malloc(sizeof(Texture));
	SDL_Color clr = {255,255,255};
	SDL_Surface *surf = TTF_RenderText_Blended(font, text, clr);
	assert(surf != NULL);	
	
	load_sdl_surf(surf, tex);
	SDL_FreeSurface(surf);
	
	return tex;
}

void draw_text(const char *text, int x, int y, TTF_Font *font) {
	Texture *tex = load_text(text, font);
	draw_texture_p(x, y, tex);
	free_texture(tex);
}