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

#ifndef FONT_H
#define FONT_H

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "texture.h"

Texture *load_text(const char *text, TTF_Font *font);
void draw_text(const char *text, int x, int y, TTF_Font *font);

struct Fonts {
	TTF_Font *biolinum;
};

extern struct Fonts _fonts;

#endif