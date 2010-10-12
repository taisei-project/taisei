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

#include "texture.h"

void load_texture(const char *filename, Texture* texture) {
	SDL_Surface *surface = IMG_Load(filename);
	
	glGenTextures(1, &texture->gltex);
	glBindTexture(GL_TEXTURE_2D, texture->gltex);
	
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
	SDL_FreeSurface(surface);
}

void draw_texture(int x, int y, Texture *tex) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	
	glBegin(GL_QUADS);
		glTexCoord2i(0,0);
		glVertex3f(x - tex->w/2, y - tex->h/2, 0.0f);
		
		glTexCoord2i(0,1);
		glVertex3f(x - tex->w/2, y + tex->trueh-tex->h/2, 0.0f);
		
		glTexCoord2i(1,1);
		glVertex3f(x + tex->truew-tex->w/2, y + tex->trueh-tex->h/2, 0.0f);
		
		glTexCoord2i(1,0);
		glVertex3f(x + tex->truew-tex->w/2, y - tex->h/2, 0.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}