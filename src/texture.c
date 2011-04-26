/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "texture.h"
#include "global.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "audio.h"
#include "shader.h"
#include "list.h"

void recurse_dir(char *path) {
	DIR *dir = opendir(path);
	if(dir == NULL)
		err(-1, "Couldn't open directory '%s'", path);
	struct dirent *dp;
	
	char buf[512];
			
	while((dp = readdir(dir)) != NULL) {
		strncpy(buf, path, sizeof(buf));
		strncat(buf, "/", sizeof(buf));
		strncat(buf, dp->d_name, sizeof(buf));
		
		struct stat statbuf;
		stat(buf, &statbuf);
		
		if(S_ISDIR(statbuf.st_mode) && dp->d_name[0] != '.') {
			recurse_dir(buf);
		} else if(strcmp(dp->d_name + strlen(dp->d_name)-4, ".png") == 0) {
			if(strncmp(dp->d_name, "ani_", 4) == 0)
				init_animation(buf);
			else
				load_texture(buf);			
		} else if(strcmp(dp->d_name + strlen(dp->d_name)-4, ".wav") == 0) {
			load_sound(buf);
		} else if(strcmp(dp->d_name + strlen(dp->d_name)-4, ".sha") == 0) {
			load_shader(buf);
		}
	}
}

void load_resources() {
	printf("load_resources():\n");
	char *path = malloc(sizeof(FILE_PREFIX)+4);
	
	printf("- textures:\n");
	strcpy(path, FILE_PREFIX);
	strncat(path, "gfx", sizeof(FILE_PREFIX)+4);
	recurse_dir(path);
	
	printf("- sounds:\n");
	strcpy(path, FILE_PREFIX);
	strncat(path, "sfx", sizeof(FILE_PREFIX)+4);
	recurse_dir(path);
	
	printf("- shader:\n");
	strcpy(path, FILE_PREFIX);
	strncat(path, "shader", sizeof(FILE_PREFIX)+4);
	recurse_dir(path);
}

Texture *get_tex(char *name) {
	Texture *t, *res = NULL;
	for(t = global.textures; t; t = t->next) {
		if(strcmp(t->name, name) == 0)
			res = t;
	}
	
	if(res == NULL)
		errx(-1,"get_tex():\n!- cannot load texture '%s'", name);
	
	return res;
}

void delete_texture(void **texs, void *tex) {
	Texture *t = (Texture *)tex;
	free(t->name);
	glDeleteTextures(1, &t->gltex);
	
	delete_element((void **)texs, tex);
}

void delete_textures() {
	delete_all_elements((void **)&global.textures, delete_texture);
}

Texture *load_texture(const char *filename) {	
	SDL_Surface *surface = IMG_Load(filename);
	
	if(surface == NULL)
		err(-1,"load_texture():\n!- cannot load '%s'", filename);
	
	Texture *texture = create_element((void **)&global.textures, sizeof(Texture));
	load_sdl_surf(surface, texture);	
	SDL_FreeSurface(surface);
	
	char *beg = strstr(filename, "gfx/") + 4;
	char *end = strrchr(filename, '.');
	
	texture->name = malloc(end - beg + 1);
	memset(texture->name, 0, end-beg + 1);
	strncpy(texture->name, beg, end-beg);
	
	printf("-- loaded '%s' as '%s'\n", filename, texture->name);
	
	return texture;
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

void free_texture(Texture *tex) {
	glDeleteTextures(1, &tex->gltex);
	free(tex);
}

void draw_texture(int x, int y, char *name) {
	draw_texture_p(x, y, get_tex(name));
}

void draw_texture_p(int x, int y, Texture *tex) {
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