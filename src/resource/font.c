/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

/*
#include <ft2build.h>
#include FT_FREETYPE_H
*/

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

enum {
	FONT_MAX_PAGES = (1 << 5),
	FONT_MAX_GLYPHS = (1 << 12),
};

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

typedef struct Glyph {
	Sprite sprite;
	GlyphMetrics metrics;
	uint32_t codepoint;
	uint page_num;
	float scale;
} Glyph;

struct Font {
	Glyph *glyphs;
	ht_int2ptr_t glyph_table;
	ht_int2int_t kern_table;
	FontMetrics metrics;
	uint num_glyphs;
	uint num_pages;
};

static struct {
	ShaderProgram *default_shader;
	Texture *render_tex;
	Framebuffer *render_buf;
} globals;

static inline int reinterpret_float2int(float val) {
	union IntFloatConv {
		int intval;
		float fltval;
	} conv;
	conv.fltval = val;
	return conv.intval;
}

static inline float reinterpret_int2float(int val) {
	union IntFloatConv {
		int intval;
		float fltval;
	} conv;
	conv.intval = val;
	return conv.fltval;
}

#define KERN_KEY(left_cp, right_cp) ((uint64_t)(left_cp) | ((uint64_t)(right_cp) << 32))

static double global_font_scale(void) {
	int w, h;
	video_get_viewport_size(&w, &h);
	return sanitize_scale(((double)h / SCREEN_H) * config_get_float(CONFIG_TEXT_QUALITY));
}

