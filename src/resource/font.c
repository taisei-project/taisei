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
	_fonts.standard = TTF_OpenFont(FILE_PREFIX "gfx/LinBiolinum.ttf", 20);
	_fonts.mainmenu = TTF_OpenFont(FILE_PREFIX "gfx/immortal.ttf", 35);
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

void draw_text(Alignment align, float x, float y, const char *text, TTF_Font *font) {
	char *nl;
	char *buf = malloc(strlen(text)+1);
	strcpy(buf, text);
	
	if((nl = strchr(buf, '\n')) != NULL && strlen(nl) > 1) {
		draw_text(AlCenter, x, y + 20, nl+1, font);
		*nl = '\0';
	}
		
	Texture *tex = load_text(buf, font);
	
	switch(align) {
	case AlCenter:
		draw_texture_p(x, y, tex);
		break;
	case AlLeft:
		draw_texture_p(x + tex->w/2.0, y, tex);
		break;
	case AlRight:
		draw_texture_p(x - tex->w/2.0, y, tex);
		break;
	}
		
	free_texture(tex);
	free(buf);
}

