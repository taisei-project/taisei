/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "font.h"
#include "global.h"

struct Fonts _fonts;

TTF_Font* load_font(char *vfspath, int size) {
	char *syspath = vfs_repr(vfspath, true);

	SDL_RWops *rwops = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rwops) {
		log_fatal("VFS error: %s", vfs_get_error());
	}

	// XXX: what would be the best rounding strategy here?
	size = rint(size * resources.fontren.quality);

	TTF_Font *f = TTF_OpenFontRW(rwops, true, size);

	if(!f) {
		log_fatal("Failed to load font '%s' @ %i: %s", syspath, size, TTF_GetError());
	}

	log_info("Loaded '%s' @ %i", syspath, size);

	free(syspath);
	return f;
}

static float sanitize_scale(float scale) {
	return max(0.1, scale);
}

void fontrenderer_init(FontRenderer *f, float quality) {
	f->quality = quality = sanitize_scale(quality);

	float r = ftopow2(quality);
	int w = FONTREN_MAXW * r;
	int h = FONTREN_MAXH * r;

	glGenBuffers(1,&f->pbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, f->pbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, w*h*4, NULL, GL_STREAM_DRAW);
	glGenTextures(1,&f->tex.gltex);

	f->tex.truew = w;
	f->tex.trueh = h;

	glBindTexture(GL_TEXTURE_2D,f->tex.gltex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, f->tex.truew, f->tex.trueh, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	log_debug("q=%f, w=%i, h=%i", f->quality, f->tex.truew, f->tex.trueh);
}

void fontrenderer_free(FontRenderer *f) {
	glDeleteBuffers(1,&f->pbo);
	glDeleteTextures(1,&f->tex.gltex);
}

void fontrenderer_draw_prerendered(FontRenderer *f, SDL_Surface *surf) {
	assert(surf != NULL);

	f->tex.w = surf->w;
	f->tex.h = surf->h;

	glBindTexture(GL_TEXTURE_2D,f->tex.gltex);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, f->pbo);

	glBufferData(GL_PIXEL_UNPACK_BUFFER, f->tex.truew*f->tex.trueh*4, NULL, GL_STREAM_DRAW);

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
}

SDL_Surface* fontrender_render(FontRenderer *f, const char *text, TTF_Font *font) {
	SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, (SDL_Color){255, 255, 255});

	if(!surf) {
		log_fatal("TTF_RenderUTF8_Blended() failed: %s", TTF_GetError());
	}

	if(surf->w > f->tex.truew || surf->h > f->tex.trueh) {
		log_fatal("Text (%s %dx%d) is too big for the internal buffer (%dx%d).", text, surf->w, surf->h, f->tex.truew, f->tex.trueh);
	}

	return surf;
}

void fontrenderer_draw(FontRenderer *f, const char *text, TTF_Font *font) {
	SDL_Surface *surf = fontrender_render(f, text, font);
	fontrenderer_draw_prerendered(f, surf);
	SDL_FreeSurface(surf);
}

void init_fonts(void) {
	TTF_Init();
	memset(&resources.fontren, 0, sizeof(resources.fontren));
}

void uninit_fonts(void) {
	free_fonts();
	TTF_Quit();
}

void load_fonts(float quality) {
	fontrenderer_init(&resources.fontren, quality);
	_fonts.standard = load_font("res/gfx/LinBiolinum.ttf", 20);
	_fonts.mainmenu = load_font("res/gfx/immortal.ttf", 35);
	_fonts.small    = load_font("res/gfx/LinBiolinum.ttf", 14);
}

void reload_fonts(float quality) {
	if(!resources.fontren.quality) {
		// never loaded
		load_fonts(quality);
		return;
	}

	if(resources.fontren.quality != sanitize_scale(quality)) {
		free_fonts();
		load_fonts(quality);
	}
}

void free_fonts(void) {
	fontrenderer_free(&resources.fontren);
	TTF_CloseFont(_fonts.standard);
	TTF_CloseFont(_fonts.mainmenu);
	TTF_CloseFont(_fonts.small);
}

static void draw_text_texture(Alignment align, float x, float y, Texture *tex) {
	float m = 1.0 / resources.fontren.quality;

	switch(align) {
	case AL_Center:
		break;

	// tex->w/2 is integer division and must be done first
	case AL_Left:
		x += m*(tex->w/2);
		break;
	case AL_Right:
		x -= m*(tex->w/2);
		break;
	}

	// if textures are odd pixeled, align them for ideal sharpness.
	if(tex->w&1)
		x += 0.5;
	if(tex->h&1)
		y += 0.5;

	glEnable(GL_TEXTURE_2D);

	glPushMatrix();
	glTranslatef(x, y, 0);
	glScalef(m, m, 1);
	draw_texture_p(0, 0, tex);
	glPopMatrix();
}

void draw_text_prerendered(Alignment align, float x, float y, SDL_Surface *surf) {
	fontrenderer_draw_prerendered(&resources.fontren, surf);
	draw_text_texture(align, x, y, &resources.fontren.tex);
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
	draw_text_texture(align, x, y, &resources.fontren.tex);
	free(buf);
}

int stringwidth(char *s, TTF_Font *font) {
	int w;
	TTF_SizeUTF8(font, s, &w, NULL);
	return w / resources.fontren.quality;
}

int stringheight(char *s, TTF_Font *font) {
	int h;
	TTF_SizeUTF8(font, s, NULL, &h);
	return h / resources.fontren.quality;
}

int charwidth(char c, TTF_Font *font) {
	char s[2];
	s[0] = c;
	s[1] = 0;
	return stringwidth(s, font);
}

void shorten_text_up_to_width(char *s, float width, TTF_Font *font) {
	while(stringwidth(s, font) > width) {
		s[strlen(s) - 1] = 0;

		int l = strlen(s);
		for(int i = 0; i < 3; ++i) {
			s[l - i - 1] = '.';
		}
	}
}
