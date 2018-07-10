/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "font.h"
#include "util.h"
#include "util/rectpack.h"
#include "config.h"
#include "video.h"
#include "events.h"
#include "renderer/api.h"

static void init_fonts(void);
static void post_init_fonts(void);
static void shutdown_fonts(void);
static char* font_path(const char*);
static bool check_font_path(const char*);
static void* load_font_begin(const char*, uint);
static void* load_font_end(void*, const char*, uint);
static void unload_font(void*);

ResourceHandler font_res_handler = {
	.type = RES_FONT,
	.typename = "font",
	.subdir = FONT_PATH_PREFIX,

	.procs = {
		.init = init_fonts,
		.post_init = post_init_fonts,
		.shutdown = shutdown_fonts,
		.find = font_path,
		.check = check_font_path,
		.begin_load = load_font_begin,
		.end_load = load_font_end,
		.unload = unload_font,
	},
};

// Handy routines for converting from fixed point
// From SDL_ttf
#define FT_FLOOR(X) (((X) & -64) / 64)
#define FT_CEIL(X)  ((((X) + 63) & -64) / 64)

#undef FTERRORS_H_
#define FT_ERRORDEF(e, v, s)  { e, s },
#undef FT_ERROR_START_LIST
#undef FT_ERROR_END_LIST

static const struct ft_error_def {
	FT_Error    err_code;
	const char *err_msg;
} ft_errors[] = {
	#include FT_ERRORS_H
	{ 0, NULL }
};

static const char* ft_error_str(FT_Error err_code) {
	for(const struct ft_error_def *e = ft_errors; e->err_msg; ++e) {
		if(e->err_code == err_code) {
			return e->err_msg;
		}
	}

	return "Unknown error";
}

typedef struct Glyph {
	Sprite sprite;
	GlyphMetrics metrics;
	ulong ft_index;
} Glyph;

typedef struct SpriteSheet {
	LIST_INTERFACE(struct SpriteSheet);
	Texture tex;
	RectPack *rectpack;
	uint glyphs;
} SpriteSheet;

typedef LIST_ANCHOR(SpriteSheet) SpriteSheetAnchor;

struct Font {
	char *source_path;
	Glyph *glyphs;
	SpriteSheetAnchor spritesheets;
	FT_Face face;
	long base_face_idx;
	int base_size;
	uint glyphs_allocated;
	uint glyphs_used;
	ht_int2int_t charcodes_to_glyph_ofs;
	ht_int2int_t ftindex_to_glyph_ofs;
	FontMetrics metrics;
};

static struct {
	FT_Library lib;
	ShaderProgram *default_shader;
	Texture render_tex;
	Framebuffer render_buf;

	struct {
		SDL_mutex *new_face;
		SDL_mutex *done_face;
	} mutex;
} globals;

static double global_font_scale(void) {
	int w, h;
	video_get_viewport_size(&w, &h);
	return sanitize_scale(((double)h / SCREEN_H) * config_get_float(CONFIG_TEXT_QUALITY));
}

static void reload_fonts(double quality);

static bool fonts_event(SDL_Event *event, void *arg) {
	if(!IS_TAISEI_EVENT(event->type)) {
		return false;
	}

	switch(TAISEI_EVENT(event->type)) {
		case TE_VIDEO_MODE_CHANGED: {
			reload_fonts(global_font_scale());
			break;
		}

		case TE_CONFIG_UPDATED: {
			if(event->user.code == CONFIG_TEXT_QUALITY) {
				ConfigValue *val = event->user.data1;
				val->f = sanitize_scale(val->f);
				reload_fonts(global_font_scale());
			}

			return false;
		}
	}

	return false;
}

static void try_create_mutex(SDL_mutex **mtx) {
	if((*mtx = SDL_CreateMutex()) == NULL) {
		log_sdl_error("SDL_CreateMutex");
	}
}

