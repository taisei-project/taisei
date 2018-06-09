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

struct Font {
	FT_Face face;
	char *source_path;
	int base_size;
	long base_face_idx;
	double scale;
	FT_Fixed fixed_scale;

	struct {
		int ascent;
		int descent;
		int max_glyph_height;
		int lineskip;
	} metrics;

	Glyph *glyphs;
	uint glyphs_allocated;
	uint glyphs_used;

	Hashtable *charcodes_to_glyph_ofs;
	Texture **textures;
	uint num_textures;
};

typedef struct SpriteSheet {
	LIST_INTERFACE(struct SpriteSheet);
	Texture *tex;
	RectPack *rectpack;
	uint glyphs;
} SpriteSheet;

typedef LIST_ANCHOR(SpriteSheet) SpriteSheetAnchor;

struct Fonts _fonts;

static struct {
	FT_Library lib;
	struct {
		SDL_mutex *new_face;
		SDL_mutex *done_face;
	} mutex;
} globals;

static double global_font_scale(void) {
	return sanitize_scale(video.quality_factor * config_get_float(CONFIG_TEXT_QUALITY));
}

static void reload_fonts(double quality);

static bool fonts_event(SDL_Event *event, void *arg) {
	reload_fonts(global_font_scale());
	return false;
}

static void fonts_quality_config_callback(ConfigIndex idx, ConfigValue v) {
	config_set_float(idx, v.f);
	reload_fonts(global_font_scale());
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
	FT_Done_FreeType(globals.lib);
	SDL_DestroyMutex(globals.mutex.new_face);
	SDL_DestroyMutex(globals.mutex.done_face);
}

void test_rectpack_stuff(void) {
	// abort();
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

	ssize_t stream_size = SDL_RWsize(rwops);

	if(stream_size < 0) {
		// "compressed stream" hint
		ftstream->size = 0x7FFFFFFF;
	} else {
		ftstream->size = stream_size;
	}

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

	fnt->scale = scale;
	fnt->fixed_scale = fixed_scale;

	// Based on SDL_ttf
	FT_Face face = fnt->face;
	fixed_scale = FT_MulFix(face->size->metrics.y_scale, fixed_scale);
	fnt->metrics.ascent = FT_CEIL(FT_MulFix(face->ascender, fixed_scale));
	fnt->metrics.descent = FT_CEIL(FT_MulFix(face->descender, fixed_scale));
	fnt->metrics.max_glyph_height = fnt->metrics.ascent - fnt->metrics.descent + 1; // FIXME: is this +1 needed?
	fnt->metrics.lineskip = FT_CEIL(FT_MulFix(face->height, fixed_scale));

	return err;
}

#define SS_WIDTH 1024
#define SS_HEIGHT 1024

static SpriteSheet* add_spritesheet(SpriteSheetAnchor *spritesheets) {
	SpriteSheet *ss = calloc(1, sizeof(SpriteSheet));
	ss->tex = calloc(1, sizeof(Texture));
	ss->rectpack = rectpack_new(SS_WIDTH, SS_HEIGHT);

	r_texture_create(ss->tex, &(TextureParams) {
		.width = SS_WIDTH,
		.height = SS_HEIGHT,
		.type = TEX_TYPE_R,
		.filter.upscale = TEX_FILTER_LINEAR,
		.filter.downscale = TEX_FILTER_LINEAR,
		.wrap.s = TEX_WRAP_CLAMP,
		.wrap.t = TEX_WRAP_CLAMP,
	});

	RenderTarget *prev_target = r_target_current();
	RenderTarget atlast_rt;
	Color cc_prev = r_clear_color_current();
	r_target_create(&atlast_rt);
	r_target_attach(&atlast_rt, ss->tex, RENDERTARGET_ATTACHMENT_COLOR0);
	r_target(&atlast_rt);
	r_clear_color4(0, 0, 0, 0);
	r_clear(CLEAR_COLOR);
	r_target(prev_target);
	r_clear_color(cc_prev);
	r_target_destroy(&atlast_rt);

	alist_append(spritesheets, ss);
	return ss;
}

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
		ss->tex,
		rect_x(sprite_pos),
		rect_y(sprite_pos),
		bitmap.width,
		bitmap.rows,
		bitmap.buffer
	);

	glyph->sprite.tex = ss->tex;
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
	// CAUTION: Do not destroy or free the texture here; the font will take ownership of it.
	// Sprite sheets are transient objects that only exist to set up the glyph sprites and
	// their backing textures.

	rectpack_free(ss->rectpack);
	alist_unlink(spritesheets, ss);
	free(ss);
}