static void init_fonts(void) {
	preload_resources(RES_FONT, RESF_PERMANENT,
		"standard",
	NULL);

	// WARNING: Preloading the default shader here is unsafe.

	globals.render_tex = r_texture_create(&(TextureParams) {
		.filter = { TEX_FILTER_LINEAR, TEX_FILTER_LINEAR },
		.wrap   = { TEX_WRAP_CLAMP, TEX_WRAP_CLAMP },
		.type   = TEX_TYPE_R,
		.stream = true,
		.width  = 1024,
		.height = 1024,
	});

	r_texture_set_debug_label(globals.render_tex, "Font render texture");

	globals.render_buf = r_framebuffer_create();
	r_framebuffer_attach(globals.render_buf, globals.render_tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_viewport(globals.render_buf, 0, 0, 1024, 1024);
	r_framebuffer_set_debug_label(globals.render_buf, "Font render Framebuffer");
}

static void post_init_fonts(void) {
	globals.default_shader = get_resource_data(RES_SHADER_PROGRAM, "text_default", RESF_PERMANENT | RESF_PRELOAD);
}

static void shutdown_fonts(void) {
	r_texture_destroy(globals.render_tex);
	r_framebuffer_destroy(globals.render_buf);
}

static char* font_path(const char *name) {
	return strjoin(FONT_PATH_PREFIX, name, FONT_EXTENSION, NULL);
}

bool check_font_path(const char *path) {
	return strstartswith(path, FONT_PATH_PREFIX) && strendswith(path, FONT_EXTENSION);
}

static void free_font_resources(Font *font) {
	ht_destroy(&font->glyph_table);
	ht_destroy(&font->kern_table);
	free(font->glyphs);
}

typedef struct FontParseState {
	Font *font;
	Glyph *current_glyph;
	char *font_basename;
	uint32_t load_flags;
	char **page_names;
} FontParseState;

static bool parse_codepoint(const char *s, uint32_t *out_cp) {
	char *end;
	uint64_t intval = strtoll(s, &end, 0);

	if(s == end) {
		log_warn("Invalid codepoint %s", s);
		return false;
	}

	if(intval > UINT32_MAX) {
		log_warn("Codepoint %"PRIX64" out of range", intval);
		return false;
	}

	*out_cp = intval;
	return true;
}

static void parse_fontdef_finalize_glyph(FontParseState *st) {
	if(st->current_glyph == NULL || st->current_glyph->page_num == FONT_MAX_PAGES) {
		return;
	}

	st->current_glyph->sprite.w = st->current_glyph->sprite.tex_area.w / st->font->metrics.texture_scale;
	st->current_glyph->sprite.h = st->current_glyph->sprite.tex_area.h / st->font->metrics.texture_scale;
}

static bool parse_fontdef(const char *key, const char *val, void *data) {
	FontParseState *st = data;

	if(!strcmp(key, "@glyph")) {
		parse_fontdef_finalize_glyph(st);

		if(st->current_glyph == NULL) {
			if(!st->page_names) {
				log_warn("No pages defined");
				return false;
			}

			if(!st->font->glyphs) {
				log_warn("num_glyphs missing");
				return false;
			}

			st->current_glyph = st->font->glyphs;
		} else {
			st->current_glyph++;
		}

		assert(st->current_glyph != NULL);

		if(st->font->glyphs - st->current_glyph >= st->font->num_glyphs) {
			log_warn("Too many glyphs defined (num_glyphs promised %d)", st->font->num_glyphs);
			return false;
		}

		uint32_t codepoint;

		if(!parse_codepoint(val, &codepoint)) {
			return false;
		}

		if(!ht_try_set(&st->font->glyph_table, codepoint, st->current_glyph, NULL, NULL)) {
			log_warn("Duplicate glyph for codepoint %08X", codepoint);
			return false;
		}

		st->current_glyph->page_num = FONT_MAX_PAGES;
		return true;
	}

	if(st->current_glyph == NULL) {
		/*
		 * Parse globals
		 */

		if(!strcmp(key, "num_pages")) {
			if(st->page_names != NULL) {
				log_warn("num_pages may not be redefined");
				return false;
			}

			int p = strtol(val, NULL, 0);

			if(p < 1) {
				log_warn("Must have at least 1 page");
				return false;
			}

			if(p > FONT_MAX_PAGES) {
				log_warn("Can't have more than %u pages", FONT_MAX_PAGES);
				return false;
			}

			st->font->num_pages = p;
			st->page_names = calloc(st->font->num_pages, sizeof(*st->page_names));

			return true;
		}

		if(!strcmp(key, "num_glyphs")) {
			if(st->font->glyphs != NULL) {
				log_warn("num_glyphs may not be redefined");
				return false;
			}

			int p = strtol(val, NULL, 0);

			if(p < 1) {
				log_warn("Must have at least 1 glyph");
				return false;
			}

			if(p > FONT_MAX_GLYPHS) {
				log_warn("Can't have more than %u glyphs", FONT_MAX_GLYPHS);
				return false;
			}

			st->font->num_glyphs = p;
			st->font->glyphs = calloc(st->font->num_glyphs, sizeof(*st->font->glyphs));

			return true;
		}

		if(!strncmp(key, "page ", 5)) {
			char *end;
			int p = strtol(key + 5, &end, 0);

			if(key + 5 == end || p < 0) {
				log_warn("Invalid page number %s", key + 5);
			}

			if(p >= st->font->num_pages) {
				log_warn("Page number out of range (num_pages promised %u pages)", st->font->num_pages);
			}

			st->page_names[p] = strdup(val);
			preload_resource(RES_TEXTURE, st->page_names[p], st->load_flags);

			return true;
		}

		return apply_keyvalue_specs(key, val, (KVSpec[]) {
			{ "ascender",      .out_float = &st->font->metrics.ascent        },
			{ "descender",     .out_float = &st->font->metrics.descent       },
			{ "lineskip",      .out_float = &st->font->metrics.lineskip      },
			{ "texture_scale", .out_float = &st->font->metrics.texture_scale },
			{ NULL }
		});
	} else {
		/*
		 * Parse glyph
		 */

		if(!strcmp(key, "page")) {
			char *end;
			int page_num = strtol(val, &end, 0);

			if(val == end) {
				log_warn("Invalid page number %s", val);
				return false;
			}

			if(page_num >= st->font->num_pages) {
				log_warn("Page number out of range (num_pages is %d)", st->font->num_pages);
				return false;
			}

			st->current_glyph->page_num = page_num;
			return true;
		}

		if(!strncmp(key, "kern ", 5)) {
			uint32_t codepoint;

			if(!parse_codepoint(key + 5, &codepoint)) {
				return false;
			}

			uint64_t kern_key = KERN_KEY(st->current_glyph->codepoint, codepoint);
			uint32_t kern_val = reinterpret_float2int(strtod(val, NULL));

			ht_set(&st->font->kern_table, kern_key, kern_val);
			return true;
		}

		return apply_keyvalue_specs(key, val, (KVSpec[]) {
			{ "bearing_x",        .out_float = &st->current_glyph->metrics.bearing_x },
			{ "bearing_y",        .out_float = &st->current_glyph->metrics.bearing_y },
			{ "advance",          .out_float = &st->current_glyph->metrics.advance   },
			{ "width",            .out_float = &st->current_glyph->metrics.width     },
			{ "height",           .out_float = &st->current_glyph->metrics.height    },
			{ "texregion_width",  .out_float = &st->current_glyph->sprite.tex_area.w },
			{ "texregion_height", .out_float = &st->current_glyph->sprite.tex_area.h },
			{ "texregion_x",      .out_float = &st->current_glyph->sprite.tex_area.x },
			{ "texregion_y",      .out_float = &st->current_glyph->sprite.tex_area.y },
			{ NULL }
		});
	}

	return false;
}

static bool load_font_finalize(FontParseState *st) {
	assert(st->page_names != NULL);
	assert(st->font->glyphs != NULL);
	assert(st->font->num_pages != 0);

	parse_fontdef_finalize_glyph(st);

	Texture *pages[st->font->num_pages];

	for(uint i = 0; i < st->font->num_pages; ++i) {
		char *tex_name = st->page_names[i];

		if(tex_name == NULL) {
			log_warn("Page %i is undefined", i);
			return false;
		}

		pages[i] = get_resource_data(RES_TEXTURE, tex_name, st->load_flags);
		free(tex_name);
		st->page_names[i] = NULL;
	}

	free(st->font_basename);
	free(st->page_names);

	for(uint i = 0; i < st->font->num_glyphs; ++i) {
		uint pnum = st->font->glyphs[i].page_num;

		if(pnum != FONT_MAX_PAGES) {
			assert(pnum < st->font->num_pages);
			st->font->glyphs[i].sprite.tex = pages[pnum];

			// FIXME: is there a better way?
			if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
				Sprite *spr = &st->font->glyphs[i].sprite;
				uint tex_h = r_texture_get_height(spr->tex, 0);
				spr->tex_area.y = tex_h - spr->tex_area.y - spr->tex_area.h;
			}
		}
	}

	return true;
}

