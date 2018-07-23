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

#define GAP_LENGTH 128
#define GAP_WIDTH 16
#define GAP_OFFSET 8
#define GAP_LIMIT 10

// #define GAP_LENGTH 160
// #define GAP_WIDTH 160
// #define GAP_OFFSET 82

#define NUM_GAPS 4
#define FOR_EACH_GAP(gap) for(Enemy *gap = global.plr.slaves.first; gap; gap = gap->next) if(gap->logic_rule == reimu_dream_gap)

static complex reimu_dream_gap_target_pos(Enemy *e) {
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

	bool focus = global.plr.inputflags & INFLAG_FOCUS;

	// complex ofs = GAP_OFFSET * (1 + I) + (GAP_LENGTH * 0.5 + GAP_LIMIT) * e->args[0];
	double ofs_x = GAP_OFFSET + (GAP_LENGTH * 0.5 + GAP_LIMIT) * creal(e->args[0]);
	double ofs_y = GAP_OFFSET + (!focus) * (GAP_LENGTH * 0.5 + GAP_LIMIT) * cimag(e->args[0]);

	x = clamp(x, ofs_x, VIEWPORT_W - ofs_x);
	y = clamp(y, ofs_y, VIEWPORT_H - ofs_y);

	if(focus) {
		/*
		if(cimag(e->pos0)) {
			x = VIEWPORT_W - x;
		}
		*/
	} else {
		if(creal(e->pos0)) {
			y = VIEWPORT_H - y;
		}
	}

	return x + I * y;
}

static int reimu_dream_gap(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		free_ref(creal(e->args[3]));
		return ACTION_ACK;
	}

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	complex new_pos = reimu_dream_gap_target_pos(e);

	if(t == 0) {
		e->pos = new_pos;
	} else {
		e->pos += (new_pos - e->pos) * 0.1;
	}

	return ACTION_NONE;
}

static void reimu_dream_gap_link(Enemy *g0, Enemy *g1) {
	int ref0 = add_ref(g0);
	int ref1 = add_ref(g1);
	g0->args[3] = ref1;
	g1->args[3] = ref0;
}

static Enemy* reimu_dream_gap_get_linked(Enemy *gap) {
	return REF(creal(gap->args[3]));
}

static void reimu_dream_gap_renderer_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	float gaps[NUM_GAPS][2];
	float angles[NUM_GAPS];
	int links[NUM_GAPS];

	int i = 0;
	FOR_EACH_GAP(gap) {
		gaps[i][0] = creal(gap->pos);
		gaps[i][1] = cimag(gap->pos);
		angles[i] = carg(gap->pos0);
		links[i] = cimag(reimu_dream_gap_get_linked(gap)->args[3]);
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
	r_uniform_vec2("gap_size", GAP_WIDTH/2.0, GAP_LENGTH/2.0);
	r_uniform("gaps[0]", NUM_GAPS, gaps);
	r_uniform("gap_angles[0]", NUM_GAPS, angles);
	r_uniform("gap_links[0]", NUM_GAPS, links);
	draw_framebuffer_tex(framebuffers->front, VIEWPORT_W, VIEWPORT_H);

	FOR_EACH_GAP(gap) {
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
				.color = RGB(0.95, 0.75, 1.0),
				.scale.both = 0.5,
			});
		}

		r_mat_pop();
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

	preload_resources(RES_SHADER_PROGRAM, flags,
		"reimu_gap",
	NULL);
}

static void reimu_dream_bomb(Player *p) {
}

static void reimu_dream_bullet_warp(Projectile *p, int t) {
	if(creal(p->args[3]) > 0 /*global.plr.power / 100*/) {
		return;
	}

	double p_long_side = max(creal(p->size), cimag(p->size));
	complex half = 0.5 * (1 + I);
	Rect p_bbox = { p->pos - p_long_side * half, p->pos + p_long_side * half };

	FOR_EACH_GAP(gap) {
		double a = (carg(-gap->pos0) - carg(p->args[0]));

		if(fabs(a) < 2*M_PI/3) {
			continue;
		}

		Rect gap_bbox, overlap;
		complex gap_size = (GAP_LENGTH + I * GAP_WIDTH) * cexp(I*carg(gap->args[0]));
		complex p0 = gap->pos - gap_size * 0.5;
		complex p1 = gap->pos + gap_size * 0.5;
		gap_bbox.top_left = min(creal(p0), creal(p1)) + I * min(cimag(p0), cimag(p1));
		gap_bbox.bottom_right = max(creal(p0), creal(p1)) + I * max(cimag(p0), cimag(p1));

		if(rect_rect_intersection(p_bbox, gap_bbox, true, &overlap)) {
			complex o = (overlap.top_left + overlap.bottom_right) / 2;
			double fract;

			if(creal(gap_size) > cimag(gap_size)) {
				fract = creal(o - gap_bbox.top_left) / creal(gap_size);
			} else {
				fract = cimag(o - gap_bbox.top_left) / cimag(gap_size);
			}

			// fract = 0.1 + 0.8 * psin(global.frames * 0.93814);

			Enemy *ngap = reimu_dream_gap_get_linked(gap);
			o = ngap->pos + ngap->args[0] * GAP_LENGTH * (1 - fract - 0.5);

			p->args[0] = -cabs(p->args[0]) * ngap->pos0;
			p->pos = o;
			p->args[3] += 1;
		}
	}
}

