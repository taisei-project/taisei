/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#include "font.h"
#include "config.h"
#include "events.h"
#include "renderer/api.h"
#include "util.h"
#include "util/glm.h"
#include "util/graphics.h"
#include "util/rectpack.h"
#include "video.h"
#include "dynarray.h"

static void init_fonts(void);
static void post_init_fonts(void);
static void shutdown_fonts(void);
static char *font_path(const char*);
static bool check_font_path(const char*);
static void load_font(ResourceLoadState *st);
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
		.load = load_font,
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

typedef struct SpriteSheet {
	LIST_INTERFACE(struct SpriteSheet);
	Texture *tex;
	RectPack *rectpack;
	uint glyphs;
} SpriteSheet;

typedef LIST_ANCHOR(SpriteSheet) SpriteSheetAnchor;

typedef struct Glyph {
	Sprite sprite;
	SpriteSheet *spritesheet;
	RectPackSection *spritesheet_section;
	GlyphMetrics metrics;
	ulong ft_index;
} Glyph;

struct Font {
	char *source_path;
	DYNAMIC_ARRAY(Glyph) glyphs;
	FT_Face face;
	FT_Stroker stroker;
	long base_face_idx;
	int base_size;
	ht_int2int_t charcodes_to_glyph_ofs;
	ht_int2int_t ftindex_to_glyph_ofs;
	FontMetrics metrics;
	bool kerning;

#ifdef DEBUG
	char debug_label[64];
#endif
};

static struct {
	FT_Library lib;
	ShaderProgram *default_shader;
	Texture *render_tex;
	Framebuffer *render_buf;
	SpriteSheetAnchor spritesheets;

	struct {
		SDL_mutex *new_face;
		SDL_mutex *done_face;
	} mutex;
} globals;

static double global_font_scale(void) {
	float w, h;
	video_get_viewport_size(&w, &h);
	return fmax(0.1, ((double)h / SCREEN_H) * config_get_float(CONFIG_TEXT_QUALITY));
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
				val->f = fmax(0.1, val->f);
				reload_fonts(global_font_scale());
			}

			return false;
		}
	}

	return false;
}

static void try_create_mutex(SDL_mutex **mtx) {
	if((*mtx = SDL_CreateMutex()) == NULL) {
		log_sdl_error(LOG_WARN, "SDL_CreateMutex");
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
		.flags  = TEX_FLAG_STREAM,
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
		log_error("Can't seek in stream (%s)", (char*)stream->pathname.pointer);
		return error;
	}

	return SDL_RWread(rwops, buffer, 1, count);
}

