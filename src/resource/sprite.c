/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "sprite.h"
#include "video.h"
#include "renderer/api.h"

ResourceHandler sprite_res_handler = {
	.type = RES_SPRITE,
	.typename = "sprite",
	.subdir = SPRITE_PATH_PREFIX,

	.procs = {
		.find = sprite_path,
		.check = check_sprite_path,
		.begin_load = load_sprite_begin,
		.end_load = load_sprite_end,
		.unload = free,
	},
};

char *sprite_path(const char *name) {
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
	uint flags;
	char *texture_name;
};

void *load_sprite_begin(const char *path, uint flags) {
	Sprite *spr = calloc(1, sizeof(Sprite));
	struct sprite_load_state *state = calloc(1, sizeof(struct sprite_load_state));
	state->spr = spr;
	state->flags = flags;

	if(check_texture_path(path)) {
		state->texture_name = resource_util_basename(TEX_PATH_PREFIX, path);
		return state;
	}

	float ofs_x = 0, ofs_y = 0;

	spr->tex_area = (FloatRect) { .offset = { 0, 0 }, .extent = { 1, 1 } };

	if(!parse_keyvalue_file_with_spec(path, (KVSpec[]) {
		{ "texture",        .out_str   = &state->texture_name },
		{ "region_x",       .out_float = &spr->tex_area.x },
		{ "region_y",       .out_float = &spr->tex_area.y },
		{ "region_w",       .out_float = &spr->tex_area.w },
		{ "region_h",       .out_float = &spr->tex_area.h },
		{ "w",              .out_float = &spr->w },
		{ "h",              .out_float = &spr->h },
		{ "offset_x",       .out_float = &ofs_x, KVSPEC_DEPRECATED("margin_left; margin_right") },
		{ "offset_y",       .out_float = &ofs_y, KVSPEC_DEPRECATED("margin_top; margin_bottom") },
		{ "padding_top",    .out_float = &spr->padding.top },
		{ "padding_bottom", .out_float = &spr->padding.bottom },
		{ "padding_left",   .out_float = &spr->padding.left },
		{ "padding_right",  .out_float = &spr->padding.right },
		{ NULL }
	})) {
		free(spr);
		free(state->texture_name);
		free(state);
		log_error("Failed to parse sprite file '%s'", path);
		return NULL;
	}

	spr->padding.left += ofs_x;
	spr->padding.right -= ofs_x;
	spr->padding.top += ofs_y;
	spr->padding.bottom -= ofs_y;

	if(!state->texture_name) {
		state->texture_name = resource_util_basename(TEX_PATH_PREFIX, path);
		log_info("%s: inferred texture name from sprite name", state->texture_name);
	}

	return state;
}

void *load_sprite_end(void *opaque, const char *path, uint flags) {
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

	spr->tex = res->data;

	char *basename = resource_util_basename(SPRITE_PATH_PREFIX, path);
	FloatExtent denorm_coords = sprite_denormalized_tex_coords(spr).extent;

	struct infermap {
		const char *dstname;
		float *dst;
		const char *srcname;
		float *src;
	} infermap[] = {
		{ "sprite width",  &spr->w, "texture region width",  &denorm_coords.w },
		{ "sprite height", &spr->h, "texture region height", &denorm_coords.h },
		{ NULL },
	};

	for(struct infermap *m = infermap; m->dst; ++m) {
		if(*m->dst <= 0) {
			*m->dst = *m->src;
			log_info("%s: inferred %s from %s (%g)", basename, m->dstname, m->srcname, *m->src);
		}
	}

	free(basename);
	return spr;
}

Sprite *prefix_get_sprite(const char *name, const char *prefix) {
	uint plen = strlen(prefix);
	char buf[plen + strlen(name) + 1];
	strcpy(buf, prefix);
	strcpy(buf + plen, name);
	Sprite *spr = get_sprite(buf);
	return spr;
}

static void begin_draw_sprite(float x, float y, float scale_x, float scale_y, Sprite *spr) {
	FloatOffset o = sprite_padded_offset(spr);

	begin_draw_texture(
		(FloatRect){ x + o.x * scale_x, y + o.y * scale_y, spr->w * scale_x, spr->h * scale_y },
		(FloatRect){ spr->tex_area.x, spr->tex_area.y, spr->tex_area.w, spr->tex_area.h },
		spr->tex
	);
}

static void end_draw_sprite(void) {
	end_draw_texture();
}

static void draw_sprite_ex(float x, float y, float scale_x, float scale_y, Sprite *spr) {
	begin_draw_sprite(x, y, scale_x, scale_y, spr);
	r_draw_quad();
	end_draw_sprite();
}

void draw_sprite(float x, float y, const char *name) {
	draw_sprite_p(x, y, get_sprite(name));
}

void draw_sprite_p(float x, float y, Sprite *spr) {
	draw_sprite_ex(x, y, 1, 1, spr);
}

FloatRect sprite_denormalized_tex_coords(const Sprite *restrict spr) {
	Texture *tex = NOT_NULL(spr->tex);
	uint tex_w, tex_h;
	r_texture_get_size(tex, 0, &tex_w, &tex_h);
	FloatRect tc = spr->tex_area;
	tc.x *= tex_w;
	tc.y *= tex_h;
	tc.w *= tex_w;
	tc.h *= tex_h;
	return tc;
}

IntRect sprite_denormalized_int_tex_coords(const Sprite *restrict spr) {
	FloatRect tc = sprite_denormalized_tex_coords(spr);
	IntRect itc;
	itc.x = lroundf(tc.x);
	itc.y = lroundf(tc.y);
	itc.w = lroundf(tc.w);
	itc.h = lroundf(tc.h);
	return itc;
}

void sprite_set_denormalized_tex_coords(Sprite *restrict spr, FloatRect tc) {
	Texture *tex = NOT_NULL(spr->tex);
	uint tex_w, tex_h;
	r_texture_get_size(tex, 0, &tex_w, &tex_h);
	spr->tex_area.x = tc.x / tex_w;
	spr->tex_area.y = tc.y / tex_h;
	spr->tex_area.w = tc.w / tex_w;
	spr->tex_area.h = tc.h / tex_h;
}
