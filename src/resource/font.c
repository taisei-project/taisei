/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "font.h"
#include "global.h"
#include "paths/native.h"

struct Fonts _fonts;

TTF_Font *load_font(char *name, int size) {
	char *buf = strjoin(get_prefix(), name, NULL);

	TTF_Font *f =  TTF_OpenFont(buf, size);
	if(!f)
		log_fatal("Failed to load font '%s'", buf);

	log_info("Loaded '%s'", buf);

	free(buf);
	return f;
}

void fontrenderer_init(FontRenderer *f) {
	glGenBuffers(1,&f->pbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, f->pbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, FONTREN_MAXW*FONTREN_MAXH*4, NULL, GL_STREAM_DRAW);
	glGenTextures(1,&f->tex.gltex);

	f->tex.truew = FONTREN_MAXW;
	f->tex.trueh = FONTREN_MAXH;

	glBindTexture(GL_TEXTURE_2D,f->tex.gltex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, f->tex.truew, f->tex.trueh, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void fontrenderer_free(FontRenderer *f) {
	glDeleteBuffers(1,&f->pbo);
	glDeleteTextures(1,&f->tex.gltex);
}

void fontrenderer_draw(FontRenderer *f, const char *text,TTF_Font *font) {
	SDL_Color clr = {255,255,255};
	SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, clr);
	if(surf == 0) {
		log_fatal("SDL_ttf: %s", TTF_GetError());
	}
	assert(surf != NULL);

	if(surf->w > FONTREN_MAXW || surf->h > FONTREN_MAXH) {
		log_fatal("Text drawn (%dx%d) is too big for the internal buffer (%dx%d).", surf->pitch, surf->h, FONTREN_MAXW, FONTREN_MAXH);
	}
	f->tex.w = surf->w;
	f->tex.h = surf->h;

	glBindTexture(GL_TEXTURE_2D,f->tex.gltex);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, f->pbo);

	glBufferData(GL_PIXEL_UNPACK_BUFFER, FONTREN_MAXW*FONTREN_MAXH*4, NULL, GL_STREAM_DRAW);

	// the written texture zero padded to avoid bits of previously drawn text bleeding in
	int winw = surf->w+1;
	int winh = surf->h+1;

	uint32_t *pixels = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	for(int y = 0; y < surf->h; y++) {
		memcpy(pixels+y*winw, ((uint8_t *)surf->pixels)+y*surf->pitch, surf->w*4);
		pixels[y*winw+surf->w]=0;
	}
	memset(pixels+(winh-1)*winw,0,winw*4);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,winw,winh,GL_RGBA,GL_UNSIGNED_BYTE,0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);


	SDL_FreeSurface(surf);
}


void init_fonts(void) {
	TTF_Init();
	fontrenderer_init(&resources.fontren);
	_fonts.standard = load_font("gfx/LinBiolinum.ttf", 20);
	_fonts.mainmenu = load_font("gfx/immortal.ttf", 35);

}

void free_fonts(void) {
	fontrenderer_free(&resources.fontren);
	TTF_CloseFont(_fonts.standard);
	TTF_CloseFont(_fonts.mainmenu);

	TTF_Quit();
}

void draw_text(Alignment align, float x, float y, const char *text, TTF_Font *font) {
	char *nl;
	char *buf = malloc(strlen(text)+1);
	strcpy(buf, text);

	if((nl = strchr(buf, '\n')) != NULL && strlen(nl) > 1) {
		draw_text(align, x, y + 20, nl+1, font);
		*nl = '\0';
	}

	fontrenderer_draw(&resources.fontren, buf, font);
	Texture *tex = &resources.fontren.tex;

	switch(align) {
	case AL_Center:
		break;
	case AL_Left:
		x += tex->w/2;
		break;
	case AL_Right:
		x -= tex->w/2;
		break;
	}

	// if textures are odd pixeled, align them for ideal sharpness.
	if(tex->w&1)
		x += 0.5;
	if(tex->h&1)
		y += 0.5;

	glEnable(GL_TEXTURE_2D);
	draw_texture_p(x, y, tex);

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
