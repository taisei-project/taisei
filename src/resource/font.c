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
#include FT_STROKER_H

#include "font.h"
#include "util.h"
#include "util/rectpack.h"
#include "util/graphics.h"
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
	Sprite outline_sprite;
	GlyphMetrics metrics;
	ulong ft_index;
} Glyph;

typedef struct SpriteSheet {
	LIST_INTERFACE(struct SpriteSheet);
	Texture *tex;
	RectPack *rectpack;
	uint glyphs;
} SpriteSheet;

typedef LIST_ANCHOR(SpriteSheet) SpriteSheetAnchor;

struct Font {
	char *source_path;
	Glyph *glyphs;
	SpriteSheetAnchor spritesheets;
	FT_Face face;
	FT_Stroker stroker;
	long base_face_idx;
	int base_size;
	uint glyphs_allocated;
	uint glyphs_used;
	ht_int2int_t charcodes_to_glyph_ofs;
	ht_int2int_t ftindex_to_glyph_ofs;
	FontMetrics metrics;

#ifdef DEBUG
	char debug_label[64];
#endif
};

static struct {
	FT_Library lib;
	ShaderProgram *default_shader;
	Texture *render_tex;
	Framebuffer *render_buf;

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

	globals.render_tex = r_texture_create(&(TextureParams) {
		.filter = { TEX_FILTER_LINEAR, TEX_FILTER_LINEAR },
		.wrap   = { TEX_WRAP_CLAMP, TEX_WRAP_CLAMP },
		.type   = TEX_TYPE_RGBA,
		.stream = true,
		.width  = 1024,
		.height = 1024,
	});

	#ifdef DEBUG
	char buf[128];
	snprintf(buf, sizeof(buf), "Font render texture");
	r_texture_set_debug_label(globals.render_tex, buf);
	#endif