static void ftstream_close(FT_Stream stream) {
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
		log_error("VFS error: %s", vfs_get_error());
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
		log_error("Failed to load font '%s' (face %li): FT_Open_Face() failed: %s", syspath, index, ft_error_str(err));
		free(syspath);
		free(ftstream);
		return NULL;
	}

	if(!(FT_IS_SCALABLE(face))) {
		log_error("Font '%s' (face %li) is not scalable. This is not supported, sorry. Load aborted", syspath, index);
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
		log_error("FT_Set_Char_Size(%u) failed: %s", pxsize, ft_error_str(err));
		return err;
	}

	if(fnt->stroker) {
		FT_Stroker_Done(fnt->stroker);
	}

	if((err = FT_Stroker_New(globals.lib, &fnt->stroker))) {
		log_error("FT_Stroker_New() failed: %s", ft_error_str(err));
		return err;
	}

	FT_Stroker_Set(fnt->stroker, FT_MulFix(1.5 * 64, fixed_scale), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

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

bool font_get_kerning_available(Font *font) {
	return FT_HAS_KERNING(font->face);
}

bool font_get_kerning_enabled(Font *font) {
	return font->kerning;
}

void font_set_kerning_enabled(Font *font, bool newval) {
	font->kerning = (newval && FT_HAS_KERNING(font->face));
}

// TODO: Figure out sensible values for these; maybe make them depend on font size in some way.
#define SS_WIDTH 1024
#define SS_HEIGHT 1024

#define SS_TEXTURE_TYPE TEX_TYPE_RGB_8
#define SS_TEXTURE_FLAGS 0

static SpriteSheet* add_spritesheet(SpriteSheetAnchor *spritesheets) {
	SpriteSheet *ss = calloc(1, sizeof(SpriteSheet));
	ss->rectpack = rectpack_new(SS_WIDTH, SS_HEIGHT);

	ss->tex = r_texture_create(&(TextureParams) {
		.width = SS_WIDTH,
		.height = SS_HEIGHT,
		.type = SS_TEXTURE_TYPE,
		.flags = SS_TEXTURE_FLAGS,
		.filter.mag = TEX_FILTER_LINEAR,
		.filter.min = TEX_FILTER_LINEAR,
		.wrap.s = TEX_WRAP_CLAMP,
		.wrap.t = TEX_WRAP_CLAMP,
		.mipmaps = TEX_MIPMAPS_MAX,
		.mipmap_mode = TEX_MIPMAP_AUTO,
	});

#ifdef DEBUG
	char buf[128];
	snprintf(buf, sizeof(buf), "Fonts spritesheet %p", (void*)ss);
	r_texture_set_debug_label(ss->tex, buf);
#endif

	r_texture_clear(ss->tex, RGBA(0, 0, 0, 0));
	alist_append(spritesheets, ss);
	return ss;
}

// The padding is needed to prevent glyph edges from bleeding in due to linear filtering.
#define GLYPH_SPRITE_PADDING 1

static bool add_glyph_to_spritesheet(Glyph *glyph, Pixmap *pixmap, SpriteSheet *ss) {
	uint padded_w = pixmap->width + 2 * GLYPH_SPRITE_PADDING;
	uint padded_h = pixmap->height + 2 * GLYPH_SPRITE_PADDING;

	glyph->spritesheet_section = rectpack_add(ss->rectpack, padded_w, padded_h);

	if(glyph->spritesheet_section == NULL) {
		return false;
	}

	glyph->spritesheet = ss;

	cmplx ofs = GLYPH_SPRITE_PADDING * (1+I);

	Rect sprite_pos = rectpack_section_rect(glyph->spritesheet_section);
	sprite_pos.bottom_right += ofs;
	sprite_pos.top_left += ofs;

	r_texture_fill_region(
		ss->tex,
		0,
		rect_x(sprite_pos),
		rect_y(sprite_pos),
		pixmap
	);

	Sprite *sprite = &glyph->sprite;
	sprite->tex = ss->tex;
	sprite->w = pixmap->width;
	sprite->h = pixmap->height;

	sprite_set_denormalized_tex_coords(sprite, (FloatRect) {
		.x = rect_x(sprite_pos),
		.y = rect_y(sprite_pos),
		.w = pixmap->width,
		.h = pixmap->height,
	});

	++ss->glyphs;

	return true;
}

static bool add_glyph_to_spritesheets(Glyph *glyph, Pixmap *pixmap, SpriteSheetAnchor *spritesheets) {
	bool result;

	for(SpriteSheet *ss = spritesheets->first; ss; ss = ss->next) {
		if((result = add_glyph_to_spritesheet(glyph, pixmap, ss))) {
			return result;
		}
	}

	return add_glyph_to_spritesheet(glyph, pixmap, add_spritesheet(spritesheets));
}

static const char* pixmode_name(FT_Pixel_Mode mode) {
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

	FT_Error err = FT_Load_Glyph(font->face, gindex, FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LIGHT);

	if(err) {
		log_warn("FT_Load_Glyph(%u) failed: %s", gindex, ft_error_str(err));
		return NULL;
	}

	Glyph *glyph = dynarray_append(&font->glyphs);

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
			font->glyphs.num_elements--;
			return NULL;
		}

		Pixmap px;
		px.origin = PIXMAP_ORIGIN_BOTTOMLEFT;
		px.format = PIXMAP_FORMAT_RGB8;
		px.width = imax(g_bm_fill->bitmap.width, imax(g_bm_border->bitmap.width, g_bm_inner->bitmap.width));
		px.height = imax(g_bm_fill->bitmap.rows, imax(g_bm_border->bitmap.rows, g_bm_inner->bitmap.rows));
		px.data.rg8 = pixmap_alloc_buffer_for_copy(&px, &px.data_size);

		int ref_left = g_bm_border->left;
		int ref_top = g_bm_border->top;

		ssize_t fill_ofs_x   =  (g_bm_fill->left   - ref_left);
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

				ssize_t fill_index   = fill_coord_x   + (g_bm_fill->bitmap.rows   - fill_coord_y   - 1) * g_bm_fill->bitmap.pitch;
				ssize_t border_index = border_coord_x + (g_bm_border->bitmap.rows - border_coord_y - 1) * g_bm_border->bitmap.pitch;
				ssize_t inner_index  = inner_coord_x  + (g_bm_inner->bitmap.rows  - inner_coord_y  - 1) * g_bm_inner->bitmap.pitch;

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

		TextureTypeQueryResult qr = { 0 };

		// TODO: Only query this once on init.
		if(r_texture_type_query(SS_TEXTURE_TYPE, SS_TEXTURE_FLAGS, px.format, px.origin, &qr)) {
			pixmap_convert_inplace_realloc(&px, qr.optimal_pixmap_format);
			pixmap_flip_to_origin_inplace(&px, qr.optimal_pixmap_origin);
		} else {
			log_error("Texture query failed!");
			assert(0);
		}

		if(!add_glyph_to_spritesheets(glyph, &px, spritesheets)) {
			log_error(
				"Glyph %u fill can't fit into any spritesheets (padded bitmap size: %ux%u; max spritesheet size: %ux%u)",
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
			--font->glyphs.num_elements;
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
			ofs = glyph ? dynarray_indexof(&fnt->glyphs, glyph) : -1;
		} else if(!ht_lookup(&fnt->ftindex_to_glyph_ofs, ft_index, &ofs)) {
			glyph = load_glyph(fnt, ft_index, &globals.spritesheets);
			ofs = glyph ? dynarray_indexof(&fnt->glyphs, glyph) : -1;
			ht_set(&fnt->ftindex_to_glyph_ofs, ft_index, ofs);
		}

		ht_set(&fnt->charcodes_to_glyph_ofs, cp, ofs);
	}

	return ofs < 0 ? NULL : dynarray_get_ptr(&fnt->glyphs, ofs);
}

