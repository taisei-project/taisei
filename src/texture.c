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
#include <png.h>
#include <err.h>
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
	SDL_Surface *surface = load_png(filename);
	
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

SDL_Surface *load_png(const char *filename) {
	FILE *fp = fopen(filename, "rb");
	if(!fp)
		err(-1, "Error loading '%s'", filename);
	
	png_structp png_ptr;
	if(!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		errx(-1,"Error loading '%s'", filename);
	
	png_infop info_ptr;
	if(!(info_ptr = png_create_info_struct(png_ptr))) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		errx(-1,"Error loading '%s'", filename);
	}
	
	png_infop end_info;
	if(!(end_info = png_create_info_struct(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		errx(-1,"Error loading '%s'", filename);
	}
	
	png_init_io(png_ptr, fp);
	
	png_read_png(png_ptr, info_ptr, 0, NULL);
	
	int width = png_get_image_width(png_ptr, info_ptr);
	int height = png_get_image_height(png_ptr, info_ptr);
	int depth = png_get_bit_depth(png_ptr, info_ptr);
	
	png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);
	
	Uint32 *pixels = malloc(sizeof(Uint32)*width*height);
	
	int i;
	for(i = 0; i < height; i++)
		memcpy(&pixels[i*width], row_pointers[i], sizeof(Uint32)*width);
	
	Uint32 rmask, gmask, bmask, amask;
	
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
	
	SDL_Surface *res = SDL_CreateRGBSurfaceFrom(pixels, width, height, depth*4, 0, rmask, gmask, bmask, amask);
	if(!res)
		errx(-1,"Error loading '%s'", filename);
	
	fclose(fp);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	
	return res;
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

void fill_screen(float xoff, float yoff, float ratio, char *name) {
	glEnable(GL_TEXTURE_2D);
	
	Texture *tex = get_tex(name);
	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	
	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(xoff,yoff, 0);
	glMatrixMode(GL_MODELVIEW);
	
	float rw = ratio;
	float rh = ratio;
	
	if(ratio == 0) {
		rw = ((float)tex->w)/tex->truew;
		rh = ((float)tex->h)/tex->trueh;
	}
	
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3f(0,0,0);
	glTexCoord2f(0,rh); glVertex3f(0,VIEWPORT_H,0);
	glTexCoord2f(rw,rh); glVertex3f(VIEWPORT_W,VIEWPORT_H,0);
	glTexCoord2f(rw,0); glVertex3f(VIEWPORT_W,0,0);
	glEnd();
	
	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}