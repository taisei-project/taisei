/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "font.h"
#include "global.h"
#include "paths/native.h"
#include <assert.h>

struct Fonts _fonts;

TTF_Font *load_font(char *name, int size) {
	char *buf = malloc(strlen(get_prefix()) + strlen(name)+1);
	strcpy(buf, get_prefix());
	strcat(buf, name);
	
	TTF_Font *f =  TTF_OpenFont(buf, size);
	if(!f)
		errx(-1, "failed to load font '%s'", buf);
	
	printf("-- loaded '%s'\n", buf);
	
	free(buf);
	return f;
}

void init_fonts() {
	TTF_Init();
	
	_fonts.standard = load_font("gfx/LinBiolinum.ttf", 20);
	_fonts.mainmenu = load_font("gfx/immortal.ttf", 35);
		
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
		draw_text(align, x, y + 20, nl+1, font);
		*nl = '\0';
	}
		
	Texture *tex = load_text(buf, font);
	
	switch(align) {
	case AL_Center:
		draw_texture_p(x, y, tex);
		break;
	case AL_Left:
		draw_texture_p(x + tex->w/2.0, y, tex);
		break;
	case AL_Right:
		draw_texture_p(x - tex->w/2.0, y, tex);
		break;
	}
		
	free_texture(tex);
	free(buf);
}