void* load_font_begin(const char *path, uint flags) {
	FontParseState pstate = { 0 };
	pstate.font = calloc(1, sizeof(Font));
	pstate.font->metrics.texture_scale = 1;
	pstate.font_basename = resource_util_basename(FONT_PATH_PREFIX, path);
	pstate.load_flags = flags;

	ht_create(&pstate.font->glyph_table);
	ht_create(&pstate.font->kern_table);

	if(
		parse_keyvalue_file_cb(path, parse_fontdef, &pstate) &&
		load_font_finalize(&pstate)
	) {
		return pstate.font;
	}

	log_warn("Failed to parse font file '%s'", path);

	for(uint i = 0; i < pstate.font->num_pages; ++i) {
		free(pstate.page_names[i]);
	}

	free(pstate.font_basename);
	free(pstate.page_names);
	free_font_resources(pstate.font);
	free(pstate.font);
	return NULL;
}

void* load_font_end(void *opaque, const char *path, uint flags) {
	return opaque;
}

void unload_font(void *vfont) {
	free_font_resources(vfont);
	free(vfont);
}

static Glyph* get_glyph(Font *font, uint32_t uchar) {
	Glyph *glyph = ht_get(&font->glyph_table, uchar, NULL);

	if(glyph == NULL) {
		glyph = ht_get(&font->glyph_table, UNICODE_UNKNOWN, NULL);
	}

	if(glyph == NULL) {
		glyph = ht_get(&font->glyph_table, 0, NULL);
	}

	return glyph;
}

static inline float apply_kerning(Font *font, Glyph *gprev, Glyph *gthis) {
	// return 0;

	if(gprev == NULL) {
		return 0;
	}

	return ht_get(&font->kern_table, KERN_KEY(gprev->codepoint, gthis->codepoint), 0);
}

float text_width_raw(Font *font, const char *text, uint maxlines) {
	const char *tptr = text;
	Glyph *prev_glyph = NULL;
	bool keming = true;
	uint numlines = 0;
	float x = 0;
	float width = 0;

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

		if(keming) {
			x += apply_kerning(font, prev_glyph, glyph);
		}

		x += glyph->metrics.advance;
		prev_glyph = glyph;
	}

	if(x > width) {
		width = x;
	}

	return width;
}

void text_bbox(Font *font, const char *text, uint maxlines, BBox *bbox) {
	const char *tptr = text;
	Glyph *prev_glyph = NULL;
	bool keming = true;
	uint numlines = 0;

	memset(bbox, 0, sizeof(*bbox));
	float x = 0, y = 0;

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

		if(keming) {
			x += apply_kerning(font, prev_glyph, glyph);
		}

		float g_x0 = x + glyph->metrics.bearing_x;
		float g_x1 = g_x0 + glyph->metrics.width;
		float g_x2 = x + glyph->metrics.advance;

		bbox->x.max = max(bbox->x.max, g_x0);
		bbox->x.max = max(bbox->x.max, g_x1);
		bbox->x.max = max(bbox->x.max, g_x2);
		bbox->x.min = min(bbox->x.min, g_x0);
		bbox->x.min = min(bbox->x.min, g_x1);
		bbox->x.min = min(bbox->x.min, g_x2);

		float g_y0 = y - glyph->metrics.bearing_y;
		float g_y1 = g_y0 + glyph->metrics.height;

		bbox->y.max = max(bbox->y.max, g_y0);
		bbox->y.max = max(bbox->y.max, g_y1);
		bbox->y.min = min(bbox->y.min, g_y0);
		bbox->y.min = min(bbox->y.min, g_y1);

		prev_glyph = glyph;
		x += glyph->metrics.advance;
	}
}