static void init_fonts(void) {
	FT_Error err;

	try_create_mutex(&globals.mutex.new_face);
	try_create_mutex(&globals.mutex.done_face);

	if((err = FT_Init_FreeType(&globals.lib))) {
		log_fatal("FT_Init_FreeType() failed: %s", ft_error_str(err));
	}

	events_register_handler(&(EventHandler) {
		fonts_event, NULL, EPRIO_SYSTEM,
	});

	preload_resources(RES_FONT, RESF_PERMANENT,
		"standard",
	NULL);

	// WARNING: Preloading the default shader here is unsafe.

	r_texture_create(&globals.render_tex, &(TextureParams) {
		.filter = { TEX_FILTER_LINEAR, TEX_FILTER_LINEAR },
		.wrap   = { TEX_WRAP_CLAMP, TEX_WRAP_CLAMP },
		.type   = TEX_TYPE_R,
		.stream = true,
		.width  = 1024,
		.height = 1024,
	});

	r_framebuffer_create(&globals.render_buf);
	r_framebuffer_attach(&globals.render_buf, &globals.render_tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_viewport(&globals.render_buf, 0, 0, globals.render_tex.w, globals.render_tex.h);
}

static void post_init_fonts(void) {
	globals.default_shader = get_resource_data(RES_SHADER_PROGRAM, "text_default", RESF_PERMANENT | RESF_PRELOAD);
}

static void shutdown_fonts(void) {
	r_texture_destroy(&globals.render_tex);
	r_framebuffer_destroy(&globals.render_buf);
	events_unregister_handler(fonts_event);
	FT_Done_FreeType(globals.lib);
	SDL_DestroyMutex(globals.mutex.new_face);
	SDL_DestroyMutex(globals.mutex.done_face);
}

static char* font_path(const char *name) {
	return strjoin(FONT_PATH_PREFIX, name, FONT_EXTENSION, NULL);
}

bool check_font_path(const char *path) {
	return strstartswith(path, FONT_PATH_PREFIX) && strendswith(path, FONT_EXTENSION);
}

static ulong ftstream_read(FT_Stream stream, ulong offset, uchar *buffer, ulong count) {
	SDL_RWops *rwops = stream->descriptor.pointer;
	ulong error = count ? 0 : 1;

	if(SDL_RWseek(rwops, offset, RW_SEEK_SET) < 0) {
		log_warn("Can't seek in stream (%s)", (char*)stream->pathname.pointer);
		return error;
	}

	return SDL_RWread(rwops, buffer, 1, count);
}

static void ftstream_close(FT_Stream stream) {
	log_debug("%s", (char*)stream->pathname.pointer);
	SDL_RWclose((SDL_RWops*)stream->descriptor.pointer);
}

static FT_Error FT_Open_Face_Thread_Safe(FT_Library library, const FT_Open_Args *args, FT_Long face_index, FT_Face *aface) {
	FT_Error err;
	SDL_LockMutex(globals.mutex.new_face);
	err = FT_Open_Face(library, args, face_index, aface);
	SDL_UnlockMutex(globals.mutex.new_face);
	return err;
}

static FT_Error FT_Done_Face_Thread_Safe(FT_Face face) {
	FT_Error err;
	SDL_LockMutex(globals.mutex.done_face);
	err = FT_Done_Face(face);
	SDL_UnlockMutex(globals.mutex.done_face);
	return err;
}

static FT_Face load_font_face(char *vfspath, long index) {
	char *syspath = vfs_repr(vfspath, true);

	SDL_RWops *rwops = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rwops) {
		log_warn("VFS error: %s", vfs_get_error());
		free(syspath);
		return NULL;
	}

	FT_Stream ftstream = calloc(1, sizeof(*ftstream));
	ftstream->descriptor.pointer = rwops;
	ftstream->pathname.pointer = syspath;
	ftstream->read = ftstream_read;
	ftstream->close = ftstream_close;
	ftstream->size = SDL_RWsize(rwops);

	FT_Open_Args ftargs;
	memset(&ftargs, 0, sizeof(ftargs));
	ftargs.flags = FT_OPEN_STREAM;
	ftargs.stream = ftstream;

	FT_Face face;
	FT_Error err;

	if((err = FT_Open_Face_Thread_Safe(globals.lib, &ftargs, index, &face))) {
		log_warn("Failed to load font '%s' (face %li): FT_Open_Face() failed: %s", syspath, index, ft_error_str(err));
		free(syspath);
		free(ftstream);
		return NULL;
	}

	if(!(FT_IS_SCALABLE(face))) {
		log_warn("Font '%s' (face %li) is not scalable. This is not supported, sorry. Load aborted", syspath, index);
		FT_Done_Face_Thread_Safe(face);
		free(syspath);
		free(ftstream);
		return NULL;
	}

	log_info("Loaded font '%s' (face %li)", syspath, index);

	return face;
}

