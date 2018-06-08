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
#include "video.h"

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
	Sprite sprite;
	float quality;
} FontRenderer;

static ObjectPool *cache_pool;
static CacheEntry *cache_entries;
static FontRenderer font_renderer;

struct Font {
	TTF_Font *ttf;
	Hashtable *cache;
	char *source_path;
	int base_size;
};

struct Fonts _fonts;

static void fontrenderer_init(float quality);
static void fontrenderer_free(void);
static void reload_fonts(float quality);

static bool fonts_event(SDL_Event *event, void *arg) {
	reload_fonts(video.quality_factor * config_get_float(CONFIG_TEXT_QUALITY));
	return false;
}

static void fonts_quality_config_callback(ConfigIndex idx, ConfigValue v) {
	config_set_float(idx, v.f);
	reload_fonts(video.quality_factor * v.f);
}

static void init_fonts(void) {
	TTF_Init();
	memset(&font_renderer, 0, sizeof(font_renderer));
	cache_pool = OBJPOOL_ALLOC(CacheEntry, 512);
	fontrenderer_init(video.quality_factor * config_get_float(CONFIG_TEXT_QUALITY));

	events_register_handler(&(EventHandler) {
		fonts_event, NULL, EPRIO_SYSTEM, MAKE_TAISEI_EVENT(TE_VIDEO_MODE_CHANGED)
	});

	config_set_callback(CONFIG_TEXT_QUALITY, fonts_quality_config_callback);

	preload_resources(RES_FONT, RESF_PERMANENT,
		"standard",
		"big",
		"small",
		"hud",
		"mono",
		"monosmall",
		"monotiny",
	NULL);

	_fonts.standard  = get_resource(RES_FONT, "standard", 0)->data;
	_fonts.mainmenu  = get_resource(RES_FONT, "big", 0)->data;
	_fonts.small     = get_resource(RES_FONT, "small", 0)->data;
	_fonts.hud       = get_resource(RES_FONT, "hud", 0)->data;
	_fonts.mono      = get_resource(RES_FONT, "mono", 0)->data;
	_fonts.monosmall = get_resource(RES_FONT, "monosmall", 0)->data;
	_fonts.monotiny  = get_resource(RES_FONT, "monotiny", 0)->data;
}

static void uninit_fonts(void) {
	events_unregister_handler(fonts_event);
	fontrenderer_free();
	TTF_Quit();
	objpool_free(cache_pool);
}

static char* font_path(const char *name) {
	return strjoin(FONT_PATH_PREFIX, name, FONT_EXTENSION, NULL);
}

bool check_font_path(const char *path) {
	return strstartswith(path, FONT_PATH_PREFIX) && strendswith(path, FONT_EXTENSION);
}

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
		log_warn("Failed to load TTF font '%s' @ %i: %s", syspath, size, TTF_GetError());
	}

	log_info("Loaded '%s' @ %i", syspath, size);

	free(syspath);
	return f;
}

void* load_font_begin(const char *path, uint flags) {
	Font font;
	memset(&font, 0, sizeof(font));

	if(!parse_keyvalue_file_with_spec(path, (KVSpec[]){
		{ "source",  .out_str   = &font.source_path },
		{ "size",    .out_int   = &font.base_size },
		{ NULL }
	})) {
		log_warn("Failed to parse font file '%s'", path);
		return NULL;
	}

	font.ttf = load_ttf(font.source_path, font.base_size);

	if(!font.ttf) {
		free(font.source_path);
		return NULL;
	}

	font.cache = hashtable_new_stringkeys();

	return memdup(&font, sizeof(font));
}

void* load_font_end(void *opaque, const char *path, uint flags) {
	return opaque;
}

static void free_font(Font *font);

void unload_font(void *vfont) {
	free_font(vfont);
}

ResourceHandler font_res_handler = {
	.type = RES_FONT,
	.typename = "font",
	.subdir = FONT_PATH_PREFIX,

	.procs = {
		.init = init_fonts,
		.shutdown = uninit_fonts,
		.find = font_path,
		.check = check_font_path,
		.begin_load = load_font_begin,
		.end_load = load_font_end,
		.unload = unload_font,
	},
};

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

