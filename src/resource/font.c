/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "font.h"

#include "config.h"
#include "dynarray.h"
#include "events.h"
#include "memory/arena.h"
#include "renderer/api.h"
#include "util.h"
#include "util/glm.h"
#include "util/kvparser.h"
#include "util/rectpack.h"
#include "video.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_MODULE_H

static void init_fonts(void);
static void post_init_fonts(void);
static void shutdown_fonts(void);
static char *font_path(const char*);
static bool check_font_path(const char*);
static void load_font_stage1(ResourceLoadState *st);
static void load_font_stage2(ResourceLoadState *st);
static void unload_font(void*);
static bool transfer_font(void*, void*);

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
		.load = load_font_stage1,
		.unload = unload_font,
		.transfer = transfer_font,
	},
};

#undef FTERRORS_H_
#define FT_ERRORDEF(e, v, s)  { e, s },
#undef FT_ERROR_START_LIST
#undef FT_ERROR_END_LIST

#define FIXED_CONVERSION(ft_typename, func_typename, factor) \
	attr_unused INLINE float func_typename##_to_float(ft_typename x) { \
		return x * (1.0f / factor); \
	} \
	\
	attr_unused INLINE ft_typename float_to_##func_typename(float x) { \
		return roundf(x * factor); \
	}

FIXED_CONVERSION(FT_F26Dot6, f26dot6, 64.0f)
FIXED_CONVERSION(FT_Fixed, f16dot16, 65536.0f)

#define GLOBAL_RENDER_MODE FT_RENDER_MODE_NORMAL

static const struct ft_error_def {
	FT_Error    err_code;
	const char *err_msg;
} ft_errors[] = {
	#include FT_ERRORS_H
	{ 0, NULL }
};

static const char *ft_error_str(FT_Error err_code) {
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
	RectPack rectpack;
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
	float base_border_inner;
	float base_border_outer;
	ht_int2int_t charcodes_to_glyph_ofs;
	ht_int2int_t ftindex_to_glyph_ofs;
	FontMetrics metrics;
	bool kerning;

	struct Font **fallbacks;
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
	MemArena arena;
	RectPackSectionPool rpspool;

	struct {
		SDL_Mutex *new_face;
		SDL_Mutex *done_face;
	} mutex;

	ResourceGroup rg;
} globals;

static float global_font_scale(void) {
	float w, h;
	video_get_viewport_size(&w, &h);
	return max(0.1f, (h / SCREEN_H) * config_get_float(CONFIG_TEXT_QUALITY));
}

static void reload_fonts(float quality);

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
				val->f = max(0.1f, val->f);
				reload_fonts(global_font_scale());
			}

			return false;
		}
	}

	return false;
}

static void try_create_mutex(SDL_Mutex **mtx) {
	if((*mtx = SDL_CreateMutex()) == NULL) {
		log_sdl_error(LOG_WARN, "SDL_CreateMutex");
	}
}

static void *ft_alloc(FT_Memory mem, long size) {
	return mem_alloc(size);
}

static void ft_free(FT_Memory mem, void *block) {
	mem_free(block);
}

static void *ft_realloc(FT_Memory mem, long cur_size, long new_size, void *block) {
	return mem_realloc(block, new_size);
}