float text_width(Font *font, const char *text, uint maxlines) {
	return text_width_raw(font, text, maxlines);
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
	return text_height_raw(font, text, maxlines);
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
// #define TEXT_DRAW_GLYPH_BBOX

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
	double iscale = 1; // 1 / font->metrics.scale;

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
	r_color(color_mul(RGBA(0.5, 0.5, 0.5, 0.5), r_color_current()));
	r_draw_quad();
	r_mat_pop();
	r_state_pop();
	#endif

	r_mat_mode(MM_TEXTURE);
	r_mat_push();
	r_mat_translate(0, 0.0, 0);
	r_mat_scale(1/bbox_w, 1/bbox_h, 1.0);
	r_mat_translate(-bbox.x.min - (x - x_orig), -bbox.y.min + font->metrics.descent, 0);

	// FIXME: is there a better way?
	float texmat_offset_sign;

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		texmat_offset_sign = -1;
	} else {
		texmat_offset_sign = 1;
	}

	bool keming = true;
	Glyph *prev_glyph = NULL;
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

		if(keming) {
			x += apply_kerning(font, prev_glyph, glyph);
		}

		if(glyph->sprite.tex != NULL) {
			sp.sprite_ptr = &glyph->sprite;
			sp.pos.x = x + glyph->metrics.bearing_x + glyph->metrics.width  * 0.5;
			sp.pos.y = y - glyph->metrics.bearing_y + glyph->metrics.height * 0.5 - font->metrics.descent;

			r_mat_push();
			r_mat_translate(sp.pos.x - x_orig, sp.pos.y * texmat_offset_sign, 0);
			r_mat_scale(sp.sprite_ptr->w, sp.sprite_ptr->h, 1.0);
			r_mat_translate(-0.5, -0.5, 0);

			if(params->glyph_callback.func != NULL) {
				params->glyph_callback.func(font, uchar, &sp, params->glyph_callback.userdata);
			}

			r_draw_sprite(&sp);
			r_mat_pop();
		}

		#ifdef TEXT_DRAW_GLYPH_BBOX
		r_state_push();
		r_mat_mode(MM_MODELVIEW);
		r_state_push();
		r_shader_standard_notex();
		r_mat_push();
		r_mat_translate(x + glyph->metrics.bearing_x + glyph->metrics.width * 0.5, y - glyph->metrics.bearing_y + glyph->metrics.height * 0.5 - font->metrics.descent, 0);
		r_mat_scale(glyph->metrics.width, glyph->metrics.height, 0);
		r_color4(0.0, 0.4, 0.2, 0.4);
		r_draw_quad();
		r_mat_pop();
		r_state_pop();
		r_state_pop();
		#endif

		x += glyph->metrics.advance;
		prev_glyph = glyph;
	}

	r_mat_pop();
	r_mat_mode(MM_MODELVIEW);
	r_mat_pop();
	r_mat_mode(mm_prev);

	return x_orig + (x - x_orig);
}

double text_draw(const char *text, const TextParams *params) {
	/*
	Font *f = get_font("big");
	Glyph *s = get_glyph(f, 'S');
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = &s->sprite,
		.shader = "text_default",
		.pos = { SCREEN_W/2, SCREEN_H/2 }
	});
	return 0;
	*/

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
	// double fontscale = font->metrics.scale;
	// font->metrics.scale = 1;

	text_draw(text, &(TextParams) {
		.font_ptr = font,
		.pos = { -out_bbox->x.min, -out_bbox->y.min + font->metrics.descent },
		.color = RGB(1, 1, 1),
		.shader = "text_default",
	});

	// font->metrics.scale = fontscale;
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
	out_sprite->w = bbox_width;
	out_sprite->h = bbox_height;
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
	return font->metrics.lineskip;
}

const GlyphMetrics* font_get_char_metrics(Font *font, charcode_t c) {
	Glyph *g = get_glyph(font, c);

	if(!g) {
		return NULL;
	}

	return &g->metrics;
}
