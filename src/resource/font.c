/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "font.h"
#include "global.h"
#include "util.h"
#include "objectpool.h"
#include "objectpool_util.h"

#define CACHE_EXPIRE_TIME 1000

#ifdef DEBUG
	// #define VERBOSE_CACHE_LOG
#endif

#ifdef VERBOSE_CACHE_LOG
	#define CACHELOG(fmt, ...) log_debug(fmt, __VA_ARGS__)
#else
	#define CACHELOG(fmt, ...)
#endif

typedef struct CacheEntry {
	OBJECT_INTERFACE(struct CacheEntry);

	SDL_Surface *surf;
	int width;
	int height;
	uint32_t ref_time;

	struct {
		 // to simplify invalidation
		Hashtable *ht;
		char *ht_key;
	} owner;
} CacheEntry;

typedef struct FontRenderer {
	Texture tex;
	float quality;
	uint32_t *pixbuf;
} FontRenderer;

static ObjectPool *cache_pool;
static CacheEntry *cache_entries;
static FontRenderer font_renderer;

struct Font {
	TTF_Font *ttf;
	Hashtable *cache;
};

struct Fonts _fonts;

static TTF_Font* load_ttf(char *vfspath, int size) {
	char *syspath = vfs_repr(vfspath, true);

	SDL_RWops *rwops = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rwops) {
		log_fatal("VFS error: %s", vfs_get_error());
	}

	// XXX: what would be the best rounding strategy here?
	size = rint(size * font_renderer.quality);

	TTF_Font *f = TTF_OpenFontRW(rwops, true, size);

	if(!f) {
		log_fatal("Failed to load font '%s' @ %i: %s", syspath, size, TTF_GetError());
	}

	log_info("Loaded '%s' @ %i", syspath, size);

	free(syspath);
	return f;
}

static Font* load_font(char *vfspath, int size) {
	TTF_Font *ttf = load_ttf(vfspath, size);

	Font *font = calloc(1, sizeof(Font));
	font->ttf = ttf;
	font->cache = hashtable_new_stringkeys(2048);

	return font;
}

static void free_cache_entry(CacheEntry *e) {
	if(!e) {
		return;
	}

	if(e->surf) {
		SDL_FreeSurface(e->surf);
	}

	CACHELOG("Wiping cache entry %p [%s]", (void*)e, e->owner.ht_key);

	free(e->owner.ht_key);
	list_unlink(&cache_entries, e);
	objpool_release(cache_pool, (ObjectInterface*)e);
}

static CacheEntry* get_cache_entry(Font *font, const char *text) {
	CacheEntry *e = hashtable_get_unsafe(font->cache, (void*)text);

	if(!e) {
		if(objpool_is_full(cache_pool)) {
			CacheEntry *oldest = cache_entries;

			for(CacheEntry *e = cache_entries->next; e; e = e->next) {
				if(e->ref_time < oldest->ref_time) {
					oldest = e;
				}
			}

			hashtable_unset_string(oldest->owner.ht, oldest->owner.ht_key);
			free_cache_entry(oldest);
		}

		e = (CacheEntry*)objpool_acquire(cache_pool);
		list_push(&cache_entries, e);
		hashtable_set_string(font->cache, text, e);
		e->owner.ht = font->cache;
		e->owner.ht_key = strdup(text);

		CACHELOG("New entry for text: [%s]", text);
	}

	e->ref_time = SDL_GetTicks();
	return e;
}

void update_font_cache(void) {
	uint32_t now = SDL_GetTicks();

	CacheEntry *next;
	for(CacheEntry *e = cache_entries; e; e = next) {
		next = e->next;

		if(now - e->ref_time > CACHE_EXPIRE_TIME) {
			hashtable_unset(e->owner.ht, e->owner.ht_key);
			free_cache_entry(e);
		}
	}
}