static Glyph* load_glyph(Font *font, FT_UInt gindex, SpriteSheetAnchor *spritesheets) {
	if(++font->glyphs_used == font->glyphs_allocated) {
		font->glyphs_allocated *= 2;
		font->glyphs = realloc(font->glyphs, sizeof(Glyph) * font->glyphs_allocated);
	}

	Glyph *glyph = font->glyphs + font->glyphs_used - 1;
	FT_Error err = FT_Load_Glyph(font->face, gindex, FT_LOAD_RENDER);

	if(err) {
		log_warn("FT_Load_Glyph(%u) failed: %s", gindex, ft_error_str(err));
		--font->glyphs_used;
		return NULL;
	}

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

	glyph->metrics.bearing_x = FT_FLOOR(font->face->glyph->metrics.horiBearingX);
	glyph->metrics.bearing_y = FT_FLOOR(font->face->glyph->metrics.horiBearingY);
	glyph->metrics.width = FT_CEIL(font->face->glyph->metrics.width);
	glyph->metrics.height = FT_CEIL(font->face->glyph->metrics.height);
	glyph->metrics.advance = FT_CEIL(font->face->glyph->metrics.horiAdvance);

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

	glyph->ft_index = gindex;
	return glyph;
}

static void load_glyphs(Font *font) {
	font->glyphs_allocated = 128;
	font->glyphs_used = 0;
	font->glyphs = malloc(sizeof(Glyph) * font->glyphs_allocated);

	font->charcodes_to_glyph_ofs = hashtable_new(NULL, hashtable_hashfunc_int, NULL, NULL);
	Hashtable *indices_to_glyph_ofs = hashtable_new(NULL, hashtable_hashfunc_int, NULL, NULL);
	SpriteSheetAnchor spritesheets = { .first = NULL };

	charcode_t charcode;
	uint gindex;
	uint num_charcodes = 0;
	uint num_glyphs = 0;

	charcode = FT_Get_First_Char(font->face, &gindex);

	while(gindex != 0) {
		Glyph *glyph;
		uint64_t gofs = hashtable_get(indices_to_glyph_ofs, gindex).uint64;

		log_debug("charcode=%lu gindex=%u gofs=%lu", charcode, gindex, gofs);

		if(gofs == 0) {
			glyph = load_glyph(font, gindex, &spritesheets);

			if(glyph == NULL) {
				continue;
			}

			gofs = (ptrdiff_t)(glyph - font->glyphs) + 1;
			hashtable_set(indices_to_glyph_ofs, gindex, gofs);
			log_debug("gofs=%lu", gofs);
			++num_glyphs;
		}

		hashtable_set(font->charcodes_to_glyph_ofs, charcode, gofs);
		charcode = FT_Get_Next_Char(font->face, charcode, &gindex);
		++num_charcodes;
	}

	hashtable_free(indices_to_glyph_ofs);

	if(font->glyphs_used < font->glyphs_allocated) {
		font->glyphs = realloc(font->glyphs, sizeof(Glyph) * font->glyphs_used);
		font->glyphs_allocated = font->glyphs_used;
	}

	uint num_textures = 0;

	for(SpriteSheet *ss = spritesheets.first; ss; ss = ss->next) {
		if(ss->glyphs) {
			num_textures++;
		}
	}

	if(num_textures == 0) {
		log_warn("No usable glyphs in font at all!");
	} else {
		font->num_textures = num_textures;
		font->textures = calloc(num_textures, sizeof(Texture*));
		uint i = 0;

		for(SpriteSheet *ss = spritesheets.first, *next; ss; ss = next) {
			next = ss->next;

			if(ss->glyphs) {
				font->textures[i++] = ss->tex;
				delete_spritesheet(&spritesheets, ss);
			}
		}
	}

	log_debug("num_charcodes=%u num_glyphs=%u", num_charcodes, num_glyphs);
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

	hashtable_free(font->charcodes_to_glyph_ofs);
	free(font->source_path);
	free(font->glyphs);

	if(font->textures) {
		for(uint i = 0; i < font->num_textures; ++i) {
			Texture *tex = font->textures[i];
			r_texture_destroy(tex);
			free(tex);
		}

		free(font->textures);
	}
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

	if(!(font.face = load_font_face(font.source_path, font.base_face_idx))) {
		free_font_resources(&font);
		return NULL;
	}

	if(set_font_size(&font, font.base_size, global_font_scale())) {
		free_font_resources(&font);
		return NULL;
	}

	return memdup(&font, sizeof(font));
}

