/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "sprite.h"
#include "resource.h"
#include "video.h"

char* sprite_path(const char *name) {
	char *path = strjoin(SPRITE_PATH_PREFIX, name, SPRITE_EXTENSION, NULL);

	VFSInfo pinfo = vfs_query(path);

	if(!pinfo.exists) {
		free(path);
		return texture_path(name);
	}

	return path;
}

bool check_sprite_path(const char *path) {
	return strendswith(path, SPRITE_EXTENSION) || check_texture_path(path);
}

struct sprite_load_state {
	Sprite *spr;
	unsigned int flags;
	char *texture_name;
};

void* load_sprite_begin(const char *path, unsigned int flags) {
	Sprite *spr = calloc(1, sizeof(Sprite));
	struct sprite_load_state *state = calloc(1, sizeof(struct sprite_load_state));
	state->spr = spr;
	state->flags = flags;

	if(check_texture_path(path)) {
		state->texture_name = resource_util_basename(TEX_PATH_PREFIX, path);
		return state;
	}

	if(!parse_keyvalue_file_with_spec(path, (KVSpec[]){
		{ "texture",  .out_str   = &state->texture_name },
		{ "region_x", .out_float = &spr->tex_area.x },
		{ "region_y", .out_float = &spr->tex_area.y },
		{ "region_w", .out_float = &spr->tex_area.w },
		{ "region_h", .out_float = &spr->tex_area.h },
		{ "w",        .out_float = &spr->w },
		{ "h",        .out_float = &spr->h },
		{ NULL }
	})) {
		free(spr);
		free(state->texture_name);
		free(state);
		log_warn("Failed to parse sprite file '%s'", path);
		return NULL;
	}

	if(!state->texture_name) {
		state->texture_name = resource_util_basename(TEX_PATH_PREFIX, path);
		log_warn("%s: inferred texture name from sprite name", state->texture_name);
	}

	return state;
}

void* load_sprite_end(void *opaque, const char *path, unsigned int flags) {
	struct sprite_load_state *state = opaque;

	if(!state) {
		return NULL;
	}

	Sprite *spr = state->spr;

	Resource *res = get_resource(RES_TEXTURE, state->texture_name, flags);

	free(state->texture_name);
	free(state);

	if(res == NULL) {
		free(spr);
		return NULL;
	}

	spr->tex = res->texture;

	float tex_w_flt = spr->tex->w;
	float tex_h_flt = spr->tex->h;

	struct infermap {
		const char *dstname;
		float *dst;
		const char *srcname;
		float *src;
	} infermap[] = {
		{ "region width",  &spr->tex_area.w, "texture width",  &tex_w_flt },
		{ "region height", &spr->tex_area.h, "texture height", &tex_h_flt },
		{ "sprite width",  &spr->w,          "region width",   &spr->tex_area.w },
		{ "sprite height", &spr->h,          "region height",  &spr->tex_area.h },
		{ NULL },
	};

	char *basename = resource_util_basename(SPRITE_PATH_PREFIX, path);

	for(struct infermap *m = infermap; m->dst; ++m) {
		if(*m->dst <= 0) {
			*m->dst = *m->src;
			log_warn("%s: inferred %s from %s (%f)", basename, m->dstname, m->srcname, *m->src);
		}
	}

	free(basename);
	return spr;
}

Sprite* get_sprite(const char *name) {
	return get_resource(RES_SPRITE, name, RESF_DEFAULT | RESF_UNSAFE)->sprite;
}

Sprite* prefix_get_sprite(const char *name, const char *prefix) {
	char *full = strjoin(prefix, name, NULL);
	Sprite *spr = get_sprite(full);
	free(full);
	return spr;
}

void draw_sprite(float x, float y, const char *name) {
	draw_sprite_p(x, y, get_sprite(name));
}

void draw_sprite_p(float x, float y, Sprite *spr) {
	draw_sprite_ex(x, y, 1, 1, true, spr);
}

void draw_sprite_unaligned(float x, float y, const char *name) {
	draw_sprite_unaligned_p(x, y, get_sprite(name));
}

void draw_sprite_unaligned_p(float x, float y, Sprite *spr) {
	draw_sprite_ex(x, y, 1, 1, false, spr);
}

void begin_draw_sprite(float x, float y, float scale_x, float scale_y, bool align, Sprite *spr) {
	/*
	if(align) {
		// pixel-"perfect" alignment hack
		// doesn't take viewport quality setting into account,
		// but i assume that if you have it below 100%, you don't care anyway.
		float q = video.quality_factor;
		x = floor(x*q+0.5)/q;
		y = floor(y*q+0.5)/q;
	}
	*/

	begin_draw_texture(
		(FloatRect){ x, y, spr->w * scale_x, spr->h * scale_y },
		(FloatRect){ spr->tex_area.x, spr->tex_area.y, spr->tex_area.w, spr->tex_area.h },
		spr->tex
	);
}

void end_draw_sprite(void) {
	end_draw_texture();
}

void draw_sprite_ex(float x, float y, float scale_x, float scale_y, bool align, Sprite *spr) {
	begin_draw_sprite(x, y, scale_x, scale_y, align, spr);
	draw_quad();
	end_draw_texture();
}
