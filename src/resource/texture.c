/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <SDL_image.h>

#include "texture.h"
#include "resource.h"
#include "global.h"
#include "video.h"
#include "renderer/api.h"

static void* load_texture_begin(const char *path, uint flags);
static void* load_texture_end(void *opaque, const char *path, uint flags);
static void free_texture(Texture *tex);

static void init_sdl_image(void) {
	int want_flags = IMG_INIT_JPG | IMG_INIT_PNG;
	int init_flags = IMG_Init(want_flags);

	if((want_flags & init_flags) != want_flags) {
		log_warn(
			"SDL_image doesn't support some of the formats we want. "
			"Requested: %i, got: %i. "
			"Textures may fail to load",
			want_flags,
			init_flags
		);
	}
}

ResourceHandler texture_res_handler = {
	.type = RES_TEXTURE,
	.typename = "texture",
	.subdir = TEX_PATH_PREFIX,

	.procs = {
		.init = init_sdl_image,
		.shutdown = IMG_Quit,
		.find = texture_path,
		.check = check_texture_path,
		.begin_load = load_texture_begin,
		.end_load = load_texture_end,
		.unload = (ResourceUnloadProc)free_texture,
	},
};

static const char *texture_image_exts[] = {
	// more are usable if you explicitly specify the source in a .tex file,
	// but these are the ones we officially support, and are tried in this
	// order for source auto-detection.
	".png",
	".jpg",
	".jpeg",
	NULL
};

static char* texture_image_path(const char *name) {
	char *p = NULL;

	for(const char **ext = texture_image_exts; *ext; ++ext) {
		if((p = try_path(TEX_PATH_PREFIX, name, *ext))) {
			return p;
		}
	}

	return NULL;
}

char* texture_path(const char *name) {
	char *p = NULL;

	if((p = try_path(TEX_PATH_PREFIX, name, TEX_EXTENSION))) {
		return p;
	}

	return texture_image_path(name);
}

