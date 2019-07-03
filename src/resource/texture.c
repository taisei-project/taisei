/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "texture.h"
#include "resource.h"
#include "global.h"
#include "video.h"
#include "renderer/api.h"
#include "util/pixmap.h"

static void* load_texture_begin(const char *path, uint flags);
static void* load_texture_end(void *opaque, const char *path, uint flags);
static void free_texture(Texture *tex);

ResourceHandler texture_res_handler = {
	.type = RES_TEXTURE,
	.typename = "texture",
	.subdir = TEX_PATH_PREFIX,

	.procs = {
		.find = texture_path,
		.check = check_texture_path,
		.begin_load = load_texture_begin,
		.end_load = load_texture_end,
		.unload = (ResourceUnloadProc)free_texture,
	},
};

char* texture_path(const char *name) {
	char *p = NULL;

	if((p = try_path(TEX_PATH_PREFIX, name, TEX_EXTENSION))) {
		return p;
	}

	return pixmap_source_path(TEX_PATH_PREFIX, name);
}

bool check_texture_path(const char *path) {
	if(strendswith(path, TEX_EXTENSION)) {
		return true;
	}

	return pixmap_check_filename(path);
}

typedef struct TexLoadData {
	SDL_Surface *surf;
	TextureParams params;
} TexLoadData;

static void parse_filter(const char *val, TextureFilterMode *out, bool allow_mipmaps) {
	if(!val) {
		return;
	}

	char buf[strlen(val) + 1];
	strcpy(buf, val);
	char *ignore, *pbuf = strtok_r(buf, " \t", &ignore);

	if(!SDL_strcasecmp(pbuf, "default")) {
		return;
	}

	if(!SDL_strcasecmp(pbuf, "nearest")) {
		*out = TEX_FILTER_NEAREST;
		return;
	}

	if(!SDL_strcasecmp(pbuf, "linear")) {
		*out = TEX_FILTER_LINEAR;
		return;
	}

	if(allow_mipmaps) {
		if(!SDL_strcasecmp(pbuf, "nearest_mipmap_nearest")) {
			*out = TEX_FILTER_NEAREST_MIPMAP_NEAREST;
			return;
		}

		if(!SDL_strcasecmp(pbuf, "nearest_mipmap_linear")) {
			*out = TEX_FILTER_NEAREST_MIPMAP_LINEAR;
			return;
		}

		if(!SDL_strcasecmp(pbuf, "linear_mipmap_nearest")) {
			*out = TEX_FILTER_LINEAR_MIPMAP_NEAREST;
			return;
		}

		if(!SDL_strcasecmp(pbuf, "linear_mipmap_linear")) {
			*out = TEX_FILTER_LINEAR_MIPMAP_LINEAR;
			return;
		}
	}

	log_fatal("Bad value `%s`, assuming default", pbuf);
}

static void parse_wrap(const char *val, TextureWrapMode *out) {
	if(!val) {
		return;
	}

	char buf[strlen(val) + 1];
	strcpy(buf, val);
	char *ignore, *pbuf = strtok_r(buf, " \t", &ignore);

	if(!SDL_strcasecmp(pbuf, "default")) {
		return;
	}

	if(!SDL_strcasecmp(pbuf, "clamp")) {
		*out = TEX_WRAP_CLAMP;
		return;
	}

	if(!SDL_strcasecmp(pbuf, "mirror")) {
		*out = TEX_WRAP_MIRROR;
		return;
	}

	if(!SDL_strcasecmp(pbuf, "repeat")) {
		*out = TEX_WRAP_REPEAT;
		return;
	}

	log_warn("Bad value `%s`, assuming default", pbuf);
}

static TextureType pixmap_format_to_texture_type(PixmapFormat fmt) {
	PixmapLayout layout = PIXMAP_FORMAT_LAYOUT(fmt);
	uint depth = PIXMAP_FORMAT_DEPTH(fmt);
	bool is_float = PIXMAP_FORMAT_IS_FLOAT(fmt);

	if(is_float) {
		switch(layout) {
			case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_32_FLOAT;
			case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_32_FLOAT;
			case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_32_FLOAT;
			case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_32_FLOAT;
		}

		UNREACHABLE;
	}

	if(depth > 8) {
		switch(layout) {
			case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_16;
			case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_16;
			case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_16;
			case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_16;
		}

		UNREACHABLE;
	}

	switch(layout) {
		case PIXMAP_LAYOUT_R:    return TEX_TYPE_R_8;
		case PIXMAP_LAYOUT_RG:   return TEX_TYPE_RG_8;
		case PIXMAP_LAYOUT_RGB:  return TEX_TYPE_RGB_8;
		case PIXMAP_LAYOUT_RGBA: return TEX_TYPE_RGBA_8;
	}

	UNREACHABLE;
}

