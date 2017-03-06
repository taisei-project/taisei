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
#include "taisei_err.h"

struct Fonts _fonts;

TTF_Font *load_font(char *name, int size) {
	char *buf = strjoin(get_prefix(), name, NULL);

	TTF_Font *f =  TTF_OpenFont(buf, size);
	if(!f)
		errx(-1, "failed to load font '%s'", buf);

	printf("-- loaded '%s'\n", buf);

	free(buf);
	return f;
}

void init_fonts(void) {
	TTF_Init();

	_fonts.standard = load_font("gfx/LinBiolinum.ttf", 20);
	_fonts.mainmenu = load_font("gfx/immortal.ttf", 35);

}

Texture *load_text(const char *text, TTF_Font *font) {
	Texture *tex = malloc(sizeof(Texture));
	SDL_Color clr = {255,255,255};
	SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, clr);
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

int stringwidth(char *s, TTF_Font *font) {
	int w;
	TTF_SizeText(font, s, &w, NULL);
	return w;
}

int stringheight(char *s, TTF_Font *font) {
	int h;
	TTF_SizeText(font, s, NULL, &h);
	return h;
}

int charwidth(char c, TTF_Font *font) {
	char s[2];
	s[0] = c;
	s[1] = 0;
	return stringwidth(s, font);
}
