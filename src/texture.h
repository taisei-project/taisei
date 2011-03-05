/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_opengl.h>
#include <math.h>

typedef struct {
	int w, h;
	int truew, trueh;
	GLuint gltex;
} Texture;

void load_texture(const char *filename, Texture *texture);
void load_sdl_surf(SDL_Surface *surface, Texture *texture);

void draw_texture(int x, int y, Texture *tex);
#endif