static bool parse_format(const char *val, PixmapFormat *out) {
	if(!val) {
		return true;
	}

	uint channels = 0;

	char buf[strlen(val) + 1];
	strcpy(buf, val);

	if(*val) {
		char *c = strrchr(buf, 0) - 1;
		while(isspace(*c) && c >= buf) {
			*c-- = 0;
		}
	}

	val = buf;

	for(uint i = 4; i >= 1; --i) {
		if(!SDL_strncasecmp(val, "rgba", i)) {
			channels = i;
			break;
		}
	}

	if(channels < 1) {
		log_warn("Invalid format '%s': bad channels specification, expected one of: RGBA, RGB, RG, R", val);
		return false;
	}

	assert(channels <= 4);

	char *end;
	uint depth = strtol(val + channels, &end, 10);

	if(val + channels == end) {
		log_warn("Invalid format '%s': bad depth specification, expected an integer", val);
		return false;
	}

	if(depth != 8 && depth != 16 && depth != 32) {
		log_warn("Invalid format '%s': invalid bit depth %d, only 8, 16, and 32 are currently supported", val, depth);
		return false;
	}

	while(isspace(*end)) {
		++end;
	}

	bool is_float = false;

	if(*end) {
		if(!SDL_strcasecmp(end, "u") || !SDL_strcasecmp(end, "uint")) {
			// is_float = false;
		} else if(!SDL_strcasecmp(end, "f") || !SDL_strcasecmp(end, "float")) {
			is_float = true;
		} else {
			log_warn("Invalid format '%s': bad type specification, expected one of: U, UINT, F, FLOAT, or nothing", val);
			return false;
		}
	}

	if(depth == 32 && !is_float) {
		log_warn("Invalid format '%s': bit depth %d is currently only supported for floating point pixels", val, depth);
		return false;
	} else if(depth < 32 && is_float) {
		log_warn("Bit depth %d is currently not supported for floating point pixels, promoting to 32", depth);
		depth = 32;
	}

	*out = PIXMAP_MAKE_FORMAT(channels, depth) | (is_float * PIXMAP_FLOAT_BIT);
	return true;
}

typedef struct TextureLoadData {
	Pixmap pixmap;
	Pixmap pixmap_alphamap;
	TextureParams params;
} TextureLoadData;

