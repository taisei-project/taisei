/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "player.h"
#include "global.h"
#include "stage.h"

#include "plrmodes.h"
#include "plrmodes/marisa.h"
#include "plrmodes/youmu.h"
#include "plrmodes/reimu.h"

static PlayerCharacter *player_characters[] = {
	&character_reimu,
	&character_marisa,
	&character_youmu,
};

static PlayerMode *player_modes[] = {
	&plrmode_reimu_a,
	&plrmode_reimu_b,
	&plrmode_marisa_a,
	&plrmode_marisa_b,
	&plrmode_youmu_a,
	&plrmode_youmu_b,
};

PlayerCharacter* plrchar_get(CharacterID id) {
	assert((unsigned)id < NUM_CHARACTERS);
	PlayerCharacter *pc = player_characters[id];
	assert(pc->id == id);
	return pc;
}

void plrchar_preload(PlayerCharacter *pc, ResourceRefGroup *rg) {
	res_group_multi_add(rg, RES_ANIMATION, RESF_DEFAULT, pc->player_sprite_name, NULL);
	res_group_multi_add(rg, RES_SPRITE, RESF_DEFAULT, pc->dialog_base_sprite_name, NULL);

	for(int i = 0; i < DIALOG_NUM_FACES; ++i) {
		res_group_multi_add(rg, RES_SPRITE, RESF_DEFAULT, pc->dialog_face_sprite_names[i], NULL);
	}
}

void plrchar_make_bomb_portrait(PlayerCharacter *pc, Sprite *out_spr) {
	r_state_push();

	Sprite *s_base = get_sprite(pc->dialog_base_sprite_name);
	Sprite *s_face = get_sprite(pc->dialog_face_sprite_names[DIALOG_FACE_NORMAL]);

	uint tex_w = s_base->tex_area.extent.w;
	uint tex_h = s_base->tex_area.extent.h;
	uint spr_w = s_base->extent.w;
	uint spr_h = s_base->extent.h;

	Texture *ptex = r_texture_create(&(TextureParams) {
		.type = TEX_TYPE_RGBA_8,
		.width = tex_w,
		.height = tex_h,
		.filter.min = TEX_FILTER_LINEAR_MIPMAP_LINEAR,
		.filter.mag = TEX_FILTER_LINEAR,
		.mipmap_mode = TEX_MIPMAP_AUTO,
		.mipmaps = 3,
	});

	char label[128];
	snprintf(label, sizeof(label), "%s bomb portrait", pc->lower_name);
	r_texture_set_debug_label(ptex, label);

	Framebuffer *fb = r_framebuffer_create();
	snprintf(label, sizeof(label), "%s bomb portrait FB", pc->lower_name);
	r_framebuffer_set_debug_label(fb, label);
	r_framebuffer_attach(fb, ptex, 0, FRAMEBUFFER_ATTACH_COLOR0);
	r_framebuffer_viewport(fb, 0, 0, tex_w, tex_h);
	r_framebuffer(fb);

	r_mat_mode(MM_PROJECTION);
	r_mat_push();
	r_mat_ortho(0, spr_w, spr_h, 0, -1, 1);
	r_mat_mode(MM_MODELVIEW);
	r_mat_push();
	r_mat_identity();

	SpriteParams sp = { 0 };
	sp.sprite_ptr = s_base;
	sp.blend = BLEND_NONE;
	sp.pos.x = spr_w / 2.0 - s_base->offset.x;
	sp.pos.y = spr_h / 2.0 - s_base->offset.y;
	sp.color = RGBA(1, 1, 1, 1);
	sp.shader_ptr = r_shader_get("sprite_default"),
	r_draw_sprite(&sp);
	sp.blend = BLEND_PREMUL_ALPHA;
	sp.sprite_ptr = s_face;
	r_draw_sprite(&sp);
	r_flush_sprites();

	r_mat_pop();
	r_mat_mode(MM_PROJECTION);
	r_mat_pop();
	r_state_pop();
	r_framebuffer_destroy(fb);

	Sprite s = { 0 };
	s.tex = res_ref_wrap_external_data(RES_TEXTURE, ptex);
	s.offset = s_base->offset;
	s.extent = s_base->extent;
	s.tex_area.w = tex_w;
	s.tex_area.h = tex_h;
	*out_spr = s;
}

void plrchar_make_dialog_actor(PlayerCharacter *pc, DialogActor *out_actor) {
	out_actor->base = get_sprite(pc->dialog_base_sprite_name);

	for(int i = 0; i < DIALOG_NUM_FACES; ++i) {
		out_actor->faces[i] = get_sprite(pc->dialog_face_sprite_names[i]);
	}

	out_actor->face = out_actor->faces[DIALOG_FACE_NORMAL];
}

int plrmode_repr(char *out, size_t outsize, PlayerMode *mode, bool internal) {
	assert(mode->character != NULL);
	assert((unsigned)mode->shot_mode < NUM_SHOT_MODES_PER_CHARACTER);

	return snprintf(out, outsize, "%s%c",
		internal ? mode->character->lower_name : mode->character->proper_name,
		mode->shot_mode + 'A'
	);
}

PlayerMode* plrmode_find(CharacterID char_id, ShotModeID shot_id) {
	for(int i = 0; i < NUM_PLAYER_MODES; ++i) {
		PlayerMode *mode = player_modes[i];

		if(mode->character->id == char_id && mode->shot_mode == shot_id) {
			return mode;
		}
	}

	return NULL;
}

PlayerMode* plrmode_parse(const char *name) {
	CharacterID char_id = (CharacterID)-1;
	ShotModeID shot_id = (ShotModeID)-1;
	char buf[strlen(name) + 1];
	strcpy(buf, name);

	for(int i = 0; i < sizeof(buf); ++i) {
		buf[i] = tolower(buf[i]);
	}

	if(!*buf) {
		log_debug("Got an empty string");
		return NULL;
	}

	char shot = buf[sizeof(buf) - 2];

	if(shot < 'a' || shot >= 'a' + NUM_SHOT_MODES_PER_CHARACTER) {
		log_debug("Got invalid shotmode: %c", shot);
		return NULL;
	}

	shot_id = shot - 'a';
	buf[sizeof(buf) - 2] = 0;

	for(int i = 0; i < NUM_CHARACTERS; ++i) {
		PlayerCharacter *chr = player_characters[i];

		if(!strcmp(buf, chr->lower_name)) {
			char_id = chr->id;
		}
	}

	if(char_id == (CharacterID)-1) {
		log_debug("Got invalid character: %s", buf);
		return NULL;
	}

	return plrmode_find(char_id, shot_id);
}

void plrmode_preload(PlayerMode *mode, ResourceRefGroup *rg) {
	assert(mode != NULL);
	plrchar_preload(mode->character, rg);

	if(mode->procs.preload) {
		mode->procs.preload(rg);
	}
}
