/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "texture.h"

void load_texture(const char *filename, Texture* texture) {
	SDL_Surface *surface = IMG_Load(filename);
	
	if(surface == NULL)
		err(EXIT_FAILURE,"load_texture():\n-- cannot load '%s'", filename);
	
	load_sdl_surf(surface, texture);	
	SDL_FreeSurface(surface);
}

void load_sdl_surf(SDL_Surface *surface, Texture *texture) {
	glGenTextures(1, &texture->gltex);
	glBindTexture(GL_TEXTURE_2D, texture->gltex);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	int nw = 2;
	int nh = 2;
	
	while(nw < surface->w) nw *= 2;
	while(nh < surface->h) nh *= 2;
	
	Uint32 *tex = calloc(sizeof(Uint32), nw*nh);
	
	int x, y;
	
	for(y = 0; y < nh; y++) {
		for(x = 0; x < nw; x++) {
			if(y < surface->h && x < surface->w)
				tex[y*nw+x] = ((Uint32*)surface->pixels)[y*surface->w+x];
			else
				tex[y*nw+x] = '\0';
		}
	}
	
	texture->w = surface->w;
	texture->h = surface->h;
	
	texture->truew = nw;
	texture->trueh = nh;
	
	
	glTexImage2D(GL_TEXTURE_2D, 0, 4, nw, nh, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
	
	free(tex);
}

void draw_texture(int x, int y, Texture *tex) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	
	glPushMatrix();
	
	float wq = ((float)tex->w)/tex->truew;
	float hq = ((float)tex->h)/tex->trueh;
	
	glTranslatef(x,y,0);
	glScalef(tex->w/2,tex->h/2,1);
	
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex3f(-1, -1, 0);
		glTexCoord2f(0,hq); glVertex3f(-1, 1, 0);
		glTexCoord2f(wq,hq); glVertex3f(1, 1, 0);
		glTexCoord2f(wq,0); glVertex3f(1, -1, 0);
	glEnd();	
		
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}