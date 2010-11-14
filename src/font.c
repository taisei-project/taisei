/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
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
	SDL_Surface *surf = TTF_RenderText_Blended(font, text, ((SDL_Color){255,255,255}));
	assert(surf != NULL);	
	
	load_sdl_surf(surf, tex);
	SDL_FreeSurface(surf);
	
	return tex;
}

void draw_text(const char *text, int x, int y, TTF_Font *font) {
	Texture *tex = load_text(text, font);
	draw_texture(x, y, tex);
	free(tex);
}