attr_nonnull(1)
static void wipe_glyph_cache(Font *font) {
	dynarray_foreach_elem(&font->glyphs, Glyph *g, {
		SpriteSheet *ss = g->spritesheet;

		if(ss == NULL) {
			continue;
		}

		RectPack *rp = ss->rectpack;
		assume(rp != NULL);

		RectPackSection *section = g->spritesheet_section;
		assume(section != NULL);

		rectpack_reclaim(rp, section);

		if(rectpack_is_empty(rp)) {
			delete_spritesheet(&globals.spritesheets, ss);
		}
	});

	ht_unset_all(&font->charcodes_to_glyph_ofs);
	ht_unset_all(&font->ftindex_to_glyph_ofs);

	font->glyphs.num_elements = 0;
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
	dynarray_free_data(&font->glyphs);
}

void load_font(ResourceLoadState *st) {
	Font font;
	memset(&font, 0, sizeof(font));

	if(!parse_keyvalue_file_with_spec(st->path, (KVSpec[]){
		{ "source",  .out_str   = &font.source_path },
		{ "size",    .out_int   = &font.base_size },
		{ "face",    .out_long  = &font.base_face_idx },
		{ NULL }
	})) {
		log_error("Failed to parse font file '%s'", st->path);
		free(font.source_path);
		res_load_failed(st);
		return;
	}

	if(!font.source_path) {
		log_error("%s: No source path specified", st->path);
		res_load_failed(st);
		return;
	}

	ht_create(&font.charcodes_to_glyph_ofs);
	ht_create(&font.ftindex_to_glyph_ofs);

	if(!(font.face = load_font_face(font.source_path, font.base_face_idx))) {
		free_font_resources(&font);
		res_load_failed(st);
	}

	if(set_font_size(&font, font.base_size, global_font_scale())) {
		free_font_resources(&font);
		res_load_failed(st);
		return;
	}

	dynarray_ensure_capacity(&font.glyphs, 32);

#ifdef DEBUG
	strlcpy(font.debug_label, st->name, sizeof(font.debug_label));
#endif

	font_set_kerning_enabled(&font, true);
	res_load_finished(st, memdup(&font, sizeof(font)));
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

	if(!font_get_kerning_enabled(font) || prev_index == 0) {
		return 0;
	}

	if(!FT_Get_Kerning(font->face, prev_index, gthis->ft_index, FT_KERNING_DEFAULT, &kvec)) {
		return kvec.x >> 6;
	}

	return 0;
}