static FT_Error set_font_size(Font *fnt, uint pxsize, double scale) {
	FT_Error err = FT_Err_Ok;

	assert(fnt != NULL);
	assert(fnt->face != NULL);

	FT_Fixed fixed_scale = round(scale * (2 << 15));
	pxsize = FT_MulFix(pxsize * 64, fixed_scale);

	if((err = FT_Set_Char_Size(fnt->face, 0, pxsize, 0, 0))) {
		log_warn("FT_Set_Char_Size(%u) failed: %s", pxsize, ft_error_str(err));
		return err;
	}

	// Based on SDL_ttf
	FT_Face face = fnt->face;
	fixed_scale = face->size->metrics.y_scale;
	fnt->metrics.ascent = FT_CEIL(FT_MulFix(face->ascender, fixed_scale));
	fnt->metrics.descent = FT_CEIL(FT_MulFix(face->descender, fixed_scale));
	fnt->metrics.max_glyph_height = fnt->metrics.ascent - fnt->metrics.descent;
	fnt->metrics.lineskip = FT_CEIL(FT_MulFix(face->height, fixed_scale));
	fnt->metrics.scale = scale;

	return err;
}

// TODO: Figure out sensible values for these; maybe make them depend on font size in some way.
#define SS_WIDTH 1024
#define SS_HEIGHT 1024