static void* load_texture_begin(const char *path, uint flags) {
	const char *source = path;
	char *source_allocated = NULL;
	char *alphamap_allocated = NULL;

	TextureLoadData ld = {
		.params = {
			.filter = {
				.mag = TEX_FILTER_LINEAR,
				.min = TEX_FILTER_LINEAR_MIPMAP_LINEAR,
			},
			.wrap = {
				.s = TEX_WRAP_REPEAT,
				.t = TEX_WRAP_REPEAT,
			},
			.mipmaps = TEX_MIPMAPS_MAX,
			.anisotropy = TEX_ANISOTROPY_DEFAULT,

			// Manual because we don't want mipmaps generated until we've applied the
			// post-load shader, which will premultiply alpha.
			// Mipmaps and filtering are basically broken without premultiplied alpha.
			.mipmap_mode = TEX_MIPMAP_MANUAL,
		}
	};

	PixmapFormat override_format = 0;

	if(strendswith(path, TEX_EXTENSION)) {
		char *str_filter_min = NULL;
		char *str_filter_mag = NULL;
		char *str_wrap_s = NULL;
		char *str_wrap_t = NULL;
		char *str_format = NULL;

		if(!parse_keyvalue_file_with_spec(path, (KVSpec[]) {
			{ "source",     .out_str  = &source_allocated },
			{ "alphamap",   .out_str  = &alphamap_allocated },
			{ "filter_min", .out_str  = &str_filter_min },
			{ "filter_mag", .out_str  = &str_filter_mag },
			{ "wrap_s",     .out_str  = &str_wrap_s },
			{ "wrap_t",     .out_str  = &str_wrap_t },
			{ "format",     .out_str  = &str_format },
			{ "mipmaps",    .out_int  = (int*)&ld.params.mipmaps },
			{ "anisotropy", .out_int  = (int*)&ld.params.anisotropy },
			{ NULL }
		})) {
			free(source_allocated);
			free(alphamap_allocated);
			return NULL;
		}

		if(!source_allocated) {
			char *basename = resource_util_basename(TEX_PATH_PREFIX, path);
			source_allocated = pixmap_source_path(TEX_PATH_PREFIX, basename);

			if(!source_allocated) {
				log_error("%s: couldn't infer source path from texture name", basename);
			} else {
				log_info("%s: inferred source path from texture name: %s", basename, source_allocated);
			}

			free(basename);

			if(!source_allocated) {
				return NULL;
			}
		}

		source = source_allocated;

		parse_filter(str_filter_min, &ld.params.filter.min, true);
		free(str_filter_min);

		parse_filter(str_filter_mag, &ld.params.filter.mag, false);
		free(str_filter_mag);

		parse_wrap(str_wrap_s, &ld.params.wrap.s);
		free(str_wrap_s);

		parse_wrap(str_wrap_t, &ld.params.wrap.t);
		free(str_wrap_t);

		bool format_ok = parse_format(str_format, &override_format);
		free(str_format);

		if(!format_ok) {
			free(source_allocated);
			free(alphamap_allocated);
			log_error("%s: bad or unsupported pixel format specification", path);
			return NULL;
		}
	}

	if(!pixmap_load_file(source, &ld.pixmap)) {
		log_error("%s: couldn't load texture image", source);
		free(source_allocated);
		free(alphamap_allocated);
		return NULL;
	}

	if(alphamap_allocated && !pixmap_load_file(alphamap_allocated, &ld.pixmap_alphamap)) {
		log_error("%s: couldn't load texture alphamap", alphamap_allocated);
		free(source_allocated);
		free(alphamap_allocated);
		return NULL;
	}

	free(source_allocated);
	free(alphamap_allocated);

	PixmapOrigin org = r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN) ? PIXMAP_ORIGIN_BOTTOMLEFT : PIXMAP_ORIGIN_TOPLEFT;

	pixmap_flip_to_origin_inplace(&ld.pixmap, org);

	if(ld.pixmap_alphamap.data.untyped) {
		if(PIXMAP_FORMAT_DEPTH(ld.pixmap_alphamap.format) > 8) {
			pixmap_convert_inplace_realloc(&ld.pixmap_alphamap, PIXMAP_FORMAT_R16);
		} else {
			pixmap_convert_inplace_realloc(&ld.pixmap_alphamap, PIXMAP_FORMAT_R8);
		}

		pixmap_flip_to_origin_inplace(&ld.pixmap_alphamap, org);
	}

	override_format = override_format ? override_format : ld.pixmap.format;
	ld.params.type = pixmap_format_to_texture_type(override_format);
	log_debug("%s: %d channels, %d bits per channel, %s",
		path,
		PIXMAP_FORMAT_LAYOUT(override_format),
		PIXMAP_FORMAT_DEPTH(override_format),
		PIXMAP_FORMAT_IS_FLOAT(override_format) ? "float" : "uint"
	);

	if(ld.params.mipmaps == 0) {
		ld.params.mipmaps = TEX_MIPMAPS_MAX;
	}

	ld.params.width = ld.pixmap.width;
	ld.params.height = ld.pixmap.height;

	return memdup(&ld, sizeof(ld));
}