static int reimu_dream_ofuda(Projectile *p, int t) {
	if(t >= 0) {
		reimu_dream_bullet_warp(p, t);
	}

	complex ov = p->args[0];
	double s = cabs(ov);
	p->args[0] *= clamp(s * (1.5 - t / 10.0), s*0.5, 1.5*s) / s;
	int r = reimu_common_ofuda(p, t);
	p->args[0] = ov;

	return r;
}

static int reimu_dream_needle(Projectile *p, int t) {
	if(t >= 0) {
		reimu_dream_bullet_warp(p, t);
	}

	p->angle = carg(p->args[0]);

	if(t < 0) {
		return ACTION_ACK;
	}

	p->pos += p->args[0];

	Color *c = color_mul(COLOR_COPY(&p->color), RGBA_MUL_ALPHA(0.75, 0.5, 1, 0.5));
	c->a = 0;

	PARTICLE(
		.sprite_ptr = p->sprite,
		.color = c,
		.timeout = 12,
		.pos = p->pos,
		.args = { p->args[0] * 0.8, 0, 0+3*I },
		.rule = linear,
		.draw_rule = ScaleFade,
		.layer = LAYER_PARTICLE_LOW,
		.flags = PFLAG_NOREFLECT,
	);

	return ACTION_NONE;
}

static void reimu_dream_shot(Player *p) {
	play_loop("generic_shot");
	int dmg = 75;

	if(!(global.frames % 6)) {
		for(int i = -1; i < 2; i += 2) {
			complex shot_dir = i * ((p->inputflags & INFLAG_FOCUS) ? 1 : I);
			complex spread_dir = shot_dir * cexp(I*M_PI*0.5);

			if(!(global.frames % 12)) {
				PROJECTILE(
					.proto = pp_needle,
					.pos = p->pos,
					.color = RGBA_MUL_ALPHA(1, 1, 1, 0.5),
					.rule = reimu_dream_needle,
					.args = { -20.0 * shot_dir },
					.type = PlrProj+dmg*2,
					.shader = "sprite_default",
				);
			} else {
				for(int j = -1; j < 2; j += 2) {
					PROJECTILE(
						.proto = pp_ofuda,
						.pos = p->pos + 10 * j * spread_dir,
						.color = RGBA_MUL_ALPHA(1, 1, 1, 0.5),
						.rule = reimu_dream_ofuda,
						.args = { -20.0 * shot_dir },
						.type = PlrProj+dmg,
						.shader = "sprite_default",
					);
				}
			}
		}
	}
}

static void reimu_dream_power(Player *p, short npow) {
}

static Enemy* reimu_dream_gap_spawn(EnemyList *elist, complex pos, complex a0, complex a1, complex a2, complex a3) {
	Enemy *gap = create_enemy_p(elist, pos, ENEMY_IMMUNE, NULL, reimu_dream_gap, a0, a1, a2, a3);
	gap->ent.draw_layer = LAYER_PLAYER_SLAVE;
	return gap;
}

static void reimu_dream_init(Player *plr) {
	Enemy* left   = reimu_dream_gap_spawn(&plr->slaves, -1, I, 0, 0, 0);
	Enemy* top    = reimu_dream_gap_spawn(&plr->slaves, -I, 1, 0, 0, 0);
	Enemy* right  = reimu_dream_gap_spawn(&plr->slaves,  1, I, 0, 0, 0);
	Enemy* bottom = reimu_dream_gap_spawn(&plr->slaves,  I, 1, 0, 0, 0);

	reimu_dream_gap_link(top, left);
	reimu_dream_gap_link(bottom, right);

	Enemy *r = create_enemy_p(&plr->slaves, 0, ENEMY_IMMUNE, reimu_dream_gap_renderer_visual, reimu_dream_gap_renderer, 0, 0, 0, 0);
	r->ent.draw_layer = LAYER_PLAYER_FOCUS;

	int idx = 0;
	FOR_EACH_GAP(gap) {
		gap->args[3] = creal(gap->args[3]) + I*idx++;
	}
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
