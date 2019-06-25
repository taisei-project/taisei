/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "sprite.h"
#include "video.h"
#include "renderer/api.h"

static char *sprite_path(const char *name) {
	char *path = strjoin(RES_PATHPREFIX_SPRITE, name, SPRITE_EXTENSION, NULL);

	VFSInfo pinfo = vfs_query(path);

	if(!pinfo.exists) {
		free(path);
		return texture_res_handler.procs.find(name);
	}

	return path;
}

static bool check_sprite_path(const char *path) {
	return strendswith(path, SPRITE_EXTENSION) || texture_res_handler.procs.check(path);
}

struct sprite_load_state {
	Sprite *spr;
	uint flags;
};

static bool sprite_load_set_texture_ref(const char *key, const char *value, void *data) {
	struct sprite_load_state *state = data;
	state->spr->tex = res_ref(RES_TEXTURE, value, state->flags);
	return true;
}

static void *load_sprite_begin(ResourceLoadInfo loadinfo) {
	Sprite *spr = calloc(1, sizeof(Sprite));
	struct sprite_load_state *state = calloc(1, sizeof(struct sprite_load_state));
	state->spr = spr;
	state->flags = loadinfo.flags;

	if(texture_res_handler.procs.check(loadinfo.path)) {
		spr->tex = res_ref(RES_TEXTURE, loadinfo.name, loadinfo.flags);
		return state;
	}

	if(!parse_keyvalue_file_with_spec(loadinfo.path, (KVSpec[]) {
		{ "texture",  .callback = sprite_load_set_texture_ref, .callback_data = state },
		{ "region_x", .out_float = &spr->tex_area.x },
		{ "region_y", .out_float = &spr->tex_area.y },
		{ "region_w", .out_float = &spr->tex_area.w },
		{ "region_h", .out_float = &spr->tex_area.h },
		{ "w",        .out_float = &spr->w },
		{ "h",        .out_float = &spr->h },
		{ "offset_x", .out_float = &spr->offset.x },
		{ "offset_y", .out_float = &spr->offset.y },
		{ NULL }
	})) {
		res_unref_if_valid(&spr->tex, 1);
		free(spr);
		free(state);
		log_error("Failed to parse sprite file '%s'", loadinfo.path);
		return NULL;
	}

	if(!res_ref_is_valid(spr->tex)) {
		log_info("%s: inferred texture name from sprite name", loadinfo.name);
		spr->tex = res_ref(RES_TEXTURE, loadinfo.name, loadinfo.flags);
	}

	return state;
}

attr_nonnull(2)
static void *load_sprite_end(ResourceLoadInfo loadinfo, void *opaque) {
	struct sprite_load_state *state = opaque;

	Sprite *spr = state->spr;
	Texture *tex = res_ref_data(state->spr->tex);

	free(state);

	if(tex == NULL) {
		res_unref(&spr->tex, 1);
		free(spr);
		return NULL;
	}

	uint tw, th;
	r_texture_get_size(tex, 0, &tw, &th);

	float tex_w_flt = tw;
	float tex_h_flt = th;

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

	for(struct infermap *m = infermap; m->dst; ++m) {
		if(*m->dst <= 0) {
			*m->dst = *m->src;
			log_info("%s: inferred %s from %s (%g)", loadinfo.name, m->dstname, m->srcname, *m->src);
		}
	}

	return spr;
}

static void unload_sprite(void *sprite) {
	Sprite *spr = sprite;
	res_unref(&spr->tex, 1);
	free(spr);
}

Sprite *get_sprite(const char *name) {
	return res_get_data(RES_SPRITE, name, RESF_DEFAULT);
}

Sprite *prefix_get_sprite(const char *name, const char *prefix) {
	uint plen = strlen(prefix);
	char buf[plen + strlen(name) + 1];
	strcpy(buf, prefix);
	strcpy(buf + plen, name);
	Sprite *spr = get_sprite(buf);
	return spr;
}

void draw_sprite(float x, float y, const char *name) {
	draw_sprite_p(x, y, get_sprite(name));
}

void draw_sprite_p(float x, float y, Sprite *spr) {
	draw_sprite_ex(x, y, 1, 1, false, spr);
}

void draw_sprite_batched(float x, float y, const char *name) {
	draw_sprite_ex(x, y, 1, 1, true, get_sprite(name));
}

void draw_sprite_batched_p(float x, float y, Sprite *spr) {
	draw_sprite_ex(x, y, 1, 1, true, spr);
}

void begin_draw_sprite(float x, float y, float scale_x, float scale_y, Sprite *spr) {
	begin_draw_texture(
		(FloatRect){ x + spr->offset.x * scale_x, y + spr->offset.y * scale_y, spr->w * scale_x, spr->h * scale_y },
		(FloatRect){ spr->tex_area.x, spr->tex_area.y, spr->tex_area.w, spr->tex_area.h },
		res_ref_data(spr->tex)
	);
}

void end_draw_sprite(void) {
	end_draw_texture();
}

void draw_sprite_ex(float x, float y, float scale_x, float scale_y, bool batched, Sprite *spr) {
	if(batched) {
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = spr,
			.pos.x = x,
			.pos.y = y,
			.scale.x = scale_x,
			.scale.y = scale_y,
			.shader_ptr = r_shader_current(),
			.color = r_color_current(),
		});
	} else {
		begin_draw_sprite(x, y, scale_x, scale_y, spr);
		r_draw_quad();
		end_draw_texture();
	}
}

ResourceHandler sprite_res_handler = {
	.type = RES_SPRITE,

	.procs = {
		.find = sprite_path,
		.check = check_sprite_path,
		.begin_load = load_sprite_begin,
		.end_load = load_sprite_end,
		.unload = unload_sprite,
	},
};