static Texture* texture_post_load(Texture *tex, Texture *alphamap) {
	// this is a bit hacky and not very efficient,
	// but it's still much faster than fixing up the texture on the CPU

	ShaderProgram *shader_saved = r_shader_current();
	Framebuffer *fb_saved = r_framebuffer_current();
	BlendMode blend_saved = r_blend_current();
	bool cullcap_saved = r_capability_current(RCAP_CULL_FACE);

	Texture *fbo_tex;
	TextureParams params;
	Framebuffer *fb;

	r_blend(BLEND_NONE);
	r_disable(RCAP_CULL_FACE);
	r_texture_get_params(tex, &params);
	params.mipmap_mode = TEX_MIPMAP_AUTO;
	r_texture_set_filter(tex, TEX_FILTER_NEAREST, TEX_FILTER_NEAREST);
	fbo_tex = r_texture_create(&params);

	r_shader("texture_post_load");
	r_uniform_sampler("tex", tex);

	if(alphamap) {
		r_uniform_sampler("alphamap", alphamap);
		r_uniform_int("have_alphamap", 1);
	} else {
		r_uniform_int("have_alphamap", 0);
	}

	r_mat_push();
	r_mat_identity();
	r_mat_mode(MM_PROJECTION);
	r_mat_push();
	r_mat_identity();
	r_mat_ortho(0, params.width, params.height, 0, -100, 100);
	r_mat_mode(MM_MODELVIEW);
	r_mat_scale(params.width, params.height, 1);
	r_mat_translate(0.5, 0.5, 0);
	fb = r_framebuffer_create();
	r_framebuffer_attach(fb, fbo_tex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_viewport(fb, 0, 0, params.width, params.height);
	r_framebuffer(fb);
	r_draw_quad();
	r_mat_pop();
	r_mat_mode(MM_PROJECTION);
	r_mat_pop();
	r_mat_mode(MM_MODELVIEW);
	r_framebuffer(fb_saved);
	r_shader_ptr(shader_saved);
	r_blend(blend_saved);
	r_capability(RCAP_CULL_FACE, cullcap_saved);
	r_framebuffer_destroy(fb);
	r_texture_destroy(tex);

	return fbo_tex;
}

static void* load_texture_end(void *opaque, const char *path, uint flags) {
	TextureLoadData *ld = opaque;

	if(!ld) {
		return NULL;
	}

	char *basename = resource_util_basename(TEX_PATH_PREFIX, path);
	Texture *texture = r_texture_create(&ld->params);
	r_texture_set_debug_label(texture, basename);
	r_texture_fill(texture, 0, &ld->pixmap);
	free(ld->pixmap.data.untyped);

	Texture *alphamap = NULL;

	if(ld->pixmap_alphamap.data.untyped) {
		TextureParams p = ld->params;
		p.type = PIXMAP_FORMAT_DEPTH(ld->pixmap_alphamap.format) > 8 ? TEX_TYPE_R_16 : TEX_TYPE_R_8;
		alphamap = r_texture_create(&p);
		const char suffix[] = " alphamap";
		char buf[strlen(basename) + sizeof(suffix)];
		snprintf(buf, sizeof(buf), "%s%s", basename, suffix);
		r_texture_set_debug_label(alphamap, buf);
		r_texture_fill(alphamap, 0, &ld->pixmap_alphamap);
		free(ld->pixmap_alphamap.data.untyped);
	}

	free(ld);

	texture = texture_post_load(texture, alphamap);
	r_texture_set_debug_label(texture, basename);
	free(basename);

	if(alphamap) {
		r_texture_destroy(alphamap);
	}

	return texture;
}

Texture* get_tex(const char *name) {
	return r_texture_get(name);
}

Texture* prefix_get_tex(const char *name, const char *prefix) {
	uint plen = strlen(prefix);
	char buf[plen + strlen(name) + 1];
	strcpy(buf, prefix);
	strcpy(buf + plen, name);
	Texture *tex = get_tex(buf);
	return tex;
}

static void free_texture(Texture *tex) {
	r_texture_destroy(tex);
}

static struct draw_texture_state {
	bool drawing;
	bool texture_matrix_tainted;
} draw_texture_state;

void begin_draw_texture(FloatRect dest, FloatRect frag, Texture *tex) {
	if(draw_texture_state.drawing) {
		log_fatal("Already drawing. Did you forget to call end_draw_texture, or call me on the wrong thread?");
	}

	draw_texture_state.drawing = true;

	r_uniform_sampler("tex", tex);
	r_mat_push();

	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);

	float x = dest.x;
	float y = dest.y;
	float w = dest.w;
	float h = dest.h;

	float s = frag.w/tw;
	float t = frag.h/th;

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		// FIXME: please somehow abstract this shit away!
		frag.y = th - frag.y - frag.h;
	}

	if(s != 1 || t != 1 || frag.x || frag.y) {
		draw_texture_state.texture_matrix_tainted = true;

		r_mat_mode(MM_TEXTURE);
		r_mat_identity();

		r_mat_scale(1.0/tw, 1.0/th, 1);

		if(frag.x || frag.y) {
			r_mat_translate(frag.x, frag.y, 0);
		}

		if(s != 1 || t != 1) {
			r_mat_scale(frag.w, frag.h, 1);
		}

		r_mat_mode(MM_MODELVIEW);
	}

	if(x || y) {
		r_mat_translate(x, y, 0);
	}

	if(w != 1 || h != 1) {
		r_mat_scale(w, h, 1);
	}
}

