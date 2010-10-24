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

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <err.h>

#include "global.h"
#include "stages/stage0.h"

SDL_Surface *display;

void init_gl() {
	glClearColor(0, 0, 0, 0);
	
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
		
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	
	glViewport(0, 0, SCREEN_W, SCREEN_H);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glOrtho(0, SCREEN_W, SCREEN_H, 0, -1000, 1000);
	glMatrixMode(GL_MODELVIEW);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

void shutdown() {
	SDL_FreeSurface(display);
	SDL_Quit();
}

int main(int argc, char** argv) {
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		errx(-1, "Error initializing SDL: %s", SDL_GetError());
		
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);
	if((display = SDL_SetVideoMode(RESX, RESY, 32, SDL_OPENGL)) == NULL)
		errx(-1, "Error opening screen: %s", SDL_GetError());
	
	init_gl();
	init_global();
	
	stage0_loop();
	
	atexit(shutdown);	
}