int text_ucs4_width_raw(Font *font, const uint32_t *text, uint maxlines) {
	const uint32_t *tptr = text;
	uint prev_glyph_idx = 0;
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

		x += apply_kerning(font, prev_glyph_idx, glyph);
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

		x += apply_kerning(font, prev_glyph_idx, glyph);

		int g_x0 = x + glyph->metrics.bearing_x;
		int g_x1 = g_x0 + imax(glyph->metrics.width, glyph->sprite.w);

		bbox->x.max = imax(bbox->x.max, g_x0);
		bbox->x.max = imax(bbox->x.max, g_x1);
		bbox->x.min = imin(bbox->x.min, g_x0);
		bbox->x.min = imin(bbox->x.min, g_x1);

		int g_y0 = y - glyph->metrics.bearing_y;
		int g_y1 = g_y0 + imax(glyph->metrics.height, glyph->sprite.h);

		bbox->y.max = imax(bbox->y.max, g_y0);
		bbox->y.max = imax(bbox->y.max, g_y1);
		bbox->y.min = imin(bbox->y.min, g_y0);
		bbox->y.min = imin(bbox->y.min, g_y1);

		prev_glyph_idx = glyph->ft_index;

		x += glyph->metrics.advance;
		bbox->x.max = imax(bbox->x.max, x);
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

		if(*tptr == '\n') {
			++tptr;
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

ShaderProgram* text_get_default_shader(void) {
	return globals.default_shader;
}

attr_returns_nonnull
static Font *font_from_params(const TextParams *params) {
	Font *font = params->font_ptr;

	if(font == NULL) {
		if(params->font != NULL) {
			font = res_font(params->font);
		} else {
			font = res_font("standard");
		}
	}

	assert(font != NULL);
	return font;
}

static void set_batch_texture(SpriteStateParams *stp, Texture *tex) {
	if(stp->primary_texture != tex) {
		stp->primary_texture = tex;
		r_sprite_batch_prepare_state(stp);
	}
}

attr_nonnull(1, 2, 3)
static double _text_ucs4_draw(Font *font, const uint32_t *ucs4text, const TextParams *params) {
	SpriteStateParams batch_state_params;

	memcpy(batch_state_params.aux_textures, params->aux_textures, sizeof(batch_state_params.aux_textures));

	if((batch_state_params.blend = params->blend) == 0) {
		batch_state_params.blend = r_blend_current();
	}

	if((batch_state_params.shader = params->shader_ptr) == NULL) {
		if(params->shader != NULL) {
			batch_state_params.shader = res_shader(params->shader);
		} else {
			batch_state_params.shader = r_shader_current();
		}
	}

	batch_state_params.primary_texture = NULL;

	BBox bbox;
	double x = params->pos.x;
	double y = params->pos.y;
	double scale = font->metrics.scale;
	double iscale = 1 / scale;

	struct {
		struct { double min, max; } x, y;
		double w, h;
	} overlay;

	text_ucs4_bbox(font, ucs4text, 0, &bbox);

	Color color;

	if(params->color == NULL) {
		// XXX: sprite batch code defaults this to RGB(1, 1, 1)
		color = *r_color_current();
	} else {
		color = *params->color;
	}

	ShaderCustomParams shader_params;

	if(params->shader_params == NULL) {
		memset(&shader_params, 0, sizeof(shader_params));
	} else {
		shader_params = *params->shader_params;
	}

	mat4 mat_texture;
	mat4 mat_model;

	r_mat_tex_current(mat_texture);
	r_mat_mv_current(mat_model);

	glm_translate(mat_model, (vec3) { x, y } );
	glm_scale(mat_model, (vec3) { iscale, iscale, 1 } );

	double orig_x = x;
	double orig_y = y;
	x = y = 0;

	adjust_xpos(font, ucs4text, params->align, 0, &x);

	if(params->overlay_projection) {
		FloatRect *op = params->overlay_projection;
		overlay.x.min = (op->x - orig_x) * scale;
		overlay.x.max = overlay.x.min + op->w * scale;
		overlay.y.min = (op->y - orig_y) * scale;
		overlay.y.max = overlay.y.min + op->h * scale;
	} else {
		overlay.x.min = bbox.x.min + x;
		overlay.x.max = bbox.x.max + x;
		overlay.y.min = bbox.y.min - font->metrics.descent;
		overlay.y.max = bbox.y.max - font->metrics.descent;
	}

	overlay.w = overlay.x.max - overlay.x.min;
	overlay.h = overlay.y.max - overlay.y.min;

	glm_scale(mat_texture, (vec3) { 1/overlay.w, 1/overlay.h, 1.0 });
	glm_translate(mat_texture, (vec3) { -overlay.x.min, overlay.y.min, 0 });

	// FIXME: is there a better way?
	float texmat_offset_sign;

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		texmat_offset_sign = -1;
	} else {
		texmat_offset_sign = 1;
	}

	uint prev_glyph_idx = 0;
	const uint32_t *tptr = ucs4text;

	while(*tptr) {
		uint32_t uchar = *tptr++;

		if(uchar == '\n') {
			adjust_xpos(font, tptr, params->align, 0, &x);
			y += font->metrics.lineskip;
			continue;
		}

		Glyph *glyph = get_glyph(font, uchar);

		if(glyph == NULL) {
			continue;
		}

		x += apply_kerning(font, prev_glyph_idx, glyph);

		if(glyph->sprite.tex != NULL) {
			Sprite *spr = &glyph->sprite;
			set_batch_texture(&batch_state_params, spr->tex);

			SpriteInstanceAttribs attribs;
			attribs.rgba = color;
			attribs.custom = shader_params;

			float g_x = x + glyph->metrics.bearing_x + spr->w * 0.5;
			float g_y = y - glyph->metrics.bearing_y + spr->h * 0.5 - font->metrics.descent;

			glm_translate_to(mat_texture, (vec3) { g_x - spr->w * 0.5, g_y * texmat_offset_sign + overlay.h - spr->h * 0.5 }, attribs.tex_transform );
			glm_scale(attribs.tex_transform, (vec3) { spr->w, spr->h, 1.0 });

			glm_translate_to(mat_model, (vec3) { g_x, g_y }, attribs.mv_transform);
			glm_scale(attribs.mv_transform, (vec3) { spr->w, spr->h, 1.0 } );

			attribs.texrect = spr->tex_area;

			// NOTE: Glyphs have their sprite w/h unadjusted for scale.
			attribs.sprite_size.w = spr->w * iscale;
			attribs.sprite_size.h = spr->h * iscale;

			if(params->glyph_callback.func != NULL) {
				params->glyph_callback.func(font, uchar, &attribs, params->glyph_callback.userdata);
			}

			r_sprite_batch_add_instance(&attribs);
		}

		x += glyph->metrics.advance;
		prev_glyph_idx = glyph->ft_index;
	}

	return x * iscale;
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

	r_mat_mv_push_identity();
	r_mat_proj_push_ortho(tex_new_w, tex_new_h);
	r_mat_tex_push_identity();

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

	r_mat_tex_pop();
	r_mat_proj_pop();
	r_mat_mv_pop();

	r_state_pop();

	out_sprite->tex = tex;
	out_sprite->tex_area.w = 1;
	out_sprite->tex_area.h = 1;
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

	while((next = strtok_r(NULL, " \t", &sptr))) {
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

double font_get_ascent(Font *font) {
	return font->metrics.descent / font->metrics.scale;
}

double font_get_descent(Font *font) {
	return font->metrics.descent / font->metrics.scale;
}

const GlyphMetrics* font_get_char_metrics(Font *font, charcode_t c) {
	Glyph *g = get_glyph(font, c);

	if(!g) {
		return NULL;
	}

	return &g->metrics;
}