static void fontrenderer_init(FontRenderer *f, float quality) {
	f->quality = quality = sanitize_scale(quality);

	float r = ftopow2(quality);
	int w = FONTREN_MAXW * r;
	int h = FONTREN_MAXH * r;

	glGenTextures(1,&f->tex.gltex);

	f->tex.truew = w;
	f->tex.trueh = h;

	f->pixbuf = calloc(f->tex.truew, f->tex.trueh);

	glBindTexture(GL_TEXTURE_2D,f->tex.gltex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, f->tex.truew, f->tex.trueh, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	log_debug("q=%f, w=%i, h=%i", f->quality, f->tex.truew, f->tex.trueh);
}

static void fontrenderer_free(FontRenderer *f) {
	glDeleteTextures(1, &f->tex.gltex);

	free(f->pixbuf);
}

static void fontrenderer_draw_prerendered(FontRenderer *f, SDL_Surface *surf) {
	assert(surf != NULL);

	glBindTexture(GL_TEXTURE_2D, f->tex.gltex);

	f->tex.w = surf->w;
	f->tex.h = surf->h;

	// the written texture is zero padded to avoid bits of previously drawn text bleeding in
	int winw = surf->w+1;
	int winh = surf->h+1;

	uint32_t *pixels = f->pixbuf;

	for(int y = 0; y < surf->h; y++) {
		memcpy(pixels+y*winw, ((uint8_t *)surf->pixels)+y*surf->pitch, surf->w*4);
		pixels[y*winw+surf->w]=0;
	}

	memset(pixels+(winh-1)*winw,0,winw*4);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, winw, winh, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

static SDL_Surface* fontrender_render(FontRenderer *f, const char *text, Font *font) {
	CacheEntry *e = get_cache_entry(font, text);
	SDL_Surface *surf = e->surf;

	if(surf) {
		return surf;
	}

	CACHELOG("Rendering text: [%s]", text);
	surf = e->surf = TTF_RenderUTF8_Blended(font->ttf, text, (SDL_Color){255, 255, 255});

	if(!surf) {
		log_fatal("TTF_RenderUTF8_Blended() failed: %s", TTF_GetError());
	}

	if(surf->w > f->tex.truew || surf->h > f->tex.trueh) {
		log_fatal("Text (%s %dx%d) is too big for the internal buffer (%dx%d).", text, surf->w, surf->h, f->tex.truew, f->tex.trueh);
	}

	return surf;
}

static void fontrenderer_draw(FontRenderer *f, const char *text, Font *font) {
	SDL_Surface *surf = fontrender_render(f, text, font);
	fontrenderer_draw_prerendered(f, surf);
}

void init_fonts(void) {
	TTF_Init();
	memset(&font_renderer, 0, sizeof(font_renderer));
	cache_pool = OBJPOOL_ALLOC(CacheEntry, 512);
}

void uninit_fonts(void) {
	free_fonts();
	TTF_Quit();
}

void load_fonts(float quality) {
	fontrenderer_init(&font_renderer, quality);
	_fonts.standard  = load_font("res/fonts/LinBiolinum.ttf",           20);
	_fonts.mainmenu  = load_font("res/fonts/immortal.ttf",              35);
	_fonts.small     = load_font("res/fonts/LinBiolinum.ttf",           14);
	_fonts.hud       = load_font("res/fonts/Laconic_Regular.otf",       19);
	_fonts.mono      = load_font("res/fonts/ShareTechMono-Regular.ttf", 19);
	_fonts.monosmall = load_font("res/fonts/ShareTechMono-Regular.ttf", 14);
	_fonts.monotiny  = load_font("res/fonts/ShareTechMono-Regular.ttf", 10);
}

void reload_fonts(float quality) {
	if(!font_renderer.quality) {
		// never loaded
		load_fonts(quality);
		return;
	}

	if(font_renderer.quality != sanitize_scale(quality)) {
		free_fonts();
		load_fonts(quality);
	}
}

static void free_font(Font *font) {
	CacheEntry *e;
	TTF_CloseFont(font->ttf);

	for(HashtableIterator *i = hashtable_iter(font->cache); hashtable_iter_next(i, 0, (void**)&e);) {
		free_cache_entry(e);
	}

	hashtable_free(font->cache);
	free(font);
}

void free_fonts(void) {
	fontrenderer_free(&font_renderer);

	Font **last = &_fonts.first + (sizeof(_fonts)/sizeof(Font*) - 1);
	for(Font **font = &_fonts.first; font <= last; ++font) {
		free_font(*font);
	}
}

static void draw_text_texture(Alignment align, float x, float y, Texture *tex) {
	float m = 1.0 / font_renderer.quality;
	bool adjust = !(align & AL_Flag_NoAdjust);
	align &= 0xf;

	if(adjust) {
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
			default:
				log_fatal("Invalid alignment %x", align);
		}

		// if textures are odd pixeled, align them for ideal sharpness.

		if(tex->w&1) {
			x += 0.5;
		}

		if(tex->h&1) {
			y += 0.5;
		}
	} else {
		switch(align) {
			case AL_Center:
				break;
			case AL_Left:
				x += m*(tex->w/2.0);
				break;
			case AL_Right:
				x -= m*(tex->w/2.0);
				break;
			default:
				log_fatal("Invalid alignment %x", align);
		}
	}

	glPushMatrix();
	glTranslatef(x, y, 0);
	glScalef(m, m, 1);
	draw_texture_p(0, 0, tex);
	glPopMatrix();
}

void draw_text(Alignment align, float x, float y, const char *text, Font *font) {
	assert(text != NULL);

	if(!*text) {
		return;
	}

	char *nl;
	char *buf = malloc(strlen(text)+1);
	strcpy(buf, text);

	if((nl = strchr(buf, '\n')) != NULL && strlen(nl) > 1) {
		draw_text(align, x, y + 20, nl+1, font);
		*nl = '\0';
	}

	fontrenderer_draw(&font_renderer, buf, font);
	draw_text_texture(align, x, y, &font_renderer.tex);
	free(buf);
}

Texture* render_text(const char *text, Font *font) {
	fontrenderer_draw(&font_renderer, text, font);
	return &font_renderer.tex;
}

void draw_text_auto_wrapped(Alignment align, float x, float y, const char *text, int width, Font *font) {
	char buf[strlen(text) * 2];
	wrap_text(buf, sizeof(buf), text, width, font);
	draw_text(align, x, y, buf, font);
}

static void string_dimensions(char *s, Font *font, int *w, int *h) {
	CacheEntry *e = get_cache_entry(font, s);

	if(e->width <= 0 || e->height <= 0) {
		TTF_SizeUTF8(font->ttf, s, &e->width, &e->height);
		CACHELOG("Got size %ix%i for text: [%s]", e->width, e->height, s);
	}

	if(w) {
		*w = e->width;
	}

	if(h) {
		*h = e->height;
	}
}

int stringwidth(char *s, Font *font) {
	int w;
	string_dimensions(s, font, &w, NULL);
	return w / font_renderer.quality;
}

int stringheight(char *s, Font *font) {
	int h;
	string_dimensions(s, font, NULL, &h);
	return h / font_renderer.quality;
}

int charwidth(char c, Font *font) {
	char s[2];
	s[0] = c;
	s[1] = 0;
	return stringwidth(s, font);
}

int font_line_spacing(Font *font) {
	return TTF_FontLineSkip(font->ttf);
}

void shorten_text_up_to_width(char *s, float width, Font *font) {
	while(stringwidth(s, font) > width) {
		int l = strlen(s);

		if(l <= 1) {
			return;
		}

		--l;
		s[l] = 0;

		for(int i = 0; i < min(3, l); ++i) {
			s[l - i - 1] = '.';
		}
	}
}

void wrap_text(char *buf, size_t bufsize, const char *src, int width, Font *font) {
	assert(buf != NULL);
	assert(src != NULL);
	assert(font != NULL);
	assert(bufsize > strlen(src) + 1);
	assert(width > 0);

	char src_copy[strlen(src) + 1];
	char *sptr = src_copy;
	char *next = NULL;
	char *curline = buf;

	strcpy(src_copy, src);
	*buf = 0;

	while((next = strtok_r(NULL, " \t\n", &sptr))) {
		int curwidth;

		if(!*next) {
			continue;
		}

		if(*curline) {
			curwidth = stringwidth(curline, font);
		} else {
			curwidth = 0;
		}

		char tmpbuf[strlen(curline) + strlen(next) + 2];
		strcpy(tmpbuf, curline);
		strcat(tmpbuf, " ");
		strcat(tmpbuf, next);

		int totalwidth = stringwidth(tmpbuf, font);

		if(totalwidth > width) {
			if(curwidth == 0) {
				log_fatal(
					"Single word '%s' won't fit on one line. "
					"Word width: %i, max width: %i, source string: %s",
					next, stringwidth(next, font), width, src
				);
			}

			strlcat(buf, "\n", bufsize);
			curline = strchr(curline, 0);
		} else {
			if(*curline) {
				strlcat(buf, " ", bufsize);
			}

		}

		strlcat(buf, next, bufsize);
	}
}