bool check_texture_path(const char *path) {
	if(strendswith(path, TEX_EXTENSION)) {
		return true;
	}

	return strendswith_any(path, texture_image_exts);
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

typedef struct TextureLoadData {
	SDL_Surface *surf;
	TextureParams params;
} TextureLoadData;

static void* load_texture_begin(const char *path, uint flags) {
	const char *source = path;
	char *source_allocated = NULL;
	SDL_Surface *surf = NULL;
	SDL_RWops *srcrw = NULL;

	TextureLoadData ld = {
		.params = {
			.type = TEX_TYPE_RGBA,
			.filter = {
				.mag = TEX_FILTER_LINEAR,
				.min = TEX_FILTER_LINEAR,
			},
			.wrap = {
				.s = TEX_WRAP_REPEAT,
				.t = TEX_WRAP_REPEAT,
			},
			.mipmap_mode = TEX_MIPMAP_MANUAL,
			.mipmaps = 1,
		}
	};

	if(strendswith(path, TEX_EXTENSION)) {
		char *str_filter_min = NULL;
		char *str_filter_mag = NULL;
		char *str_wrap_s = NULL;
		char *str_wrap_t = NULL;

		if(!parse_keyvalue_file_with_spec(path, (KVSpec[]) {
			{ "source",     .out_str  = &source_allocated },
			{ "filter_min", .out_str  = &str_filter_min },
			{ "filter_mag", .out_str  = &str_filter_mag },
			{ "wrap_s",     .out_str  = &str_wrap_s },
			{ "wrap_t",     .out_str  = &str_wrap_t },
			{ "mipmaps",    .out_int  = (int*)&ld.params.mipmaps },
			{ NULL }
		})) {
			free(source_allocated);
			return NULL;
		}

		if(!source_allocated) {
			char *basename = resource_util_basename(TEX_PATH_PREFIX, path);
			source_allocated = texture_image_path(basename);

			if(!source_allocated) {
				log_warn("%s: couldn't infer source path from texture name", basename);
			} else {
				log_warn("%s: inferred source path from texture name: %s", basename, source_allocated);
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
	}

	srcrw = vfs_open(source, VFS_MODE_READ | VFS_MODE_SEEKABLE);

	if(!srcrw) {
		log_warn("VFS error: %s", vfs_get_error());
		free(source_allocated);
		return NULL;
	}

	if(strendswith(source, ".tga")) {
		surf = IMG_LoadTGA_RW(srcrw);
	} else {
		surf = IMG_Load_RW(srcrw, false);
	}

	free(source_allocated);

	if(!surf) {
		log_warn("IMG_Load_RW failed: %s", IMG_GetError());
		return NULL;
	}

	SDL_RWclose(srcrw);

	SDL_Surface *converted_surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_FreeSurface(surf);

	if(!converted_surf) {
		log_warn("SDL_ConvertSurfaceFormat(): failed: %s", SDL_GetError());
		return NULL;
	}

	if(ld.params.mipmaps == 0) {
		ld.params.mipmaps = TEX_MIPMAPS_MAX;
	}

	ld.params.width = converted_surf->w;
	ld.params.height = converted_surf->h;
	ld.surf = converted_surf;

	return memdup(&ld, sizeof(ld));
}

static void texture_post_load(Texture *tex) {
	// this is a bit hacky and not very efficient,
	// but it's still much faster than fixing up the texture on the CPU

	ShaderProgram *shader_saved = r_shader_current();
	Texture *texture_saved = r_texture_current(0);
	Framebuffer *fb_saved = r_framebuffer_current();
	BlendMode blend_saved = r_blend_current();
	bool cullcap_saved = r_capability_current(RCAP_CULL_FACE);

	Texture fbo_tex;
	TextureParams params;
	Framebuffer fb;

	r_blend(BLEND_NONE);
	r_disable(RCAP_CULL_FACE);
	r_texture_get_params(tex, &params);
	params.mipmap_mode = TEX_MIPMAP_MANUAL;
	r_texture_set_filter(tex, TEX_FILTER_NEAREST, TEX_FILTER_NEAREST);
	r_texture_create(&fbo_tex, &params);
	r_texture_ptr(0, tex);
	r_shader("texture_post_load");
	r_mat_push();
	r_mat_identity();
	r_mat_mode(MM_PROJECTION);
	r_mat_push();
	r_mat_identity();
	r_mat_ortho(0, tex->w, tex->h, 0, -100, 100);
	r_mat_mode(MM_MODELVIEW);
	r_mat_scale(tex->w, tex->h, 1);
	r_mat_translate(0.5, 0.5, 0);
	r_mat_scale(1, -1, 1);
	r_framebuffer_create(&fb);

	for(uint mipmap = 0; mipmap < params.mipmaps; ++mipmap) {
		r_framebuffer_attach(&fb, &fbo_tex, mipmap, FRAMEBUFFER_ATTACH_COLOR0);
		r_framebuffer_viewport(&fb, 0, 0, max(1, floor(tex->w / pow(2, mipmap))), max(1, floor(tex->h / pow(2, mipmap))));
		r_framebuffer(&fb);
		r_draw_quad();
	}

	r_mat_pop();
	r_mat_mode(MM_PROJECTION);
	r_mat_pop();
	r_mat_mode(MM_MODELVIEW);
	r_framebuffer(fb_saved);
	r_shader_ptr(shader_saved);
	r_texture_ptr(0, texture_saved);
	r_blend(blend_saved);
	r_capability(RCAP_CULL_FACE, cullcap_saved);
	r_framebuffer_destroy(&fb);
	r_texture_destroy(tex);

	memcpy(tex, &fbo_tex, sizeof(fbo_tex));
}

static void* load_texture_end(void *opaque, const char *path, uint flags) {
	TextureLoadData *ld = opaque;

	if(!ld) {
		return NULL;
	}

	Texture *texture = malloc(sizeof(Texture));

	r_texture_create(texture, &ld->params);
	SDL_LockSurface(ld->surf);
	r_texture_fill(texture, 0, ld->surf->pixels);
	SDL_UnlockSurface(ld->surf);
	SDL_FreeSurface(ld->surf);
	free(ld);

	texture_post_load(texture);
	return texture;
}

Texture* get_tex(const char *name) {
	return r_texture_get(name);
}

Texture* prefix_get_tex(const char *name, const char *prefix) {
	char *full = strjoin(prefix, name, NULL);
	Texture *tex = get_tex(full);
	free(full);
	return tex;
}

static void free_texture(Texture *tex) {
	r_texture_destroy(tex);
	free(tex);
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

	r_texture_ptr(0, tex);
	r_mat_push();

	float x = dest.x;
	float y = dest.y;
	float w = dest.w;
	float h = dest.h;

	float s = frag.w/tex->w;
	float t = frag.h/tex->h;

	if(s != 1 || t != 1 || frag.x || frag.y) {
		draw_texture_state.texture_matrix_tainted = true;

		r_mat_mode(MM_TEXTURE);
		r_mat_identity();

		r_mat_scale(1.0/tex->w, 1.0/tex->h, 1);

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

void draw_texture_p(float x, float y, Texture *tex) {
	begin_draw_texture((FloatRect){ x, y, tex->w, tex->h }, (FloatRect){ 0, 0, tex->w, tex->h }, tex);
	r_draw_quad();
	end_draw_texture();
}

void draw_texture(float x, float y, const char *name) {
	draw_texture_p(x, y, get_tex(name));
}

void draw_texture_with_size_p(float x, float y, float w, float h, Texture *tex) {
	begin_draw_texture((FloatRect){ x, y, w, h }, (FloatRect){ 0, 0, tex->w, tex->h }, tex);
	r_draw_quad();
	end_draw_texture();
}

void draw_texture_with_size(float x, float y, float w, float h, const char *name) {
	draw_texture_with_size_p(x, y, w, h, get_tex(name));
}

void fill_viewport(float xoff, float yoff, float ratio, const char *name) {
	fill_viewport_p(xoff, yoff, ratio, 1, 0, get_tex(name));
}

void fill_viewport_p(float xoff, float yoff, float ratio, float aspect, float angle, Texture *tex) {
	r_texture_ptr(0, tex);

	float rw, rh;

	if(ratio == 0) {
		rw = aspect;
		rh = 1;
	} else {
		rw = ratio * aspect;
		rh = ratio;
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
	begin_draw_texture((FloatRect){ SCREEN_W*0.5, SCREEN_H*0.5, SCREEN_W, SCREEN_H }, (FloatRect){ 0, 0, tex->w, tex->h }, tex);
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

	r_texture_ptr(0, texture);
	r_draw_quad();

	r_mat_mode(MM_TEXTURE);
	r_mat_identity();
	r_mat_mode(MM_MODELVIEW);

	r_mat_pop();
}

void loop_tex_line(complex a, complex b, float w, float t, const char *texture) {
	loop_tex_line_p(a, b, w, t, get_tex(texture));
}