void* load_font_end(void *opaque, const char *path, uint flags) {
	Font *font = opaque;

	if(font == NULL) {
		return NULL;
	}

	load_glyphs(font);
	return font;
}

void unload_font(void *vfont) {
	free_font_resources(vfont);
	free(vfont);
}

static Glyph* get_glyph(Font *fnt, charcode_t cp) {
	uint64_t ofs = hashtable_get_unsafe(fnt->charcodes_to_glyph_ofs, cp).uint64;

	if(ofs == 0) {
		return NULL;
	}

	return fnt->glyphs + (ofs - 1);
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

static void reload_font(Font *font) {
	// font->ttf = load_ttf(font->source_path, font->base_size);
}

static void* reload_font_callback(const char *name, Resource *res, void *varg) {
	reload_font((Font*)res->data);
	return NULL;
}

static void reload_fonts(double quality) {
	/*
	if(font_renderer.quality == sanitize_scale(quality)) {
		return;
	}
	*/

	resource_for_each(RES_FONT, reload_font_callback, NULL);
}

#include "hirestime.h"

static void draw_text_line(Alignment align, float x, float y, const char *text, Font *font) {
	const char *tptr = text;
	SpriteParams sp;
	Glyph *prev_glyph;

	if(!*text) {
		return;
	}

	bool scaled = false;

	if(font->scale != 1) {
		double iscale = 1 / font->scale;
		r_mat_push();
		r_mat_translate(x, y, 0);
		r_mat_scale(iscale, iscale, 1);
		x = y = 0;
		scaled = true;
	}

	memset(&sp, 0, sizeof(sp));

	bool keming = FT_HAS_KERNING(font->face);

	/*
	Color c = rgba(1, 1, 1, 1);

	if(keming && ((int)time_get()) % 2) {
		keming = false;
		c = rgba(1, 1, 0.5, 1);
	}
	*/

	Color c = r_color_current();

	while(*tptr) {
		uint32_t uchar = utf8_getch(&tptr);
		Glyph *glyph = get_glyph(font, uchar);

		log_debug("%08x [%c] %p", uchar, uchar, (void*) glyph);

		if(glyph == NULL) {
			continue;
		}

		if(keming && prev_glyph) {
			FT_Vector kvec;

			if(!FT_Get_Kerning(font->face, prev_glyph->ft_index, glyph->ft_index, FT_KERNING_DEFAULT, &kvec)) {
				x += kvec.x >> 6;
				y += kvec.y >> 6;
			}
		}

		sp.sprite_ptr = &glyph->sprite;
		sp.shader = "sprite_text_test";
		sp.color = c; // r_color_current();
		sp.pos.x = x + glyph->metrics.bearing_x + glyph->sprite.w * 0.5;
		sp.pos.y = y - glyph->metrics.bearing_y + glyph->sprite.h * 0.5;
		r_draw_sprite(&sp);

		x += glyph->metrics.advance;
		prev_glyph = glyph;
	}

	if(scaled) {
		r_mat_pop();
	}
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
		draw_text(align, x, y + font->metrics.lineskip / font->scale, nl+1, font);
		*nl = '\0';
	}

	draw_text_line(align, x, y, buf, font);
	free(buf);
}

void render_text(const char *text, Font *font, Sprite *out_spr) {

}

void draw_text_auto_wrapped(Alignment align, float x, float y, const char *text, int width, Font *font) {
	char buf[strlen(text) * 2];
	wrap_text(buf, sizeof(buf), text, width, font);
	draw_text(align, x, y, buf, font);
}

static void string_dimensions(char *s, Font *font, int *w, int *h) {
	if(w) {
		*w = 8;
	}

	if(h) {
		*h = 8;
	}
}

int stringwidth(char *s, Font *font) {
	int w;
	string_dimensions(s, font, &w, NULL);
	return w/* / font_renderer.quality*/;
}

int stringheight(char *s, Font *font) {
	int h;
	string_dimensions(s, font, NULL, &h);
	return h/* / font_renderer.quality*/;
}

int charwidth(char c, Font *font) {
	char s[2];
	s[0] = c;
	s[1] = 0;
	return stringwidth(s, font);
}

int font_line_spacing(Font *font) {
	// return TTF_FontLineSkip(font->ttf)/* / font_renderer.quality*/;
	return 8;
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