void end_draw_texture(void) {
	if(!draw_texture_state.drawing) {
		log_fatal("Not drawing. Did you forget to call begin_draw_texture, or call me on the wrong thread?");
	}

	if(draw_texture_state.texture_matrix_tainted) {
		draw_texture_state.texture_matrix_tainted = false;
		r_mat_mode(MM_TEXTURE);
		r_mat_identity();
		r_mat_mode(MM_MODELVIEW);
	}

	r_mat_pop();
	draw_texture_state.drawing = false;
}

void fill_viewport(float xoff, float yoff, float ratio, const char *name) {
	fill_viewport_p(xoff, yoff, ratio, 1, 0, get_tex(name));
}

void fill_viewport_p(float xoff, float yoff, float ratio, float aspect, float angle, Texture *tex) {
	r_uniform_sampler("tex", tex);

	float rw, rh;

	if(ratio == 0) {
		rw = aspect;
		rh = 1;
	} else {
		rw = ratio * aspect;
		rh = ratio;
	}

	if(r_supports(RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN)) {
		// FIXME: we should somehow account for this globally if possible...
		yoff *= -1;
		yoff += (1 - ratio);
	}

	bool texture_matrix_tainted = false;

	if(xoff || yoff || rw != 1 || rh != 1 || angle) {
		texture_matrix_tainted = true;
		r_mat_mode(MM_TEXTURE);

		if(xoff || yoff) {
			r_mat_translate(xoff, yoff, 0);
		}

		if(rw != 1 || rh != 1) {
			r_mat_scale(rw, rh, 1);
		}

		if(angle) {
			r_mat_translate(0.5, 0.5, 0);
			r_mat_rotate_deg(angle, 0, 0, 1);
			r_mat_translate(-0.5, -0.5, 0);
		}

		r_mat_mode(MM_MODELVIEW);
	}

	r_mat_push();
	r_mat_translate(VIEWPORT_W*0.5, VIEWPORT_H*0.5, 0);
	r_mat_scale(VIEWPORT_W, VIEWPORT_H, 1);
	r_draw_quad();
	r_mat_pop();

	if(texture_matrix_tainted) {
		r_mat_mode(MM_TEXTURE);
		r_mat_identity();
		r_mat_mode(MM_MODELVIEW);
	}
}

void fill_screen(const char *name) {
	fill_screen_p(get_tex(name));
}

void fill_screen_p(Texture *tex) {
	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);
	begin_draw_texture((FloatRect){ SCREEN_W*0.5, SCREEN_H*0.5, SCREEN_W, SCREEN_H }, (FloatRect){ 0, 0, tw, th }, tex);
	r_draw_quad();
	end_draw_texture();
}

// draws a thin, w-width rectangle from point A to point B with a texture that
// moves along the line.
//
void loop_tex_line_p(complex a, complex b, float w, float t, Texture *texture) {
	complex d = b-a;
	complex c = (b+a)/2;

	r_mat_push();
	r_mat_translate(creal(c),cimag(c),0);
	r_mat_rotate_deg(180/M_PI*carg(d),0,0,1);
	r_mat_scale(cabs(d),w,1);

	r_mat_mode(MM_TEXTURE);
	// r_mat_identity();
	r_mat_translate(t, 0, 0);
	r_mat_mode(MM_MODELVIEW);

	r_uniform_sampler("tex", texture);
	r_draw_quad();

	r_mat_mode(MM_TEXTURE);
	r_mat_identity();
	r_mat_mode(MM_MODELVIEW);

	r_mat_pop();
}

void loop_tex_line(complex a, complex b, float w, float t, const char *texture) {
	loop_tex_line_p(a, b, w, t, get_tex(texture));
}