static void init_fonts(void) {
	FT_Error err;

	marena_init(&globals.arena, sizeof(RectPackSection) * 128);

	static typeof(*(FT_Memory)0) ft_mem = {
		.alloc = ft_alloc,
		.free = ft_free,
		.realloc = ft_realloc,
	};

	try_create_mutex(&globals.mutex.new_face);
	try_create_mutex(&globals.mutex.done_face);

	if((err = FT_New_Library(&ft_mem, &globals.lib))) {
		log_fatal("FT_New_Library() failed: %s", ft_error_str(err));
	}

	FT_Add_Default_Modules(globals.lib);

	events_register_handler(&(EventHandler) {
		fonts_event, NULL, EPRIO_SYSTEM,
	});

	res_group_init(&globals.rg);

	res_group_preload(&globals.rg, RES_FONT, RESF_DEFAULT,
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
	res_group_preload(&globals.rg, RES_SHADER_PROGRAM, RESF_DEFAULT, "text_default", NULL);
	globals.default_shader = res_shader("text_default");
}

static void shutdown_fonts(void) {
	res_group_release(&globals.rg);
	res_purge();
	r_texture_destroy(globals.render_tex);
	r_framebuffer_destroy(globals.render_buf);
	events_unregister_handler(fonts_event);
	FT_Done_Library(globals.lib);
	SDL_DestroyMutex(globals.mutex.new_face);
	SDL_DestroyMutex(globals.mutex.done_face);
	marena_deinit(&globals.arena);
}

static char *font_path(const char *name) {
	return strjoin(FONT_PATH_PREFIX, name, FONT_EXTENSION, NULL);
}

bool check_font_path(const char *path) {
	return strstartswith(path, FONT_PATH_PREFIX) && strendswith(path, FONT_EXTENSION);
}

static ulong ftstream_read(FT_Stream stream, ulong offset, uchar *buffer, ulong count) {
	SDL_IOStream *rwops = stream->descriptor.pointer;
	ulong error = count ? 0 : 1;

	if(SDL_SeekIO(rwops, offset, SDL_IO_SEEK_SET) < 0) {
		log_error("Can't seek in stream (%s): %s", (char*)stream->pathname.pointer, SDL_GetError());
		return error;
	}

	if(count == 0) {
		return 0;
	}

	size_t r = SDL_ReadIO(rwops, buffer, count);

	if(r == 0 && SDL_GetIOStatus(rwops) != SDL_IO_STATUS_EOF) {
		log_error("Read error (%s): %s", (char*)stream->pathname.pointer, SDL_GetError());
	}

	return r;
}

static void ftstream_close(FT_Stream stream) {
	SDL_CloseIO((SDL_IOStream*)stream->descriptor.pointer);
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

	SDL_IOStream *rwops = vfs_open(vfspath, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!rwops) {
		log_error("VFS error: %s", vfs_get_error());
		mem_free(syspath);
		return NULL;
	}

	FT_Stream ftstream = ALLOC(typeof(*ftstream), {
		.descriptor.pointer = rwops,
		.pathname.pointer = syspath,
		.read = ftstream_read,
		.close = ftstream_close,
		.size = SDL_GetIOSize(rwops),
	});

	FT_Open_Args ftargs = {
		.flags = FT_OPEN_STREAM,
		.stream = ftstream,
	};

	FT_Face face;
	FT_Error err;

	if((err = FT_Open_Face_Thread_Safe(globals.lib, &ftargs, index, &face))) {
		log_error("Failed to load font '%s' (face %li): FT_Open_Face() failed: %s", syspath, index, ft_error_str(err));
		mem_free(syspath);
		mem_free(ftstream);
		return NULL;
	}

	if(!(FT_IS_SCALABLE(face))) {
		log_error("Font '%s' (face %li) is not scalable. This is not supported, sorry. Load aborted", syspath, index);
		FT_Done_Face_Thread_Safe(face);
		mem_free(syspath);
		mem_free(ftstream);
		return NULL;
	}

	log_info("Loaded font '%s' (face %li)", syspath, index);
	return face;
}

static FT_Error set_font_size(Font *fnt, float scale) {
	FT_Error err = FT_Err_Ok;
	uint pxsize = fnt->base_size;

	assert(fnt != NULL);
	assert(fnt->face != NULL);

	pxsize = float_to_f26dot6(pxsize * scale);

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

	FT_Face face = fnt->face;
	FT_Fixed fixed_scale = face->size->metrics.y_scale;
	fnt->metrics.ascent = f26dot6_to_float(FT_MulFix(face->ascender, fixed_scale));
	fnt->metrics.descent = f26dot6_to_float(FT_MulFix(face->descender, fixed_scale));
	fnt->metrics.max_glyph_height = fnt->metrics.ascent - fnt->metrics.descent;
	fnt->metrics.lineskip = f26dot6_to_float(FT_MulFix(face->height, fixed_scale));
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
#define SS_WIDTH 2048
#define SS_HEIGHT 2048

#define SS_TEXTURE_TYPE TEX_TYPE_RGB_8
#define SS_TEXTURE_FLAGS 0

INLINE RectPackSectionSource rps_source(void) {
	return (RectPackSectionSource) {
		.arena = &globals.arena,
		.pool = &globals.rpspool,
	};
}

static SpriteSheet *add_spritesheet(SpriteSheetAnchor *spritesheets) {
	auto ss = ALLOC(SpriteSheet, {
		.tex = r_texture_create(&(TextureParams) {
			.width = SS_WIDTH,
			.height = SS_HEIGHT,
			.type = SS_TEXTURE_TYPE,
			.flags = SS_TEXTURE_FLAGS,
			.filter.mag = TEX_FILTER_LINEAR,
			.filter.min = TEX_FILTER_LINEAR,
			.wrap.s = TEX_WRAP_CLAMP,
			.wrap.t = TEX_WRAP_CLAMP,
			.mipmaps = 1,
			.mipmap_mode = TEX_MIPMAP_MANUAL,
		})
	});

	rectpack_init(&ss->rectpack, SS_WIDTH, SS_HEIGHT);

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

	glyph->spritesheet_section = rectpack_add(&ss->rectpack, rps_source(), padded_w, padded_h, false);

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
		0, 0,
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

static const char *pixmode_name(FT_Pixel_Mode mode) {
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
	assert(rectpack_is_empty(&ss->rectpack));
	alist_unlink(spritesheets, ss);
	mem_free(ss);
}

static Glyph *load_glyph(Font *font, FT_UInt gindex, SpriteSheetAnchor *spritesheets) {
	// log_debug("Loading glyph 0x%08x", gindex);

	FT_Render_Mode render_mode = GLOBAL_RENDER_MODE;
	FT_Error err = FT_Load_Glyph(font->face, gindex,
		FT_LOAD_NO_BITMAP |
		FT_LOAD_TARGET_(render_mode) |
	0);

	if(err) {
		log_warn("FT_Load_Glyph(%u) failed: %s", gindex, ft_error_str(err));
		return NULL;
	}

	Glyph *glyph = dynarray_append(&font->glyphs, {
		.metrics = {
			.bearing_x = f26dot6_to_float(font->face->glyph->metrics.horiBearingX),
			.bearing_y = f26dot6_to_float(font->face->glyph->metrics.horiBearingY),
			.width = f26dot6_to_float(font->face->glyph->metrics.width),
			.height = f26dot6_to_float(font->face->glyph->metrics.height),
			.advance = f26dot6_to_float(font->face->glyph->metrics.horiAdvance),
			.lsb_delta = f26dot6_to_float(font->face->glyph->lsb_delta),
			.rsb_delta = f26dot6_to_float(font->face->glyph->rsb_delta),
		}
	});

	FT_Glyph g_src = NULL, g_fill = NULL, g_border = NULL, g_inner = NULL;
	FT_BitmapGlyph g_bm_fill = NULL, g_bm_border = NULL, g_bm_inner = NULL;
	FT_Get_Glyph(font->face->glyph, &g_src);
	FT_Glyph_Copy(g_src, &g_fill);
	FT_Glyph_Copy(g_src, &g_border);
	FT_Glyph_Copy(g_src, &g_inner);

	assert(g_src->format == FT_GLYPH_FORMAT_OUTLINE);

	bool have_bitmap = FT_Glyph_To_Bitmap(&g_fill, render_mode, NULL, true) == FT_Err_Ok;

	if(have_bitmap) {
		have_bitmap = ((FT_BitmapGlyph)g_fill)->bitmap.width > 0;
	}

	if(!have_bitmap) {
		// Some glyphs may be invisible, but we still need the metrics data for them (e.g. space)
		glyph->sprite = (typeof(glyph->sprite)) {};
	} else {
		FT_Stroker_Set(font->stroker,
			FT_MulFix(
				float_to_f26dot6(font->base_border_outer),
				font->face->size->metrics.y_scale),
			FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0
		);

		FT_Glyph_StrokeBorder(&g_border, font->stroker, false, true);
		FT_Glyph_To_Bitmap(&g_border, GLOBAL_RENDER_MODE, NULL, true);

		FT_Stroker_Set(font->stroker,
			FT_MulFix(
				float_to_f26dot6(font->base_border_inner),
				font->face->size->metrics.y_scale),
			FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_BEVEL, 0
		);

		FT_Glyph_StrokeBorder(&g_inner, font->stroker, true, true);
		FT_Glyph_To_Bitmap(&g_inner, GLOBAL_RENDER_MODE, NULL, true);

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
		px.width = max(g_bm_fill->bitmap.width, max(g_bm_border->bitmap.width, g_bm_inner->bitmap.width));
		px.height = max(g_bm_fill->bitmap.rows, max(g_bm_border->bitmap.rows, g_bm_inner->bitmap.rows));
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

		TextureTypeQueryResult qr = {};

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
				px.width + 2,
				px.height + 2,
				SS_WIDTH,
				SS_HEIGHT
			);

			mem_free(px.data.rg8);
			FT_Done_Glyph(g_src);
			FT_Done_Glyph(g_fill);
			FT_Done_Glyph(g_border);
			FT_Done_Glyph(g_inner);
			--font->glyphs.num_elements;
			return NULL;
		}

		mem_free(px.data.rg8);

		float xpad = px.width - g_bm_fill->bitmap.width;
		float ypad = px.height - g_bm_fill->bitmap.rows;
		glyph->sprite.padding.extent.w = xpad;
		glyph->sprite.padding.extent.h = ypad;
		glyph->sprite.padding.offset.x = -xpad;
		glyph->sprite.padding.offset.y = -ypad;
		glyph->sprite.extent.as_cmplx += glyph->sprite.padding.extent.as_cmplx;
	}

	FT_Done_Glyph(g_src);
	FT_Done_Glyph(g_fill);
	FT_Done_Glyph(g_border);
	FT_Done_Glyph(g_inner);

	glyph->ft_index = gindex;
	return glyph;
}

static Glyph *_get_glyph(Font *fnt, charcode_t cp) {
	int64_t ofs;

	if(!ht_lookup(&fnt->charcodes_to_glyph_ofs, cp, &ofs)) {
		Glyph *glyph;
		uint ft_index = FT_Get_Char_Index(fnt->face, cp);
		// log_debug("Glyph for charcode 0x%08lx not cached", cp);

		if(ft_index == 0 && cp != UNICODE_UNKNOWN) {
			if(fnt->fallbacks) {
				for(Font **fallback = fnt->fallbacks; *fallback != NULL; fallback++) {
					glyph = _get_glyph(*fallback, cp);
					if(glyph != NULL) {
						return glyph;
					}
				}
			}
			ofs = -1;
		} else if(!ht_lookup(&fnt->ftindex_to_glyph_ofs, ft_index, &ofs)) {
			glyph = load_glyph(fnt, ft_index, &globals.spritesheets);
			ofs = glyph ? dynarray_indexof(&fnt->glyphs, glyph) : -1;
			ht_set(&fnt->ftindex_to_glyph_ofs, ft_index, ofs);
		}

		ht_set(&fnt->charcodes_to_glyph_ofs, cp, ofs);
	}

	return ofs < 0 ? NULL : dynarray_get_ptr(&fnt->glyphs, ofs);
}

static Glyph *get_glyph(Font *fnt, charcode_t cp) {
	Glyph *glyph = _get_glyph(fnt, cp);
	if(glyph == NULL) {
		log_debug("Font or fallbacks have no glyph for charcode 0x%08lx", cp);
		glyph = _get_glyph(fnt, UNICODE_UNKNOWN);
		int64_t ofs = glyph ? dynarray_indexof(&fnt->glyphs, glyph) : -1;
		ht_set(&fnt->charcodes_to_glyph_ofs, cp, ofs);
	}
	return glyph;
}

attr_nonnull(1)
static void wipe_glyph_cache(Font *font) {
	dynarray_foreach_elem(&font->glyphs, Glyph *g, {
		SpriteSheet *ss = g->spritesheet;

		if(ss == NULL) {
			continue;
		}

		RectPack *rp = &ss->rectpack;
		RectPackSection *section = g->spritesheet_section;
		assume(section != NULL);

		rectpack_reclaim(rp, rps_source(), section);

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
			mem_free(stream->pathname.pointer);
			mem_free(stream);
		}
	}

	if(font->stroker) {
		FT_Stroker_Done(font->stroker);
	}

	if(font->fallbacks) {
		mem_free(font->fallbacks);
	}

	wipe_glyph_cache(font);

	ht_destroy(&font->charcodes_to_glyph_ofs);
	ht_destroy(&font->ftindex_to_glyph_ofs);

	mem_free(font->source_path);
	dynarray_free_data(&font->glyphs);
}

static void finish_reload(ResourceLoadState *st);

static int parse_fallbacks(char **commalist) {
	int num_fallbacks = 0;
	if(*commalist) {
		int l = strlen(*commalist);

		char *out = mem_alloc(l+2); // ensures "\0\0" sentinel
		char *w = out;
		for(char *r = *commalist; *r != '\0'; r++) {
			if(*r == ',') {
				num_fallbacks++;
				*w = '\0';
				w++;
			} else if(*r != ' ') {
				*w = *r;
				w++;
			}
		}
		num_fallbacks++;
		mem_free(*commalist);
		*commalist = out;
	}
	return num_fallbacks;
}

typedef struct FontLoadData {
	Font *font;
	char *fallbacks;
} FontLoadData;

void load_font_stage1(ResourceLoadState *st) {
	Font font = {
		.base_border_inner = 0.5f,
		.base_border_outer = 1.5f,
	};

	SDL_IOStream *rw = res_open_file(st, st->path, VFS_MODE_READ);

	if(UNLIKELY(!rw)) {
		log_error("VFS error: %s", vfs_get_error());
		res_load_failed(st);
		return;
	}

	char *fallbacks = NULL;

	bool parsed = parse_keyvalue_stream_with_spec(rw, (KVSpec[]){
		{ "source",        .out_str   = &font.source_path },
		{ "size",          .out_int   = &font.base_size },
		{ "face",          .out_long  = &font.base_face_idx },
		{ "border_inner",  .out_float = &font.base_border_inner, },
		{ "border_outer",  .out_float = &font.base_border_outer, },
		{ "fallbacks",     .out_str   = &fallbacks, },
		{}
	});

	SDL_CloseIO(rw);

	if(UNLIKELY(!parsed)) {
		log_error("Failed to parse font file '%s'", st->path);
		mem_free(font.source_path);
		mem_free(fallbacks);
		res_load_failed(st);
		return;
	}

	int num_fallbacks = parse_fallbacks(&fallbacks);
	if(num_fallbacks > 0) {
		font.fallbacks = ALLOC_ARRAY(num_fallbacks + 1, Font *);
		for(char *fallback = fallbacks; *fallback != '\0'; fallback += strlen(fallback) + 1) {
			res_load_dependency(st, RES_FONT, fallback);
		}
	}

	if(!font.source_path) {
		log_error("%s: No source path specified", st->path);
		mem_free(fallbacks);
		res_load_failed(st);
		return;
	}

	ht_create(&font.charcodes_to_glyph_ofs);
	ht_create(&font.ftindex_to_glyph_ofs);

	if(!(font.face = load_font_face(font.source_path, font.base_face_idx))) {
		free_font_resources(&font);
		mem_free(fallbacks);
		res_load_failed(st);
	}

	if(set_font_size(&font, global_font_scale())) {
		free_font_resources(&font);
		mem_free(fallbacks);
		res_load_failed(st);
		return;
	}

	dynarray_ensure_capacity(&font.glyphs, 32);

#ifdef DEBUG
	strlcpy(font.debug_label, st->name, sizeof(font.debug_label));
#endif
	font_set_kerning_enabled(&font, true);
	Font *newfont = memdup(&font, sizeof(font));

	FontLoadData *ld = ALLOC(FontLoadData);
	ld->font = newfont;
	ld->fallbacks = fallbacks;
	res_load_continue_after_dependencies(st, load_font_stage2, ld);
}

static void load_font_stage2(ResourceLoadState *st) {
	FontLoadData *ld = NOT_NULL(st->opaque);
	auto font = ld->font;

	if(ld->fallbacks != NULL) {
		int i = 0;
		for(char *fallback = ld->fallbacks; *fallback != '\0'; fallback += strlen(fallback) + 1) {
			font->fallbacks[i] = res_get_data(RES_FONT, fallback, st->flags);
			i++;
		}
	}
	mem_free(ld->fallbacks);
	mem_free(ld);

	if(st->flags & RESF_RELOAD) {
		// workaround to avoid data race (font in use on main thread)
		res_load_continue_on_main(st, finish_reload, font);
	} else {
		res_load_finished(st, font);
	}
}

static void finish_reload(ResourceLoadState *st) {
	res_load_finished(st, st->opaque);
}

void unload_font(void *vfont) {
	free_font_resources(vfont);
	mem_free(vfont);
}

struct rlfonts_arg {
	float quality;
};

attr_nonnull(1)
static void reload_font(Font *font, float quality) {
	if(font->metrics.scale != quality) {
		wipe_glyph_cache(font);
		set_font_size(font, quality);
	}
}

static void *reload_font_callback(const char *name, Resource *res, void *varg) {
	struct rlfonts_arg *a = varg;
	reload_font((Font*)res->data, a->quality);
	return NULL;
}

static void reload_fonts(float quality) {
	res_for_each(RES_FONT, reload_font_callback, &(struct rlfonts_arg) { quality });
}

static inline float apply_kerning(Font *font, uint prev_index, Glyph *gthis) {
	FT_Vector kvec;

	if(!font_get_kerning_enabled(font) || prev_index == 0) {
		return 0;
	}

	if(!FT_Get_Kerning(font->face, prev_index, gthis->ft_index, FT_KERNING_UNFITTED, &kvec)) {
		return f26dot6_to_float(kvec.x);
	}

	return 0;
}

static float apply_subpixel_shift(Glyph *g, float prev_rsb_delta) {
	float shift = 0;
	float diff = prev_rsb_delta - g->metrics.lsb_delta;

#if 0
	if(diff > 32 * FIXED_TO_FLOAT) {
		shift = -FIXED_TO_FLOAT;
	} else if(diff < -31 * FIXED_TO_FLOAT) {
		shift = FIXED_TO_FLOAT;
	}
#else
	shift -= diff;
#endif

	return shift;
}

typedef struct Cursor {
	Font *font;
	float x;
	float prev_rsb_delta;
	uint prev_glyph_idx;
} Cursor;

static Cursor cursor_init(Font *font) {
	return (Cursor) { .font = font };
}

static void cursor_reset(Cursor *c) {
	c->x = 0;
	c->prev_rsb_delta = 0;
	c->prev_glyph_idx = 0;
}

static float cursor_advance(Cursor *c, Glyph *g) {
	if(g == NULL) {
		return c->x;
	}

	c->x += apply_kerning(c->font, c->prev_glyph_idx, g);
	c->x += apply_subpixel_shift(g, c->prev_rsb_delta);

	float startpos = c->x;
	c->x += g->metrics.advance;

	c->prev_rsb_delta = g->metrics.rsb_delta;
	c->prev_glyph_idx = g->ft_index;

	return startpos;
}

float text_ucs4_width_raw(Font *font, const uint32_t *text, uint maxlines) {
	const uint32_t *tptr = text;
	uint numlines = 0;
	float width = 0;
	Cursor c = cursor_init(font);

	while(*tptr) {
		uint32_t uchar = *tptr++;

		if(uchar == '\n') {
			if(++numlines == maxlines) {
				break;
			}

			cursor_reset(&c);
			continue;
		}

		cursor_advance(&c, get_glyph(font, uchar));

		if(c.x > width) {
			width = c.x;
		}
	}

	return width;
}

float text_width_raw(Font *font, const char *text, uint maxlines) {
	uint32_t buf[strlen(text) + 1];
	utf8_to_ucs4(text, ARRAY_SIZE(buf), buf);
	return text_ucs4_width_raw(font, buf, maxlines);
}

void text_ucs4_bbox(Font *font, const uint32_t *text, uint maxlines, TextBBox *bbox) {
	const uint32_t *tptr = text;
	uint numlines = 0;

	*bbox = (typeof(*bbox)) {};
	float y = 0;
	Cursor c = cursor_init(font);

	while(*tptr) {
		uint32_t uchar = *tptr++;

		if(uchar == '\n') {
			if(++numlines == maxlines) {
				break;
			}

			y += font->metrics.lineskip;
			cursor_reset(&c);
			continue;
		}

		Glyph *glyph = get_glyph(font, uchar);

		if(glyph == NULL) {
			continue;
		}

		float x = cursor_advance(&c, glyph);
		bbox->x.max = max(bbox->x.max, c.x);

		FloatOffset ofs = glyph->sprite.padding.offset;

		float g_x0 = x + glyph->metrics.bearing_x + ofs.x;
		float g_x1 = g_x0 + max(glyph->metrics.width, glyph->sprite.w);

		bbox->x.max = max(bbox->x.max, g_x0);
		bbox->x.max = max(bbox->x.max, g_x1);
		bbox->x.min = min(bbox->x.min, g_x0);
		bbox->x.min = min(bbox->x.min, g_x1);

		float g_y0 = y - glyph->metrics.bearing_y + ofs.y;
		float g_y1 = g_y0 + max(glyph->metrics.height, glyph->sprite.h);

		bbox->y.max = max(bbox->y.max, g_y0);
		bbox->y.max = max(bbox->y.max, g_y1);
		bbox->y.min = min(bbox->y.min, g_y0);
		bbox->y.min = min(bbox->y.min, g_y1);
	}
}

void text_bbox(Font *font, const char *text, uint maxlines, TextBBox *bbox) {
	uint32_t buf[strlen(text) + 1];
	utf8_to_ucs4(text, ARRAY_SIZE(buf), buf);
	text_ucs4_bbox(font, buf, maxlines, bbox);
}

float text_ucs4_width(Font *font, const uint32_t *text, uint maxlines) {
	return text_ucs4_width_raw(font, text, maxlines) / font->metrics.scale;
}

float text_width(Font *font, const char *text, uint maxlines) {
	return text_width_raw(font, text, maxlines) / font->metrics.scale;
}

float text_ucs4_height_raw(Font *font, const uint32_t *text, uint maxlines) {
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

float text_height_raw(Font *font, const char *text, uint maxlines) {
	uint32_t buf[strlen(text) + 1];
	utf8_to_ucs4(text, ARRAY_SIZE(buf), buf);
	return text_ucs4_height_raw(font, buf, maxlines);
}

float text_ucs4_height(Font *font, const uint32_t *text, uint maxlines) {
	return text_ucs4_height_raw(font, text, maxlines) / font->metrics.scale;
}

float text_height(Font *font, const char *text, uint maxlines) {
	return text_height_raw(font, text, maxlines) / font->metrics.scale;
}

static inline void adjust_xpos(
	Font *font, const uint32_t *ucs4text, Alignment align, float x_orig, float *x
) {
	float line_width;

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

ShaderProgram *text_get_default_shader(void) {
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
static float _text_ucs4_draw(Font *font, const uint32_t *ucs4text, const TextParams *params) {
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

	TextBBox bbox;
	float y = params->pos.y;
	float scale = font->metrics.scale;
	float iscale = 1.0f / scale;

	Cursor c = cursor_init(font);
	c.x = params->pos.x;

	struct {
		struct { float min, max; } x, y;
		float w, h;
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
		shader_params = (typeof(shader_params)) {};
	} else {
		shader_params = *params->shader_params;
	}

	mat4 mat_texture;
	mat4 mat_model;

	r_mat_tex_current(mat_texture);
	r_mat_mv_current(mat_model);

	glm_translate(mat_model, (vec3) { c.x, y } );
	glm_scale(mat_model, (vec3) { iscale, iscale, 1 } );

	float orig_x = c.x;
	float orig_y = y;
	c.x = y = 0;

	adjust_xpos(font, ucs4text, params->align, 0, &c.x);

	if(params->overlay_projection) {
		FloatRect *op = params->overlay_projection;
		overlay.x.min = (op->x - orig_x) * scale;
		overlay.x.max = overlay.x.min + op->w * scale;
		overlay.y.min = (op->y - orig_y) * scale;
		overlay.y.max = overlay.y.min + op->h * scale;
	} else {
		overlay.x.min = bbox.x.min + c.x;
		overlay.x.max = bbox.x.max + c.x;
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

	const uint32_t *tptr = ucs4text;

	while(*tptr) {
		uint32_t uchar = *tptr++;

		if(uchar == '\n') {
			cursor_reset(&c);
			adjust_xpos(font, tptr, params->align, 0, &c.x);
			y += font->metrics.lineskip;
			continue;
		}

		Glyph *glyph = get_glyph(font, uchar);

		if(glyph == NULL) {
			continue;
		}

		float x = cursor_advance(&c, glyph);

		if(glyph->sprite.tex == NULL) {
			continue;
		}

		Sprite *spr = &glyph->sprite;
		set_batch_texture(&batch_state_params, spr->tex);

		SpriteInstanceAttribs attribs;
		attribs.rgba = color;
		attribs.custom = shader_params;

		FloatOffset ofs = spr->padding.offset;
		FloatExtent imgdims = spr->extent;
		imgdims.as_cmplx -= spr->padding.extent.as_cmplx;

		float g_x = x + glyph->metrics.bearing_x + spr->w * 0.5f + ofs.x;
		float g_y = y - glyph->metrics.bearing_y + spr->h * 0.5f - font->metrics.descent + ofs.y;

		glm_translate_to(mat_texture, (vec3) {
			g_x - imgdims.w * 0.5f,
			g_y * texmat_offset_sign + overlay.h - imgdims.h * 0.5f
		}, attribs.tex_transform);
		glm_scale(attribs.tex_transform, (vec3) { imgdims.w, imgdims.h, 1.0 });

		glm_translate_to(mat_model, (vec3) { g_x, g_y }, attribs.mv_transform);
		glm_scale(attribs.mv_transform, (vec3) { imgdims.w, imgdims.h, 1.0 } );

		attribs.texrect = spr->tex_area;

		// NOTE: Glyphs have their sprite w/h unadjusted for scale.
		attribs.sprite_size.w = imgdims.w * iscale;
		attribs.sprite_size.h = imgdims.h * iscale;

		if(params->glyph_callback.func != NULL) {
			params->glyph_callback.func(
				font, uchar, &attribs, params->glyph_callback.userdata);
		}

		r_sprite_batch_add_instance(&attribs);
	}

	return c.x * iscale;
}

static float _text_draw(Font *font, const char *text, const TextParams *params) {
	uint32_t buf[strlen(text) + 1];
	utf8_to_ucs4(text, ARRAY_SIZE(buf), buf);

	if(params->max_width > 0) {
		text_ucs4_shorten(font, buf, params->max_width);
	}

	return _text_ucs4_draw(font, buf, params);
}

float text_draw(const char *text, const TextParams *params) {
	return _text_draw(font_from_params(params), text, params);
}

float text_ucs4_draw(const uint32_t *text, const TextParams *params) {
	Font *font = font_from_params(params);

	if(params->max_width > 0) {
		uint32_t buf[ucs4len(text) + 1];
		memcpy(buf, text, sizeof(buf));
		text_ucs4_shorten(font, buf, params->max_width);
		return _text_ucs4_draw(font, buf, params);
	}

	return _text_ucs4_draw(font, text, params);
}

float text_draw_wrapped(const char *text, float max_width, const TextParams *params) {
	Font *font = font_from_params(params);
	char buf[strlen(text) * 2 + 1];
	text_wrap(font, text, max_width, buf, sizeof(buf));
	return _text_draw(font, buf, params);
}

void text_render(const char *text, Font *font, Sprite *out_sprite, TextBBox *out_bbox) {
	text_bbox(font, text, 0, out_bbox);

	float bbox_width = out_bbox->x.max - out_bbox->x.min;
	float bbox_height = out_bbox->y.max - out_bbox->y.min;

	if(bbox_height < font->metrics.max_glyph_height) {
		out_bbox->y.min -= font->metrics.max_glyph_height - bbox_height;
		bbox_height = out_bbox->y.max - out_bbox->y.min;
	}

	Texture *tex = globals.render_tex;

	uint tex_new_w = ceilf(bbox_width); // max(tex->w, bbox_width);
	uint tex_new_h = ceilf(bbox_height); // max(tex->h, bbox_height);
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

		r_texture_set_debug_label(tex, "Font render texture");
		r_framebuffer_attach(globals.render_buf, tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
		r_framebuffer_viewport(globals.render_buf, 0, 0, tex_new_w, tex_new_h);
	}

	r_state_push();

	r_framebuffer(globals.render_buf);
	r_clear(BUFFER_COLOR, RGBA(0, 0, 0, 0), 1);

	r_blend(BLEND_PREMUL_ALPHA);
	r_disable(RCAP_CULL_FACE);
	r_disable(RCAP_DEPTH_TEST);

	r_mat_mv_push_identity();
	r_mat_proj_push_ortho(tex_new_w, tex_new_h);
	r_mat_tex_push_identity();

	// HACK: Coordinates are in texel space, font scale must not be used.
	// This probably should be exposed in the text_draw API.
	float fontscale = font->metrics.scale;
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
	out_sprite->tex_area.w = bbox_width / tex_new_w;
	out_sprite->tex_area.h = bbox_height / tex_new_h;
	out_sprite->tex_area.x = 0;
	out_sprite->tex_area.y = 0;
	out_sprite->w = bbox_width / font->metrics.scale;
	out_sprite->h = bbox_height / font->metrics.scale;
}

void text_ucs4_shorten(Font *font, uint32_t *text, float width) {
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

void text_wrap(Font *font, const char *src, float width, char *buf, size_t bufsize) {
	assert(bufsize > strlen(src) + 1);
	assert(width > 0);

	char src_copy[strlen(src) + 1];
	char *sptr = src_copy;
	char *next = NULL;
	char *curline = buf;

	strcpy(src_copy, src);
	*buf = 0;

	while((next = strtok_r(NULL, " \t", &sptr))) {
		float curwidth;

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

		float totalwidth = text_width(font, tmpbuf, 0);

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

const FontMetrics *font_get_metrics(Font *font) {
	return &font->metrics;
}

float font_get_lineskip(Font *font) {
	return font->metrics.lineskip / font->metrics.scale;
}

float font_get_ascent(Font *font) {
	return font->metrics.descent / font->metrics.scale;
}

float font_get_descent(Font *font) {
	return font->metrics.descent / font->metrics.scale;
}

const GlyphMetrics *font_get_char_metrics(Font *font, charcode_t c) {
	Glyph *g = get_glyph(font, c);

	if(!g) {
		return NULL;
	}

	return &g->metrics;
}

static bool transfer_font(void *dst, void *src) {
	auto dfont = CASTPTR_ASSUME_ALIGNED(dst, Font);
	auto sfont = CASTPTR_ASSUME_ALIGNED(src, Font);
	free_font_resources(dfont);
	*dfont = *sfont;
	mem_free(sfont);
	return true;
}