static SpriteSheet* add_spritesheet(SpriteSheetAnchor *spritesheets) {
	SpriteSheet *ss = calloc(1, sizeof(SpriteSheet));
	ss->rectpack = rectpack_new(SS_WIDTH, SS_HEIGHT);

	r_texture_create(&ss->tex, &(TextureParams) {
		.width = SS_WIDTH,
		.height = SS_HEIGHT,
		.type = TEX_TYPE_R,
		.filter.mag = TEX_FILTER_LINEAR,
		.filter.min = TEX_FILTER_LINEAR,
		.wrap.s = TEX_WRAP_CLAMP,
		.wrap.t = TEX_WRAP_CLAMP,
	});

	// FIXME:
	//
	// This code just zeroes the texture out.
	//
	// We should add an r_texture_clear function to hide this monstrosity.
	// It would also allow a non-GL backend to have a different implementation,
	// if we ever get around to writing one.
	//
	// To future generations: if such a function is already in the renderer API,
	// but this crap is still here, please convert it.
	Framebuffer *prev_fb = r_framebuffer_current();
	Framebuffer atlast_fb;
	Color cc_prev = r_clear_color_current();
	r_framebuffer_create(&atlast_fb);
	r_framebuffer_attach(&atlast_fb, &ss->tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer(&atlast_fb);
	r_clear_color4(0, 0, 0, 0);
	r_clear(CLEAR_COLOR);
	r_framebuffer(prev_fb);
	r_clear_color(cc_prev);
	r_framebuffer_destroy(&atlast_fb);

	alist_append(spritesheets, ss);
	return ss;
}

// The padding is needed to prevent glyph edges from bleeding in due to linear filtering.
#define GLYPH_SPRITE_PADDING 1

static bool add_glyph_to_spritesheet(Font *font, Glyph *glyph, FT_Bitmap bitmap, SpriteSheet *ss) {
	uint padded_w = bitmap.width + 2 * GLYPH_SPRITE_PADDING;
	uint padded_h = bitmap.rows + 2 * GLYPH_SPRITE_PADDING;
	Rect sprite_pos;

	if(!rectpack_add(ss->rectpack, padded_w, padded_h, &sprite_pos)) {
		return false;
	}

	complex ofs = GLYPH_SPRITE_PADDING * (1+I);
	sprite_pos.bottom_right += ofs;
	sprite_pos.top_left += ofs;

	r_texture_fill_region(
		&ss->tex,
		0,
		rect_x(sprite_pos),
		rect_y(sprite_pos),
		bitmap.width,
		bitmap.rows,
		bitmap.buffer
	);

	glyph->sprite.tex = &ss->tex;
	glyph->sprite.w = glyph->metrics.width; // bitmap.width / font->scale;
	glyph->sprite.h = glyph->metrics.height; // bitmap.rows / font->scale;
	glyph->sprite.tex_area.x = rect_x(sprite_pos);
	glyph->sprite.tex_area.y = rect_y(sprite_pos);
	glyph->sprite.tex_area.w = bitmap.width;
	glyph->sprite.tex_area.h = bitmap.rows;

	++ss->glyphs;

	return true;
}

static bool add_glyph_to_spritesheets(Font *font, Glyph *glyph, FT_Bitmap bitmap, SpriteSheetAnchor *spritesheets) {
	bool result;

	for(SpriteSheet *ss = spritesheets->first; ss; ss = ss->next) {
		if((result = add_glyph_to_spritesheet(font, glyph, bitmap, ss))) {
			return result;
		}
	}

	return add_glyph_to_spritesheet(font, glyph, bitmap, add_spritesheet(spritesheets));
}

static const char *const pixmode_name(FT_Pixel_Mode mode) {
	switch(mode) {
		case FT_PIXEL_MODE_NONE   : return "FT_PIXEL_MODE_NONE";
		case FT_PIXEL_MODE_MONO   : return "FT_PIXEL_MODE_MONO";
		case FT_PIXEL_MODE_GRAY   : return "FT_PIXEL_MODE_GRAY";
		case FT_PIXEL_MODE_GRAY2  : return "FT_PIXEL_MODE_GRAY2";
		case FT_PIXEL_MODE_GRAY4  : return "FT_PIXEL_MODE_GRAY4";
		case FT_PIXEL_MODE_LCD    : return "FT_PIXEL_MODE_LCD";
		case FT_PIXEL_MODE_LCD_V  : return "FT_PIXEL_MODE_LCD_V";
		// case FT_PIXEL_MODE_RGBA: return "FT_PIXEL_MODE_RGBA";
		default                   : return "FT_PIXEL_MODE_UNKNOWN";
	}
}

static void delete_spritesheet(SpriteSheetAnchor *spritesheets, SpriteSheet *ss) {
	r_texture_destroy(&ss->tex);
	rectpack_free(ss->rectpack);
	alist_unlink(spritesheets, ss);
	free(ss);
}

static Glyph* load_glyph(Font *font, FT_UInt gindex, SpriteSheetAnchor *spritesheets) {
	log_debug("Loading glyph 0x%08x", gindex);

	if(++font->glyphs_used == font->glyphs_allocated) {
		font->glyphs_allocated *= 2;
		font->glyphs = realloc(font->glyphs, sizeof(Glyph) * font->glyphs_allocated);
	}

	Glyph *glyph = font->glyphs + font->glyphs_used - 1;

	FT_Error err = FT_Load_Glyph(font->face, gindex, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT);

	if(err) {
		log_warn("FT_Load_Glyph(%u) failed: %s", gindex, ft_error_str(err));
		--font->glyphs_used;
		return NULL;
	}

	glyph->metrics.bearing_x = FT_FLOOR(font->face->glyph->metrics.horiBearingX);
	glyph->metrics.bearing_y = FT_FLOOR(font->face->glyph->metrics.horiBearingY);
	glyph->metrics.width = FT_CEIL(font->face->glyph->metrics.width);
	glyph->metrics.height = FT_CEIL(font->face->glyph->metrics.height);
	glyph->metrics.advance = FT_CEIL(font->face->glyph->metrics.horiAdvance);

	if(font->face->glyph->bitmap.buffer == NULL) {
		// Some glyphs may be invisible, but we still need the metrics data for them (e.g. space)
		memset(&glyph->sprite, 0, sizeof(Sprite));
	} else {
		if(font->face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
			log_warn(
				"Glyph %u returned bitmap with pixel format %s. Only %s is supported, sorry. Ignoring",
				gindex,
				pixmode_name(font->face->glyph->bitmap.pixel_mode),
				pixmode_name(FT_PIXEL_MODE_GRAY)
			);
			--font->glyphs_used;
			return NULL;
		}

		if(!add_glyph_to_spritesheets(font, glyph, font->face->glyph->bitmap, spritesheets)) {
			log_warn(
				"Glyph %u can't fit into any spritesheets (padded bitmap size: %ux%u; max spritesheet size: %ux%u)",
				gindex,
				font->face->glyph->bitmap.width + 2 * GLYPH_SPRITE_PADDING,
				font->face->glyph->bitmap.rows  + 2 * GLYPH_SPRITE_PADDING,
				SS_WIDTH,
				SS_HEIGHT
			);
			--font->glyphs_used;
			return NULL;
		}
	}

	glyph->ft_index = gindex;
	return glyph;
}

static Glyph* get_glyph(Font *fnt, charcode_t cp) {
	int64_t ofs;

	if(!ht_lookup(&fnt->charcodes_to_glyph_ofs, cp, &ofs)) {
		Glyph *glyph;
		uint ft_index = FT_Get_Char_Index(fnt->face, cp);
		log_debug("Glyph for charcode 0x%08lx not cached", cp);

		if(ft_index == 0 && cp != UNICODE_UNKNOWN) {
			log_debug("Font has no glyph for charcode 0x%08lx", cp);
			glyph = get_glyph(fnt, UNICODE_UNKNOWN);
			ofs = glyph ? (ptrdiff_t)(glyph - fnt->glyphs) : -1;
		} else {
			if(ht_lookup(&fnt->ftindex_to_glyph_ofs, ft_index, &ofs)) {
				glyph = fnt->glyphs + ofs;
			} else {
				glyph = load_glyph(fnt, ft_index, &fnt->spritesheets);
				ofs = glyph ? (ptrdiff_t)(glyph - fnt->glyphs) : -1;
				ht_set(&fnt->ftindex_to_glyph_ofs, ft_index, ofs);
			}
		}

		ht_set(&fnt->charcodes_to_glyph_ofs, cp, ofs);
	}

	return ofs < 0 ? NULL : fnt->glyphs + ofs;
}

attr_nonnull(1)
static void wipe_glyph_cache(Font *font) {
	ht_unset_all(&font->charcodes_to_glyph_ofs);
	ht_unset_all(&font->ftindex_to_glyph_ofs);

	for(SpriteSheet *ss = font->spritesheets.first, *next; ss; ss = next) {
		next = ss->next;
		delete_spritesheet(&font->spritesheets, ss);
	}

	font->glyphs_used = 0;
}

static void free_font_resources(Font *font) {
	if(font->face) {
		FT_Stream stream = font->face->stream;
		FT_Done_Face_Thread_Safe(font->face);

		if(stream) {
			free(stream->pathname.pointer);
			free(stream);
		}
	}

	wipe_glyph_cache(font);

	ht_destroy(&font->charcodes_to_glyph_ofs);
	ht_destroy(&font->ftindex_to_glyph_ofs);

	free(font->source_path);
	free(font->glyphs);
}

void* load_font_begin(const char *path, uint flags) {
	Font font;
	memset(&font, 0, sizeof(font));

	if(!parse_keyvalue_file_with_spec(path, (KVSpec[]){
		{ "source",  .out_str   = &font.source_path },
		{ "size",    .out_int   = &font.base_size },
		{ "face",    .out_long  = &font.base_face_idx },
		{ NULL }
	})) {
		log_warn("Failed to parse font file '%s'", path);
		return NULL;
	}

	ht_create(&font.charcodes_to_glyph_ofs);
	ht_create(&font.ftindex_to_glyph_ofs);

	if(!(font.face = load_font_face(font.source_path, font.base_face_idx))) {
		free_font_resources(&font);
		return NULL;
	}

	if(set_font_size(&font, font.base_size, global_font_scale())) {
		free_font_resources(&font);
		return NULL;
	}

	font.glyphs_allocated = 32;
	font.glyphs = calloc(font.glyphs_allocated, sizeof(Glyph));

	return memdup(&font, sizeof(font));
}

void* load_font_end(void *opaque, const char *path, uint flags) {
	Font *font = opaque;

	if(font == NULL) {
		return NULL;
	}

	return font;
}

void unload_font(void *vfont) {
	free_font_resources(vfont);
	free(vfont);
}

struct rlfonts_arg {
	double quality;
};

attr_nonnull(1)
static void reload_font(Font *font, double quality) {
	if(font->metrics.scale != quality) {
		wipe_glyph_cache(font);
		set_font_size(font, font->base_size, quality);
	}
}

static void* reload_font_callback(const char *name, Resource *res, void *varg) {
	struct rlfonts_arg *a = varg;
	reload_font((Font*)res->data, a->quality);
	return NULL;
}

static void reload_fonts(double quality) {
	resource_for_each(RES_FONT, reload_font_callback, &(struct rlfonts_arg) { quality });
}

static inline int apply_kerning(Font *font, uint prev_index, Glyph *gthis) {
	FT_Vector kvec;

	if(!FT_Get_Kerning(font->face, prev_index, gthis->ft_index, FT_KERNING_DEFAULT, &kvec)) {
		return kvec.x >> 6;
	}

	return 0;
}

int text_width_raw(Font *font, const char *text, uint maxlines) {
	const char *tptr = text;
	uint prev_glyph_idx = 0;
	bool keming = FT_HAS_KERNING(font->face);
	uint numlines = 0;
	int x = 0;
	int width = 0;

	while(*tptr) {
		uint32_t uchar = utf8_getch(&tptr);

		if(uchar == '\n') {
			if(++numlines == maxlines) {
				break;
			}

			if(x > width) {
				width = x;
			}

			x = 0;
			continue;
		}

		Glyph *glyph = get_glyph(font, uchar);

		if(glyph == NULL) {
			continue;
		}

		if(keming && prev_glyph_idx) {
			x += apply_kerning(font, prev_glyph_idx, glyph);
		}

		x += glyph->metrics.advance;
		prev_glyph_idx = glyph->ft_index;
	}

	if(x > width) {
		width = x;
	}

	return width;
}

void text_bbox(Font *font, const char *text, uint maxlines, BBox *bbox) {
	const char *tptr = text;
	uint prev_glyph_idx = 0;
	bool keming = FT_HAS_KERNING(font->face);
	uint numlines = 0;

	memset(bbox, 0, sizeof(*bbox));
	int x = 0, y = 0;

	while(*tptr) {
		uint32_t uchar = utf8_getch(&tptr);

		if(uchar == '\n') {
			if(++numlines == maxlines) {
				break;
			}

			x = 0;
			y += font->metrics.lineskip;

			continue;
		}

		Glyph *glyph = get_glyph(font, uchar);

		if(glyph == NULL) {
			continue;
		}

		if(keming && prev_glyph_idx) {
			x += apply_kerning(font, prev_glyph_idx, glyph);
		}

		int g_x0 = x + glyph->metrics.bearing_x;
		int g_x1 = g_x0 + glyph->metrics.width;

		bbox->x.max = max(bbox->x.max, g_x0);
		bbox->x.max = max(bbox->x.max, g_x1);
		bbox->x.min = min(bbox->x.min, g_x0);
		bbox->x.min = min(bbox->x.min, g_x1);

		int g_y0 = y - glyph->metrics.bearing_y;
		int g_y1 = g_y0 + glyph->metrics.height;

		bbox->y.max = max(bbox->y.max, g_y0);
		bbox->y.max = max(bbox->y.max, g_y1);
		bbox->y.min = min(bbox->y.min, g_y0);
		bbox->y.min = min(bbox->y.min, g_y1);

		prev_glyph_idx = glyph->ft_index;
		x += glyph->metrics.advance;
	}
}

double text_width(Font *font, const char *text, uint maxlines) {
	return text_width_raw(font, text, maxlines) / font->metrics.scale;
}

int text_height_raw(Font *font, const char *text, uint maxlines) {
	// FIXME: I'm not sure this is correct. Perhaps it should consider max_glyph_height at least?

	uint text_lines = 1;
	const char *tptr = text;

	while((tptr = strchr(tptr, '\n'))) {
		if(text_lines++ == maxlines) {
			break;
		}
	}

	return font->metrics.lineskip * text_lines;
}

double text_height(Font *font, const char *text, uint maxlines) {
	return text_height_raw(font, text, maxlines) / font->metrics.scale;
}

static inline void adjust_xpos(Font *font, const char *text, Alignment align, double x_orig, double *x) {
	double line_width;

	switch(align) {
		case ALIGN_LEFT: {
			*x = x_orig;
			break;
		}

		case ALIGN_RIGHT: {
			line_width = text_width_raw(font, text, 1);
			*x = x_orig - line_width;
			break;
		}

		case ALIGN_CENTER: {
			line_width = text_width_raw(font, text, 1);
			*x = x_orig - line_width * 0.5;
			break;
		}
	}
}

Font* get_font(const char *font) {
	return get_resource_data(RES_FONT, font, RESF_DEFAULT);
}

ShaderProgram* text_get_default_shader(void) {
	return globals.default_shader;
}

// #define TEXT_DRAW_BBOX

attr_returns_nonnull
static Font* font_from_params(const TextParams *params) {
	Font *font = params->font_ptr;

	if(font == NULL) {
		if(params->font != NULL) {
			font = get_font(params->font);
		} else {
			font = get_font("standard");
		}
	}

	assert(font != NULL);
	return font;
}

attr_nonnull(1, 2, 3)
static double _text_draw(Font *font, const char *text, const TextParams *params) {
	SpriteParams sp = { .sprite = NULL };
	BBox bbox;
	double x = params->pos.x;
	double y = params->pos.y;
	double iscale = 1 / font->metrics.scale;

	text_bbox(font, text, 0, &bbox);
	sp.shader_ptr = params->shader_ptr;

	if(sp.shader_ptr == NULL) {
		// don't defer this to r_draw_sprite; we don't want to call r_shader_get more than necessary.

		if(params->shader != NULL) {
			sp.shader_ptr = r_shader_get(params->shader);
		} else {
			sp.shader_ptr = r_shader_current();
		}
	}

	sp.color = params->color;
	sp.blend = params->blend;
	sp.custom = params->custom;

	if(sp.color == 0) {
		// XXX: sprite batch code defaults this to rgb(1, 1, 1)
		sp.color = r_color_current();
	}

	MatrixMode mm_prev = r_mat_mode_current();
	r_mat_mode(MM_MODELVIEW);
	r_mat_push();
	r_mat_translate(x, y, 0);
	r_mat_scale(iscale, iscale, 1);
	x = y = 0;

	double x_orig = x;
	adjust_xpos(font, text, params->align, x_orig, &x);

	double bbox_w = bbox.x.max - bbox.x.min;
	double bbox_h = bbox.y.max - bbox.y.min;

	#ifdef TEXT_DRAW_BBOX
	// TODO: align this correctly in the multi-line case
	double bbox_x_mid = x + bbox.x.min + bbox_w * 0.5;
	double bbox_y_mid = y + bbox.y.min - font->metrics.descent + bbox_h * 0.5;

	r_state_push();
	r_shader_standard_notex();
	r_mat_push();
	r_mat_translate(bbox_x_mid, bbox_y_mid, 0);
	r_mat_scale(bbox_w, bbox_h, 0);
	r_color(multiply_colors(rgba(1, 1, 1, 0.5), r_color_current()));
	r_draw_quad();
	r_mat_pop();
	r_state_pop();
	#endif

	r_mat_mode(MM_TEXTURE);
	r_mat_push();
	r_mat_translate(0, 0.0, 0);
	r_mat_scale(1/bbox_w, 1/bbox_h, 1.0);
	r_mat_translate(-bbox.x.min - (x - x_orig), -bbox.y.min + font->metrics.descent, 0);

	bool keming = FT_HAS_KERNING(font->face);
	uint prev_glyph_idx = 0;
	const char *tptr = text;

	while(*tptr) {
		uint32_t uchar = utf8_getch(&tptr);

		if(uchar == '\n') {
			adjust_xpos(font, tptr, params->align, x_orig, &x);
			y += font->metrics.lineskip;
			continue;
		}

		Glyph *glyph = get_glyph(font, uchar);

		if(glyph == NULL) {
			continue;
		}

		if(keming && prev_glyph_idx) {
			x += apply_kerning(font, prev_glyph_idx, glyph);
		}

		if(glyph->sprite.tex != NULL) {
			sp.sprite_ptr = &glyph->sprite;
			sp.pos.x = x + glyph->metrics.bearing_x + glyph->sprite.w * 0.5;
			sp.pos.y = y - glyph->metrics.bearing_y + glyph->sprite.h * 0.5 - font->metrics.descent;

			// HACK/FIXME: Glyphs have their sprite w/h unadjusted for scale.
			// We have to temporarily fix that up here so that the shader gets resolution-independent dimensions.
			float w_saved = sp.sprite_ptr->w;
			float h_saved = sp.sprite_ptr->h;
			sp.sprite_ptr->w /= font->metrics.scale;
			sp.sprite_ptr->h /= font->metrics.scale;
			sp.scale.both = font->metrics.scale;

			r_mat_push();
			r_mat_translate(sp.pos.x - x_orig, sp.pos.y, 0);
			r_mat_scale(w_saved, h_saved, 1.0);
			r_mat_translate(-0.5, -0.5, 0);

			if(params->glyph_callback.func != NULL) {
				params->glyph_callback.func(font, uchar, &sp, params->glyph_callback.userdata);
			}

			r_draw_sprite(&sp);

			// HACK/FIXME: See above.
			sp.sprite_ptr->w = w_saved;
			sp.sprite_ptr->h = h_saved;

			r_mat_pop();
		}

		x += glyph->metrics.advance;
		prev_glyph_idx = glyph->ft_index;
	}

	r_mat_pop();
	r_mat_mode(MM_MODELVIEW);
	r_mat_pop();
	r_mat_mode(mm_prev);

	return x_orig + (x - x_orig) / font->metrics.scale;
}

double text_draw(const char *text, const TextParams *params) {
	return _text_draw(font_from_params(params), text, params);
}

double text_draw_wrapped(const char *text, double max_width, const TextParams *params) {
	Font *font = font_from_params(params);
	char buf[strlen(text) * 2 + 1];
	text_wrap(font, text, max_width, buf, sizeof(buf));
	return _text_draw(font, buf, params);
}

void text_render(const char *text, Font *font, Sprite *out_sprite, BBox *out_bbox) {
	text_bbox(font, text, 0, out_bbox);

	int bbox_width = out_bbox->x.max - out_bbox->x.min;
	int bbox_height = out_bbox->y.max - out_bbox->y.min;

	if(bbox_height < font->metrics.max_glyph_height) {
		out_bbox->y.min -= font->metrics.max_glyph_height - bbox_height;
		bbox_height = out_bbox->y.max - out_bbox->y.min;
	}

	Texture *tex = &globals.render_tex;

	int tex_new_w = bbox_width; // max(tex->w, bbox_width);
	int tex_new_h = bbox_height; // max(tex->h, bbox_height);

	if(tex_new_w != tex->w || tex_new_h != tex->h) {
		log_info(
			"Resizing texture: %ix%i --> %ix%i",
			tex->w, tex->h,
			tex_new_w, tex_new_h
		);

		TextureParams params;
		r_texture_get_params(tex, &params);
		r_texture_destroy(tex);
		params.width = tex_new_w;
		params.height = tex_new_h;
		params.mipmaps = 0;
		r_texture_create(tex, &params);
		r_framebuffer_attach(&globals.render_buf, tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
		r_framebuffer_viewport(&globals.render_buf, 0, 0, tex_new_w, tex_new_h);
	}

	r_state_push();

	r_framebuffer(&globals.render_buf);
	r_clear_color4(0, 0, 0, 0);
	r_clear(CLEAR_COLOR);

	r_blend(BLEND_ALPHA);
	r_enable(RCAP_CULL_FACE);
	r_cull(CULL_FRONT);
	r_disable(RCAP_DEPTH_TEST);

	r_mat_mode(MM_MODELVIEW);
	r_mat_push();
	r_mat_identity();

	r_mat_mode(MM_PROJECTION);
	r_mat_push();
	r_mat_identity();
	// XXX: y-flipped because that's how our textures are...
	r_mat_ortho(0, tex->w, 0, tex->h, -100, 100);

	r_mat_mode(MM_TEXTURE);
	r_mat_push();
	r_mat_identity();

	// HACK: Coordinates are in texel space, font scale must not be used.
	// This probably should be exposed in the text_draw API.
	double fontscale = font->metrics.scale;
	font->metrics.scale = 1;

	text_draw(text, &(TextParams) {
		.font_ptr = font,
		.pos = { -out_bbox->x.min, -out_bbox->y.min + font->metrics.descent },
		.color = rgb(1, 1, 1),
		.shader = "text_default",
	});

	font->metrics.scale = fontscale;
	r_flush_sprites();

	// r_mat_mode(MM_TEXTURE);
	r_mat_pop();

	r_mat_mode(MM_PROJECTION);
	r_mat_pop();

	r_mat_mode(MM_MODELVIEW);
	r_mat_pop();

	r_state_pop();

	out_sprite->tex = tex;
	out_sprite->tex_area.w = bbox_width;
	out_sprite->tex_area.h = bbox_height;
	out_sprite->tex_area.x = 0;
	out_sprite->tex_area.y = 0;
	out_sprite->w = bbox_width / font->metrics.scale;
	out_sprite->h = bbox_height / font->metrics.scale;
}

void text_shorten(Font *font, char *text, double width) {
	// TODO: rewrite this to use utf8_getch

	assert(!strchr(text, '\n'));

	while(text_width(font, text, 0) > width) {
		int l = strlen(text);

		if(l <= 1) {
			return;
		}

		--l;
		text[l] = 0;

		for(int i = 0; i < min(3, l); ++i) {
			text[l - i - 1] = '.';
		}
	}
}

void text_wrap(Font *font, const char *src, double width, char *buf, size_t bufsize) {
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
			curwidth = text_width(font, curline, 0);
		} else {
			curwidth = 0;
		}

		char tmpbuf[strlen(curline) + strlen(next) + 2];
		strcpy(tmpbuf, curline);
		strcat(tmpbuf, " ");
		strcat(tmpbuf, next);

		double totalwidth = text_width(font, tmpbuf, 0);

		if(totalwidth > width) {
			if(curwidth == 0) {
				log_fatal(
					"Single word '%s' won't fit on one line. "
					"Word width: %g, max width: %g, source string: %s",
					next, text_width(font, tmpbuf, 0), width, src
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

const FontMetrics* font_get_metrics(Font *font) {
	return &font->metrics;
}

double font_get_lineskip(Font *font) {
	return font->metrics.lineskip / font->metrics.scale;
}

const GlyphMetrics* font_get_char_metrics(Font *font, charcode_t c) {
	Glyph *g = get_glyph(font, c);

	if(!g) {
		return NULL;
	}

	return &g->metrics;
}