	globals.render_buf = r_framebuffer_create();
	r_framebuffer_attach(globals.render_buf, globals.render_tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_viewport(globals.render_buf, 0, 0, 1024, 1024);
}

static void post_init_fonts(void) {
	globals.default_shader = get_resource_data(RES_SHADER_PROGRAM, "text_default", RESF_PERMANENT | RESF_PRELOAD);
}

static void shutdown_fonts(void) {
	r_texture_destroy(globals.render_tex);
	r_framebuffer_destroy(globals.render_buf);
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

	if((err = FT_Stroker_New(globals.lib, &fnt->stroker))) {
		log_warn("FT_Stroker_New() failed: %s", ft_error_str(err));
		return err;
	}

	FT_Stroker_Set(fnt->stroker, FT_MulFix(1 * 64, fixed_scale), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

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

static SpriteSheet* add_spritesheet(Font *font, SpriteSheetAnchor *spritesheets) {
	SpriteSheet *ss = calloc(1, sizeof(SpriteSheet));
	ss->rectpack = rectpack_new(SS_WIDTH, SS_HEIGHT);

	ss->tex = r_texture_create(&(TextureParams) {
		.width = SS_WIDTH,
		.height = SS_HEIGHT,
		.type = TEX_TYPE_RGB_8,
		.filter.mag = TEX_FILTER_LINEAR,
		.filter.min = TEX_FILTER_LINEAR,
		.wrap.s = TEX_WRAP_CLAMP,
		.wrap.t = TEX_WRAP_CLAMP,
		.mipmaps = TEX_MIPMAPS_MAX,
		.mipmap_mode = TEX_MIPMAP_AUTO,
	});

#ifdef DEBUG
	char buf[128];
	snprintf(buf, sizeof(buf), "Font %s spritesheet", font->debug_label);
	r_texture_set_debug_label(ss->tex, buf);
#endif

	r_texture_clear(ss->tex, RGBA(0, 0, 0, 0));
	alist_append(spritesheets, ss);
	return ss;
}

// The padding is needed to prevent glyph edges from bleeding in due to linear filtering.
#define GLYPH_SPRITE_PADDING 1

static bool add_glyph_to_spritesheet(Font *font, Sprite *sprite, Pixmap *pixmap, SpriteSheet *ss) {
	uint padded_w = pixmap->width + 2 * GLYPH_SPRITE_PADDING;
	uint padded_h = pixmap->height + 2 * GLYPH_SPRITE_PADDING;
	Rect sprite_pos;

	if(!rectpack_add(ss->rectpack, padded_w, padded_h, &sprite_pos)) {
		return false;
	}

	complex ofs = GLYPH_SPRITE_PADDING * (1+I);
	sprite_pos.bottom_right += ofs;
	sprite_pos.top_left += ofs;

	r_texture_fill_region(
		ss->tex,
		0,
		rect_x(sprite_pos),
		rect_y(sprite_pos),
		pixmap
	);

	sprite->tex = ss->tex;
	sprite->w = pixmap->width;
	sprite->h = pixmap->height;
	sprite->tex_area.x = rect_x(sprite_pos);
	sprite->tex_area.y = rect_y(sprite_pos);
	sprite->tex_area.w = pixmap->width;
	sprite->tex_area.h = pixmap->height;

	++ss->glyphs;

	return true;
}

static bool add_glyph_to_spritesheets(Font *font, Sprite *sprite, Pixmap *pixmap, SpriteSheetAnchor *spritesheets) {
	bool result;

	for(SpriteSheet *ss = spritesheets->first; ss; ss = ss->next) {
		if((result = add_glyph_to_spritesheet(font, sprite, pixmap, ss))) {
			return result;
		}
	}

	return add_glyph_to_spritesheet(font, sprite, pixmap, add_spritesheet(font, spritesheets));
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
	r_texture_destroy(ss->tex);
	rectpack_free(ss->rectpack);
	alist_unlink(spritesheets, ss);
	free(ss);
}

static Glyph* load_glyph(Font *font, FT_UInt gindex, SpriteSheetAnchor *spritesheets) {
	// log_debug("Loading glyph 0x%08x", gindex);

	if(++font->glyphs_used == font->glyphs_allocated) {
		font->glyphs_allocated *= 2;
		font->glyphs = realloc(font->glyphs, sizeof(Glyph) * font->glyphs_allocated);
	}

	Glyph *glyph = font->glyphs + font->glyphs_used - 1;

	FT_Error err = FT_Load_Glyph(font->face, gindex, FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LIGHT);

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

	FT_Glyph g_src = NULL, g_fill = NULL, g_border = NULL, g_inner = NULL;
	FT_BitmapGlyph g_bm_fill = NULL, g_bm_border = NULL, g_bm_inner = NULL;
	FT_Get_Glyph(font->face->glyph, &g_src);
	FT_Glyph_Copy(g_src, &g_fill);
	FT_Glyph_Copy(g_src, &g_border);
	FT_Glyph_Copy(g_src, &g_inner);

	assert(g_src->format == FT_GLYPH_FORMAT_OUTLINE);

	bool have_bitmap = FT_Glyph_To_Bitmap(&g_fill, FT_RENDER_MODE_LIGHT, NULL, true) == FT_Err_Ok;

	if(have_bitmap) {
		have_bitmap = ((FT_BitmapGlyph)g_fill)->bitmap.width > 0;
	}

	if(!have_bitmap) {
		// Some glyphs may be invisible, but we still need the metrics data for them (e.g. space)
		memset(&glyph->sprite, 0, sizeof(Sprite));
	} else {
		FT_Glyph_StrokeBorder(&g_border, font->stroker, false, true);
		FT_Glyph_To_Bitmap(&g_border, FT_RENDER_MODE_LIGHT, NULL, true);

		FT_Glyph_StrokeBorder(&g_inner, font->stroker, true, true);
		FT_Glyph_To_Bitmap(&g_inner, FT_RENDER_MODE_LIGHT, NULL, true);

		g_bm_fill = (FT_BitmapGlyph)g_fill;
		g_bm_border = (FT_BitmapGlyph)g_border;
		g_bm_inner = (FT_BitmapGlyph)g_inner;

		if(
			g_bm_fill->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY ||
			g_bm_border->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY ||
			g_bm_inner->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY
		) {
			log_warn(
				"Glyph %u returned bitmap with pixel format %s. Only %s is supported, sorry. Ignoring",
				gindex,
				pixmode_name(g_bm_fill->bitmap.pixel_mode),
				pixmode_name(FT_PIXEL_MODE_GRAY)
			);

			FT_Done_Glyph(g_src);
			FT_Done_Glyph(g_fill);
			FT_Done_Glyph(g_border);
			FT_Done_Glyph(g_inner);
			--font->glyphs_used;
			return NULL;
		}

		Pixmap px;
		px.origin = PIXMAP_ORIGIN_TOPLEFT;
		px.format = PIXMAP_FORMAT_RGB8;
		px.width = imax(g_bm_fill->bitmap.width, imax(g_bm_border->bitmap.width, g_bm_inner->bitmap.width));
		px.height = imax(g_bm_fill->bitmap.rows, imax(g_bm_border->bitmap.rows, g_bm_inner->bitmap.rows));
		px.data.rg8 = pixmap_alloc_buffer_for_copy(&px);

		log_debug(
			"(%i %i) (%i %i) (%i %i)",
			g_bm_fill->top,
			g_bm_fill->left,
			g_bm_border->top,
			g_bm_border->left,
			g_bm_inner->top,
			g_bm_inner->left
		);

		int ref_left = g_bm_border->left;
		int ref_top = g_bm_border->top;

		ssize_t fill_ofs_x   =  (g_bm_fill->left    - ref_left);
		ssize_t fill_ofs_y   = -(g_bm_fill->top    - ref_top);
		ssize_t border_ofs_x =  (g_bm_border->left - ref_left);
		ssize_t border_ofs_y = -(g_bm_border->top  - ref_top);
		ssize_t inner_ofs_x  =  (g_bm_inner->left  - ref_left);
		ssize_t inner_ofs_y  = -(g_bm_inner->top   - ref_top);

		for(ssize_t x = 0; x < px.width; ++x) {
			for(ssize_t y = 0; y < px.height; ++y) {
				PixelRGB8 *p = px.data.rgb8 + (x + y * px.width);

				ssize_t fill_coord_x = x - fill_ofs_x;
				ssize_t fill_coord_y = y - fill_ofs_y;
				ssize_t border_coord_x = x - border_ofs_x;
				ssize_t border_coord_y = y - border_ofs_y;
				ssize_t inner_coord_x = x - inner_ofs_x;
				ssize_t inner_coord_y = y - inner_ofs_y;

				ssize_t fill_index = fill_coord_x + fill_coord_y * g_bm_fill->bitmap.pitch;
				ssize_t border_index = border_coord_x + border_coord_y * g_bm_border->bitmap.pitch;
				ssize_t inner_index = inner_coord_x + inner_coord_y * g_bm_inner->bitmap.pitch;

				if(
					fill_coord_x >= 0 && fill_coord_x < g_bm_fill->bitmap.width &&
					fill_coord_y >= 0 && fill_coord_y < g_bm_fill->bitmap.rows
				) {
					p->r = g_bm_fill->bitmap.buffer[fill_index];
				} else {
					p->r = 0;
				}

				if(
					border_coord_x >= 0 && border_coord_x < g_bm_border->bitmap.width &&
					border_coord_y >= 0 && border_coord_y < g_bm_border->bitmap.rows
				) {
					p->g = g_bm_border->bitmap.buffer[border_index];
				} else {
					p->g = 0;
				}

				if(
					inner_coord_x >= 0 && inner_coord_x < g_bm_inner->bitmap.width &&
					inner_coord_y >= 0 && inner_coord_y < g_bm_inner->bitmap.rows
				) {
					p->b = g_bm_inner->bitmap.buffer[inner_index];
				} else {
					p->b = 0;
				}
			}
		}

		if(!add_glyph_to_spritesheets(font, &glyph->sprite, &px, spritesheets)) {
			log_warn(
				"Glyph %u fill can't fit into any spritesheets (padded bitmap size: %zux%zu; max spritesheet size: %ux%u)",
				gindex,
				px.width + 2 * GLYPH_SPRITE_PADDING,
				px.height + 2 * GLYPH_SPRITE_PADDING,
				SS_WIDTH,
				SS_HEIGHT
			);

			free(px.data.rg8);
			FT_Done_Glyph(g_src);
			FT_Done_Glyph(g_fill);
			FT_Done_Glyph(g_border);
			FT_Done_Glyph(g_inner);
			--font->glyphs_used;
			return NULL;
		}

		free(px.data.rg8);
	}

	FT_Done_Glyph(g_src);
	FT_Done_Glyph(g_fill);
	FT_Done_Glyph(g_border);
	FT_Done_Glyph(g_inner);

	glyph->ft_index = gindex;
	return glyph;
}

static Glyph* get_glyph(Font *fnt, charcode_t cp) {
	int64_t ofs;

	if(!ht_lookup(&fnt->charcodes_to_glyph_ofs, cp, &ofs)) {
		Glyph *glyph;
		uint ft_index = FT_Get_Char_Index(fnt->face, cp);
		// log_debug("Glyph for charcode 0x%08lx not cached", cp);

		if(ft_index == 0 && cp != UNICODE_UNKNOWN) {
			log_debug("Font has no glyph for charcode 0x%08lx", cp);
			glyph = get_glyph(fnt, UNICODE_UNKNOWN);
			ofs = glyph ? (ptrdiff_t)(glyph - fnt->glyphs) : -1;
		} else if(!ht_lookup(&fnt->ftindex_to_glyph_ofs, ft_index, &ofs)) {
			glyph = load_glyph(fnt, ft_index, &fnt->spritesheets);
			ofs = glyph ? (ptrdiff_t)(glyph - fnt->glyphs) : -1;
			ht_set(&fnt->ftindex_to_glyph_ofs, ft_index, ofs);
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

	if(font->stroker) {
		FT_Stroker_Done(font->stroker);
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

#ifdef DEBUG
	char *basename = resource_util_basename(FONT_PATH_PREFIX, path);
	strlcpy(font.debug_label, basename, sizeof(font.debug_label));
#endif

	return memdup(&font, sizeof(font));
}

void* load_font_end(void *opaque, const char *path, uint flags) {
	return opaque;
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

int text_ucs4_width_raw(Font *font, const uint32_t *text, uint maxlines) {
	const uint32_t *tptr = text;
	uint prev_glyph_idx = 0;
	bool keming = FT_HAS_KERNING(font->face);
	uint numlines = 0;
	int x = 0;
	int width = 0;

	while(*tptr) {
		uint32_t uchar = *tptr++;

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

int text_width_raw(Font *font, const char *text, uint maxlines) {
	uint32_t buf[strlen(text) + 1];
	utf8_to_ucs4(text, sizeof(buf), buf);
	return text_ucs4_width_raw(font, buf, maxlines);
}

void text_ucs4_bbox(Font *font, const uint32_t *text, uint maxlines, BBox *bbox) {
	const uint32_t *tptr = text;
	uint prev_glyph_idx = 0;
	bool keming = FT_HAS_KERNING(font->face);
	uint numlines = 0;

	memset(bbox, 0, sizeof(*bbox));
	int x = 0, y = 0;

	while(*tptr) {
		uint32_t uchar = *tptr++;

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

void text_bbox(Font *font, const char *text, uint maxlines, BBox *bbox) {
	uint32_t buf[strlen(text) + 1];
	utf8_to_ucs4(text, sizeof(buf), buf);
	text_ucs4_bbox(font, buf, maxlines, bbox);
}

double text_ucs4_width(Font *font, const uint32_t *text, uint maxlines) {
	return text_ucs4_width_raw(font, text, maxlines) / font->metrics.scale;
}

double text_width(Font *font, const char *text, uint maxlines) {
	return text_width_raw(font, text, maxlines) / font->metrics.scale;
}

int text_ucs4_height_raw(Font *font, const uint32_t *text, uint maxlines) {
	// FIXME: I'm not sure this is correct. Perhaps it should consider max_glyph_height at least?

	uint text_lines = 1;
	const uint32_t *tptr = text;

	while((tptr = ucs4chr(tptr, '\n'))) {
		if(text_lines++ == maxlines) {
			break;
		}
	}

	return font->metrics.lineskip * text_lines;
}

int text_height_raw(Font *font, const char *text, uint maxlines) {
	uint32_t buf[strlen(text) + 1];
	utf8_to_ucs4(text, sizeof(buf), buf);
	return text_ucs4_height_raw(font, buf, maxlines);
}

double text_ucs4_height(Font *font, const uint32_t *text, uint maxlines) {
	return text_ucs4_height_raw(font, text, maxlines) / font->metrics.scale;
}

double text_height(Font *font, const char *text, uint maxlines) {
	return text_height_raw(font, text, maxlines) / font->metrics.scale;
}

static inline void adjust_xpos(Font *font, const uint32_t *ucs4text, Alignment align, double x_orig, double *x) {
	double line_width;

	switch(align) {
		case ALIGN_LEFT: {
			*x = x_orig;
			break;
		}

		case ALIGN_RIGHT: {
			line_width = text_ucs4_width_raw(font, ucs4text, 1);
			*x = x_orig - line_width;
			break;
		}

		case ALIGN_CENTER: {
			line_width = text_ucs4_width_raw(font, ucs4text, 1);
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
static double _text_ucs4_draw(Font *font, const uint32_t *ucs4text, const TextParams *params) {
	SpriteParams sp = { .sprite = NULL };
	BBox bbox;
	double x = params->pos.x;
	double y = params->pos.y;
	double iscale = 1 / font->metrics.scale;

	text_ucs4_bbox(font, ucs4text, 0, &bbox);
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
	sp.shader_params = params->shader_params;
	memcpy(sp.aux_textures, params->aux_textures, sizeof(sp.aux_textures));

	if(sp.color == NULL) {
		// XXX: sprite batch code defaults this to RGB(1, 1, 1)
		sp.color = r_color_current();
	}

	MatrixMode mm_prev = r_mat_mode_current();
	r_mat_mode(MM_MODELVIEW);
	r_mat_push();
	r_mat_translate(x, y, 0);
	r_mat_scale(iscale, iscale, 1);
	x = y = 0;

	double x_orig = x;
	adjust_xpos(font, ucs4text, params->align, x_orig, &x);

	// bbox.y.max = imax(bbox.y.max, font->metrics.ascent);
	// bbox.y.min = imin(bbox.y.min, font->metrics.descent);

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
	r_color(color_mul(RGBA(0.5, 0.5, 0.5, 0.5), r_color_current()));
	r_draw_quad();
	r_mat_pop();
	r_state_pop();
	#endif

	r_mat_mode(MM_TEXTURE);
	r_mat_push();
	r_mat_scale(1/bbox_w, 1/bbox_h, 1.0);
	r_mat_translate(-bbox.x.min - (x - x_orig), -bbox.y.min + font->metrics.descent, 0);

	// FIXME: is there a better way?
	float texmat_offset_sign;

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		texmat_offset_sign = -1;
	} else {
		texmat_offset_sign = 1;
	}

	bool keming = FT_HAS_KERNING(font->face);
	uint prev_glyph_idx = 0;
	const uint32_t *tptr = ucs4text;

	while(*tptr) {
		uint32_t uchar = *tptr++;

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

		sp.sprite_ptr = &glyph->sprite;

		if(glyph->sprite.tex != NULL) {
			sp.pos.x = x + glyph->metrics.bearing_x + sp.sprite_ptr->w * 0.5;
			sp.pos.y = y - glyph->metrics.bearing_y + sp.sprite_ptr->h * 0.5 - font->metrics.descent;

			// HACK/FIXME: Glyphs have their sprite w/h unadjusted for scale.
			// We have to temporarily fix that up here so that the shader gets resolution-independent dimensions.
			float w_saved = sp.sprite_ptr->w;
			float h_saved = sp.sprite_ptr->h;
			sp.sprite_ptr->w /= font->metrics.scale;
			sp.sprite_ptr->h /= font->metrics.scale;
			sp.scale.both = font->metrics.scale;

			r_mat_push();
			r_mat_translate(sp.pos.x - x_orig, sp.pos.y * texmat_offset_sign, 0);
			r_mat_scale(w_saved, h_saved, 1.0);
			r_mat_translate(-0.5, -0.5, 0);

			if(params->glyph_callback.func != NULL) {
				x += params->glyph_callback.func(font, uchar, &sp, params->glyph_callback.userdata);
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

static double _text_draw(Font *font, const char *text, const TextParams *params) {
	uint32_t buf[strlen(text) + 1];
	utf8_to_ucs4(text, sizeof(buf), buf);

	if(params->max_width > 0) {
		text_ucs4_shorten(font, buf, params->max_width);
	}

	return _text_ucs4_draw(font, buf, params);
}

double text_draw(const char *text, const TextParams *params) {
	return _text_draw(font_from_params(params), text, params);
}

double text_ucs4_draw(const uint32_t *text, const TextParams *params) {
	Font *font = font_from_params(params);

	if(params->max_width > 0) {
		uint32_t buf[ucs4len(text) + 1];
		memcpy(buf, text, sizeof(buf));
		text_ucs4_shorten(font, buf, params->max_width);
		return _text_ucs4_draw(font, buf, params);
	}

	return _text_ucs4_draw(font, text, params);
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

	Texture *tex = globals.render_tex;

	int tex_new_w = bbox_width; // max(tex->w, bbox_width);
	int tex_new_h = bbox_height; // max(tex->h, bbox_height);
	uint tex_w, tex_h;
	r_texture_get_size(tex, 0, &tex_w, &tex_h);

	if(tex_new_w != tex_w || tex_new_h != tex_h) {
		log_info(
			"Resizing texture: %ix%i --> %ix%i",
			tex_w, tex_h,
			tex_new_w, tex_new_h
		);

		TextureParams params;
		r_texture_get_params(tex, &params);
		r_texture_destroy(tex);
		params.width = tex_new_w;
		params.height = tex_new_h;
		params.mipmaps = 0;
		globals.render_tex = tex = r_texture_create(&params);

		#ifdef DEBUG
		char buf[128];
		snprintf(buf, sizeof(buf), "Font render texture");
		r_texture_set_debug_label(tex, buf);
		#endif

		r_framebuffer_attach(globals.render_buf, tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
		r_framebuffer_viewport(globals.render_buf, 0, 0, tex_new_w, tex_new_h);
	}

	r_state_push();

	r_framebuffer(globals.render_buf);
	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);

	r_blend(BLEND_PREMUL_ALPHA);
	r_enable(RCAP_CULL_FACE);
	r_cull(CULL_BACK);
	r_disable(RCAP_DEPTH_TEST);

	r_mat_mode(MM_MODELVIEW);
	r_mat_push();
	r_mat_identity();

	r_mat_mode(MM_PROJECTION);
	r_mat_push();
	r_mat_identity();
	r_mat_ortho(0, tex_new_w, tex_new_h, 0, -100, 100);

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
		.color = RGB(1, 1, 1),
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

void text_ucs4_shorten(Font *font, uint32_t *text, double width) {
	assert(!ucs4chr(text, '\n'));

	if(text_ucs4_width(font, text, 0) <= width) {
		return;
	}

	int l = ucs4len(text);

	do {
		if(l < 1) {
			return;
		}

		text[l] = 0;
		text[l - 1] = UNICODE_ELLIPSIS;
		--l;
	} while(text_ucs4_width(font, text, 0) > width);
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