static void fontrenderer_init_tex(Texture *tex) {
	r_texture_create(tex, &(TextureParams) {
		.type = TEX_TYPE_RGBA,
		.filter = {
			.upscale = TEX_FILTER_LINEAR,
			.downscale = TEX_FILTER_LINEAR,
		},
		.wrap = {
			.s = TEX_WRAP_CLAMP,
			.t = TEX_WRAP_CLAMP,
		},
		.width = 1,
		.height = 1,
		.stream = true,
	});
}

static void fontrenderer_init(float quality) {
	font_renderer.quality = quality = sanitize_scale(quality);
	fontrenderer_init_tex(&font_renderer.tex);
	font_renderer.sprite.tex = &font_renderer.tex;
}

static void fontrenderer_free(void) {
	r_texture_destroy(&font_renderer.tex);
}

static void fontrenderer_upload(SDL_Surface *surf) {
	assert(surf != NULL);

	r_texture_replace(&font_renderer.tex, font_renderer.tex.type, surf->w, surf->h, surf->pixels);

	font_renderer.sprite.tex_area.w = surf->w;
	font_renderer.sprite.tex_area.h = surf->h;
	font_renderer.sprite.w = font_renderer.sprite.tex_area.w / font_renderer.quality;
	font_renderer.sprite.h = font_renderer.sprite.tex_area.h / font_renderer.quality;
}

static SDL_Surface* fontrender_render(const char *text, Font *font) {
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

	return surf;
}

static void free_font_cache(Hashtable *cache) {
	CacheEntry *e;

	for(HashtableIterator *i = hashtable_iter(cache); hashtable_iter_next(i, 0, (void**)&e);) {
		free_cache_entry(e);
	}
}

static void free_font(Font *font) {
	TTF_CloseFont(font->ttf);
	free_font_cache(font->cache);
	hashtable_free(font->cache);
	free(font->source_path);
	free(font);
}

static void reload_font(Font *font) {
	TTF_CloseFont(font->ttf);
	free_font_cache(font->cache);
	hashtable_unset_all(font->cache);
	font->ttf = load_ttf(font->source_path, font->base_size);
}

static void* reload_font_callback(const char *name, Resource *res, void *varg) {
	reload_font((Font*)res->data);
	return NULL;
}

static void reload_fonts(float quality) {
	if(font_renderer.quality == sanitize_scale(quality)) {
		return;
	}

	fontrenderer_free();
	fontrenderer_init(quality);
	resource_for_each(RES_FONT, reload_font_callback, NULL);
}

static void draw_text_line(Alignment align, float x, float y, const char *text, Font *font) {
	float m = 1.0 / font_renderer.quality;
	bool adjust = !(align & AL_Flag_NoAdjust);
	align &= 0xf;

	fontrenderer_upload(fontrender_render(text, font));

	// XXX: all of these hacks are probably broken

	int w = font_renderer.sprite.tex_area.w;
	int h = font_renderer.sprite.tex_area.h;

	if(adjust) {
		switch(align) {
			case AL_Center:
				break;
			// w/2 is integer division and must be done first
			case AL_Left:
				x += m*(w/2);
				break;
			case AL_Right:
				x -= m*(w/2);
				break;
			default:
				log_fatal("Invalid alignment %x", align);
		}

		// if textures are odd pixeled, align them for ideal sharpness.

		if(w&1) {
			x += 0.5;
		}

		if(h&1) {
			y += 0.5;
		}
	} else {
		switch(align) {
			case AL_Center:
				break;
			case AL_Left:
				x += m*(w/2.0);
				break;
			case AL_Right:
				x -= m*(w/2.0);
				break;
			default:
				log_fatal("Invalid alignment %x", align);
		}
	}

	draw_sprite_p(x, y, &font_renderer.sprite);
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

	draw_text_line(align, x, y, buf, font);
	free(buf);
}

void render_text(const char *text, Font *font, Sprite *out_spr) {
	fontrenderer_upload(fontrender_render(text, font));
	memcpy(out_spr, &font_renderer.sprite, sizeof(Sprite));
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
	return TTF_FontLineSkip(font->ttf) / font_renderer.quality;
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
