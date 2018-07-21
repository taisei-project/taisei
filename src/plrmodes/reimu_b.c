/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "reimu.h"
#include "stagedraw.h"

#define GAP_LENGTH 64
#define GAP_WIDTH 8
#define GAP_OFFSET 8

static int reimu_dream_gap(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		return ACTION_ACK;
	}

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	double x, y;

	if(creal(e->pos0)) {
		x = creal(e->pos0) * VIEWPORT_W;
	} else {
		x = creal(global.plr.pos);
	}

	if(cimag(e->pos0)) {
		y = cimag(e->pos0) * VIEWPORT_H;
	} else {
		y = cimag(global.plr.pos);
	}

	x = clamp(x, GAP_OFFSET, VIEWPORT_W - GAP_OFFSET);
	y = clamp(y, GAP_OFFSET, VIEWPORT_H - GAP_OFFSET);

	if(global.plr.inputflags & INFLAG_FOCUS) {
		if(cimag(e->pos0)) {
			x = VIEWPORT_W - x;
		}
	} else {
		if(creal(e->pos0)) {
			y = VIEWPORT_H - y;
		}
	}

	complex new_pos = x + I * y;
	e->pos += (new_pos - e->pos) * 0.1;
	// e->pos = x + I * y;

	return ACTION_NONE;
}

static void reimu_dream_gap_renderer_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	#define NUM_GAPS 4
	float gaps[NUM_GAPS][4];
	float angles[NUM_GAPS];
	float flipmasks[NUM_GAPS][2];

	int i = 0;

	for(Enemy *gap = global.plr.slaves.first; i < NUM_GAPS; gap = gap->next) {
		if(gap->logic_rule != reimu_dream_gap) {
			continue;
		}

		angles[i] = carg(gap->pos0);
		flipmasks[i][0] = (float)(creal(gap->pos0) != 0);
		flipmasks[i][1] = (float)(cimag(gap->pos0) != 0);
		gaps[i][0] = creal(gap->pos);
		gaps[i][1] = cimag(gap->pos);
		gaps[i][2] = GAP_WIDTH/2.0;
		gaps[i][3] = GAP_LENGTH/2.0;

		++i;
	}

	FBPair *framebuffers = stage_get_fbpair(FBPAIR_FG);

	fbpair_swap(framebuffers);
	Framebuffer *target_fb = framebuffers->back;

	// This change must propagate
	r_state_pop();
	r_framebuffer(target_fb);
	r_state_push();

	r_shader("reimu_gap");
	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_uniform_float("time", t / (float)FPS);
	r_uniform("gaps[0]", NUM_GAPS, gaps);
	r_uniform("gap_angles[0]", NUM_GAPS, angles);
	r_uniform("gap_flipmasks[0]", NUM_GAPS, flipmasks);
	draw_framebuffer_tex(framebuffers->front, VIEWPORT_W, VIEWPORT_H);

	i = 0;

	for(Enemy *gap = global.plr.slaves.first; i < NUM_GAPS; gap = gap->next) {
		if(gap->logic_rule != reimu_dream_gap) {
			continue;
		}

		r_mat_push();
		r_mat_translate(creal(gap->pos), cimag(gap->pos), 0);
		complex stretch_vector = gap->args[0];

		for(float ofs = -0.5; ofs <= 0.5; ofs += 1) {
			r_draw_sprite(&(SpriteParams) {
				.sprite = "yinyang",
				.shader = "sprite_yinyang",
				.pos = {
					creal(stretch_vector) * GAP_LENGTH * ofs,
					cimag(stretch_vector) * GAP_LENGTH * ofs
				},
				.rotation.angle = global.frames * -6 * DEG2RAD,
				.color = rgb(0.95, 0.75, 1.0),
				.scale.both = 0.5,
			});
		}

		r_mat_pop();
		++i;
	}
}

static int reimu_dream_gap_renderer(Enemy *e, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	return ACTION_NONE;
}

static void reimu_dream_preload(void) {
	const int flags = RESF_DEFAULT;

	//preload_resources(RES_TEXTURE, flags,
	//NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"reimu_gap",
	NULL);

	//preload_resources(RES_SFX, flags | RESF_OPTIONAL,
	//NULL);
}

static void reimu_dream_bomb(Player *p) {
}

static void reimu_dream_shot(Player *p) {
}

static void reimu_dream_power(Player *p, short npow) {
}

static Enemy* reimu_dream_gap_spawn(EnemyList *elist, complex pos, complex a0, complex a1, complex a2, complex a3) {
	Enemy *gap = create_enemy_p(elist, pos, ENEMY_IMMUNE, NULL, reimu_dream_gap, a0, a1, a2, a3);
	gap->ent.draw_layer = LAYER_PLAYER_SLAVE;
	return gap;
}

static void reimu_dream_init(Player *plr) {
	reimu_dream_gap_spawn(&plr->slaves, -1, I, 0, 0, 0);
	reimu_dream_gap_spawn(&plr->slaves,  1, I, 0, 0, 0);
	reimu_dream_gap_spawn(&plr->slaves, -I, 1, 0, 0, 0);
	reimu_dream_gap_spawn(&plr->slaves,  I, 1, 0, 0, 0);

	Enemy *r = create_enemy_p(&plr->slaves, 0, ENEMY_IMMUNE, reimu_dream_gap_renderer_visual, reimu_dream_gap_renderer, 0, 0, 0, 0);
	r->ent.draw_layer = LAYER_PLAYER_FOCUS;
}

PlayerMode plrmode_reimu_b = {
	.name = "Dream Sign",
	.character = &character_reimu,
	.dialog = &dialog_reimu,
	.shot_mode = PLR_SHOT_REIMU_DREAM,
	.procs = {
		.property = reimu_common_property,
		.bomb = reimu_dream_bomb,
		.shot = reimu_dream_shot,
		.power = reimu_dream_power,
		.init = reimu_dream_init,
		.preload = reimu_dream_preload,
	},
};
