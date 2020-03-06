/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
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

static Enemy *gap_renderer;

static cmplx reimu_dream_gap_target_pos(Enemy *e) {
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

static int reimu_dream_gap_bomb_projectile(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	if(t == EVENT_DEATH) {
		Sprite *spr = get_sprite("part/blast");

		double range = GAP_LENGTH;
		double damage = 50;

		PARTICLE(
			.sprite_ptr = spr,
			.color = &p->color,
			.pos = p->pos,
			.timeout = 20,
			.draw_rule = pdraw_timeout_scalefade(0, 3 * range / spr->w, 1, 0),
			.layer = LAYER_BOSS + 2,
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
		);

		stage_clear_hazards_at(p->pos, range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
		ent_area_damage(p->pos, range, &(DamageInfo) { damage, DMG_PLAYER_BOMB }, NULL, NULL);
		return ACTION_ACK;
	}

	p->pos += p->args[0];
	p->angle = carg(p->args[0]);

	if(!(t % 3)) {
		double range = GAP_LENGTH * 0.5;
		double damage = 50;
		// Yes, I know, this is inefficient as hell, but I'm too lazy to write a
		// stage_clear_hazards_inside_rectangle function.
		stage_clear_hazards_at(p->pos, range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
		ent_area_damage(p->pos, range, &(DamageInfo) { damage, DMG_PLAYER_BOMB }, NULL, NULL);
	}

	return ACTION_NONE;
}

static void reimu_dream_gap_bomb_projectile_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = p->sprite,
		.shader_ptr = p->shader,
		.color = &p->color,
		.shader_params = &(ShaderCustomParams) {{ p->opacity }},
		.pos = { creal(p->pos), cimag(p->pos) },
		.scale.both = 0.75 * clamp(t / 5.0, 0.1, 1.0),
	});
}

static void reimu_dream_gap_bomb(Enemy *e, int t) {
	if(!(t % 4)) {
		PROJECTILE(
			.sprite = "glowball",
			.size = 32 * (1 + I),
			.color = HSLA(t/30.0, 0.5, 0.5, 0.5),
			.pos = e->pos + e->args[0] * GAP_LENGTH * 0.25 * rng_sreal(),
			.rule = reimu_dream_gap_bomb_projectile,
			.draw_rule = reimu_dream_gap_bomb_projectile_draw,
			.type = PROJ_PLAYER,
			.damage_type = DMG_PLAYER_BOMB,
			.damage = 75,
			.args = { -20 * e->pos0 },
		);

		global.shake_view += 5;

		if(!(t % 16)) {
			play_sound("boon");
		}
	}
}

static int reimu_dream_gap(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		free_ref(creal(e->args[3]));
		return ACTION_ACK;
	}

	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	if(player_is_bomb_active(&global.plr)) {
		reimu_dream_gap_bomb(e, t + cimag(e->args[3]));
	}

	cmplx new_pos = reimu_dream_gap_target_pos(e);

	if(t == 0) {
		e->pos = new_pos;
	} else {
		e->pos += (new_pos - e->pos) * 0.1 * (0.1 + 0.9 * (1 - sqrt(gap_renderer->args[0])));
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

static void reimu_dream_gap_draw_lights(int time, double strength) {
	if(strength <= 0) {
		return;
	}

	r_shader("reimu_gap_light");
	r_uniform_sampler("tex", "gaplight");
	r_uniform_float("time", time / 60.0);
	r_uniform_float("strength", strength);

	FOR_EACH_GAP(gap) {
		const float len = GAP_LENGTH * 3 * sqrt(log(strength + 1) / 0.693);
		cmplx center = gap->pos - gap->pos0 * (len * 0.5 - GAP_WIDTH * 0.6);

		r_mat_mv_push();
		r_mat_mv_translate(creal(center), cimag(center), 0);
		r_mat_mv_rotate(carg(gap->pos0)+M_PI, 0, 0, 1);
		r_mat_mv_scale(len, GAP_LENGTH, 1);
		r_draw_quad();
		r_mat_mv_pop();
	}
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
	bool render_to_fg = r_framebuffer_current() == framebuffers->back;

	if(render_to_fg) {
		fbpair_swap(framebuffers);
		Framebuffer *target_fb = framebuffers->back;

		// This change must propagate
		r_state_pop();
		r_framebuffer(target_fb);
		r_state_push();

		r_shader("reimu_gap");
		r_uniform_int("draw_background", true);
		r_blend(BLEND_NONE);
	} else {
		r_shader("reimu_gap");
		r_uniform_int("draw_background", false);
		r_blend(BLEND_PREMUL_ALPHA);
	}

	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_uniform_float("time", t / (float)FPS);
	r_uniform_vec2("gap_size", GAP_WIDTH/2.0, GAP_LENGTH/2.0);
	r_uniform_vec2_array("gaps[0]", 0, NUM_GAPS, gaps);
	r_uniform_float_array("gap_angles[0]", 0, NUM_GAPS, angles);
	r_uniform_int_array("gap_links[0]", 0, NUM_GAPS, links);
	draw_framebuffer_tex(framebuffers->front, VIEWPORT_W, VIEWPORT_H);

	r_blend(BLEND_PREMUL_ALPHA);

	FOR_EACH_GAP(gap) {
		r_mat_mv_push();
		r_mat_mv_translate(creal(gap->pos), cimag(gap->pos), 0);
		cmplx stretch_vector = gap->args[0];

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

		r_mat_mv_pop();
	}

	reimu_dream_gap_draw_lights(t, pow(e->args[0], 2));
}

static int reimu_dream_gap_renderer(Enemy *e, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	if(player_is_bomb_active(&global.plr)) {
		e->args[0] = approach(e->args[0], 1.0, 0.1);
	} else {
		e->args[0] = approach(e->args[0], 0.0, 0.025);
	}

	return ACTION_NONE;
}

static void reimu_dream_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"yinyang",
		"proj/ofuda",
		"proj/needle2",
		"proj/glowball",
		"part/myon",
		"part/stardust",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"runes",
		"gaplight",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"sprite_yinyang",
		"reimu_gap",
		"reimu_gap_light",
		"reimu_bomb_bg",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_marisa_a",
		"boon",
	NULL);
}

static void reimu_dream_bomb(Player *p) {
	play_sound("bomb_marisa_a");
}

static void reimu_dream_bomb_bg(Player *p) {
	float a = gap_renderer->args[0];
	reimu_common_bomb_bg(p, a);
}


static void reimu_dream_spawn_warp_effect(cmplx pos, bool exit) {
	PARTICLE(
		.sprite = "myon",
		.pos = pos,
		.color = RGBA(0.5, 0.5, 0.5, 0.5),
		.timeout = 20,
		.angle = rng_angle(),
		.draw_rule = pdraw_timeout_scalefade(0.2, 1, 1, 0),
		.layer = LAYER_PLAYER_FOCUS,
	);

	Color *clr = color_mul_scalar(RGBA(0.75, rng_range(0, 0.4), 0.4, 0), 0.8-0.4*exit);
	PARTICLE(
		.sprite = exit ? "stain" : "stardust",
		.pos = pos,
		.color = clr,
		.timeout = 20,
		.angle = rng_angle(),
		.draw_rule = pdraw_timeout_scalefade(0.1, 0.6, 1, 0),
		.layer = LAYER_PLAYER_FOCUS,
	);
}

static void reimu_dream_bullet_warp(Projectile *p, int *warp_count) {
	if(*warp_count < 1) {
		return;
	}

	double p_long_side = max(creal(p->size), cimag(p->size));
	cmplx half = 0.5 * (1 + I);
	Rect p_bbox = { p->pos - p_long_side * half, p->pos + p_long_side * half };

	FOR_EACH_GAP(gap) {
		double a = (carg(-gap->pos0) - carg(p->move.velocity));

		if(fabs(a) < 2*M_PI/3) {
			continue;
		}

		Rect gap_bbox, overlap;
		cmplx gap_size = (GAP_LENGTH + I * GAP_WIDTH) * cexp(I*carg(gap->args[0]));
		cmplx p0 = gap->pos - gap_size * 0.5;
		cmplx p1 = gap->pos + gap_size * 0.5;
		gap_bbox.top_left = min(creal(p0), creal(p1)) + I * min(cimag(p0), cimag(p1));
		gap_bbox.bottom_right = max(creal(p0), creal(p1)) + I * max(cimag(p0), cimag(p1));

		if(rect_rect_intersection(p_bbox, gap_bbox, true, false, &overlap)) {
			cmplx o = (overlap.top_left + overlap.bottom_right) / 2;
			double fract;

			if(creal(gap_size) > cimag(gap_size)) {
				fract = creal(o - gap_bbox.top_left) / creal(gap_size);
			} else {
				fract = cimag(o - gap_bbox.top_left) / cimag(gap_size);
			}

			Enemy *ngap = reimu_dream_gap_get_linked(gap);
			o = ngap->pos + ngap->args[0] * GAP_LENGTH * (1 - fract - 0.5);

			reimu_dream_spawn_warp_effect(gap->pos + gap->args[0] * GAP_LENGTH * (fract - 0.5), false);
			reimu_dream_spawn_warp_effect(o, true);

			// p->args[0] = -cabs(p->args[0]) * ngap->pos0;
			// p->pos = o + p->args[0];
			// p->args[3] += 1;

			cmplx new_vel = -cabs(p->move.velocity) * ngap->pos0;
			real angle_diff = carg(new_vel) - carg(p->move.velocity);

			p->move.velocity *= cdir(angle_diff);
			p->move.acceleration *= cdir(angle_diff);
			p->pos = o + p->move.velocity;
			--*warp_count;
		}
	}
}

TASK(reimu_dream_ofuda, { cmplx pos; cmplx vel; }) {
	Projectile *ofuda = PROJECTILE(
		.proto = pp_ofuda,
		.pos = ARGS.pos,
		.color = RGBA_MUL_ALPHA(1, 1, 1, 0.5),
		.move = move_asymptotic(1.5 * ARGS.vel, ARGS.vel, 0.8),
		.type = PROJ_PLAYER,
		.damage = 60,
		.shader = "sprite_particle",
	);

	BoxedProjectile b_ofuda = ENT_BOX(ofuda);
	ProjectileList trails = { 0 };

	int warp_cnt = 1;
	int t = 0;
	while((ofuda = ENT_UNBOX(b_ofuda)) || trails.first) {
		if(ofuda) {
			reimu_dream_bullet_warp(ofuda, &warp_cnt);
			reimu_common_ofuda_swawn_trail(ofuda, &trails);
		}

		for(Projectile *p = trails.first; p; p = p->next) {
			p->color.g *= 0.95;
		}

		process_projectiles(&trails, false);
		YIELD;
		++t;
	}
}

static void reimu_dream_shot(Player *p) {
	play_loop("generic_shot");

	if(!(global.frames % 6)) {
		for(int i = -1; i < 2; i += 2) {
			cmplx shot_dir = i * ((p->inputflags & INFLAG_FOCUS) ? 1 : I);
			cmplx spread_dir = shot_dir * cexp(I*M_PI*0.5);

			for(int j = -1; j < 2; j += 2) {
				INVOKE_TASK(reimu_dream_ofuda, p->pos + 10 * j * spread_dir, -20.0 * shot_dir);
			}
		}
	}
}

static void reimu_dream_slave_visual(Enemy *e, int t, bool render) {
	if(render) {
		r_draw_sprite(&(SpriteParams) {
			.sprite = "yinyang",
			.shader = "sprite_yinyang",
			.pos = {
				creal(e->pos),
				cimag(e->pos),
			},
			.rotation.angle = global.frames * -6 * DEG2RAD,
			.color = RGB(0.95, 0.75, 1.0),
			.scale.both = 0.5,
		});
	}
}

TASK(reimu_dream_needle, { cmplx pos; cmplx vel; }) {
	Projectile *p = TASK_BIND_UNBOXED(PROJECTILE(
		.proto = pp_needle2,
		.pos = ARGS.pos,
		.color = RGBA_MUL_ALPHA(1, 1, 1, 0.35),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage = 42,
		.shader = "sprite_particle",
	));

	Color *trail_color = color_mul(COLOR_COPY(&p->color), RGBA_MUL_ALPHA(0.75, 0.5, 1, 0.35));
	trail_color->a = 0;

	int warp_cnt = 1;
	for(int t = 0;; ++t) {
		reimu_dream_bullet_warp(p, &warp_cnt);

		PARTICLE(
			.sprite_ptr = p->sprite,
			.color = trail_color,
			.timeout = 12,
			.pos = p->pos,
			.move = move_linear(p->move.velocity * 0.8),
			.draw_rule = pdraw_timeout_scalefade(0, 3, 1, 0),
			.layer = LAYER_PARTICLE_LOW,
			.flags = PFLAG_NOREFLECT,
		);

		YIELD;
	}
}

static int reimu_dream_slave(Enemy *e, int t) {
	if(t < 0) {
		return ACTION_ACK;
	}

	// double a = M_PI * psin(t * 0.1) + creal(e->args[0]) + M_PI/2;
	double a = t * -0.1 + creal(e->args[0]) + M_PI/2;
	cmplx ofs = e->pos0;
	cmplx shotdir = e->args[1];

	if(global.plr.inputflags & INFLAG_FOCUS) {
		ofs = cimag(ofs) + I * creal(ofs);
		shotdir = cimag(shotdir) + I * creal(shotdir);
	}

	if(t == 0) {
		e->pos = global.plr.pos;
	} else {
		double x = creal(ofs);
		double y = cimag(ofs);
		cmplx tpos = global.plr.pos + x * sin(a) + y * I * cos(a);
		e->pos += (tpos - e->pos) * 0.5;
	}

	if(player_should_shoot(&global.plr, true)) {
		if(!((global.frames + 3) % 6)) {
			INVOKE_TASK(reimu_dream_needle, e->pos, 20 * shotdir);
		}
	}

	return ACTION_NONE;
}

static Enemy* reimu_dream_spawn_slave(Player *plr, cmplx pos, cmplx a0, cmplx a1, cmplx a2, cmplx a3) {
	Enemy *e = create_enemy_p(&plr->slaves, pos, ENEMY_IMMUNE, reimu_dream_slave_visual, reimu_dream_slave, a0, a1, a2, a3);
	e->ent.draw_layer = LAYER_PLAYER_SLAVE;
	return e;
}

static void reimu_dream_kill_slaves(EnemyList *slaves) {
	for(Enemy *e = slaves->first, *next; e; e = next) {
		next = e->next;

		if(e->logic_rule == reimu_dream_slave) {
			delete_enemy(slaves, e);
		}
	}
}

static void reimu_dream_respawn_slaves(Player *plr, short npow) {
	reimu_dream_kill_slaves(&plr->slaves);

	int p = 2 * (npow / 100);
	double s = 1;

	for(int i = 0; i < p; ++i, s = -s) {
		reimu_dream_spawn_slave(plr, 48+32*I, ((double)i/p)*(M_PI*2), s*I, 0, 0);
	}
}

static void reimu_dream_power(Player *p, short npow) {
	if(p->power / 100 != npow / 100) {
		reimu_dream_respawn_slaves(p, npow);
	}
}

static Enemy* reimu_dream_spawn_gap(Player *plr, cmplx pos, cmplx a0, cmplx a1, cmplx a2, cmplx a3) {
	Enemy *gap = create_enemy_p(&plr->slaves, pos, ENEMY_IMMUNE, NULL, reimu_dream_gap, a0, a1, a2, a3);
	gap->ent.draw_layer = LAYER_PLAYER_SLAVE;
	return gap;
}

static void reimu_dream_think(Player *plr) {
	if(player_is_bomb_active(plr)) {
		global.shake_view_fade = max(global.shake_view_fade, 5);
	}
}

static void reimu_dream_init(Player *plr) {
	Enemy* left   = reimu_dream_spawn_gap(plr, -1, I, 0, 0, 0);
	Enemy* top    = reimu_dream_spawn_gap(plr, -I, 1, 0, 0, 0);
	Enemy* right  = reimu_dream_spawn_gap(plr,  1, I, 0, 0, 0);
	Enemy* bottom = reimu_dream_spawn_gap(plr,  I, 1, 0, 0, 0);

	reimu_dream_gap_link(top, left);
	reimu_dream_gap_link(bottom, right);

	gap_renderer= create_enemy_p(&plr->slaves, 0, ENEMY_IMMUNE, reimu_dream_gap_renderer_visual, reimu_dream_gap_renderer, 0, 0, 0, 0);
	gap_renderer->ent.draw_layer = LAYER_PLAYER_FOCUS;

	int idx = 0;
	FOR_EACH_GAP(gap) {
		gap->args[3] = creal(gap->args[3]) + I*idx++;
	}

	reimu_dream_respawn_slaves(plr, plr->power);
	reimu_common_bomb_buffer_init();
}

PlayerMode plrmode_reimu_b = {
	.name = "Dream Shaper",
	.description = "Turn your understanding of reality upside-down, and tie the boundaries of spacetime into knots as easily as the ribbons in your hair.",
	.spellcard_name = "Dream Sign “Ethereal Boundary Rupture”",
	.character = &character_reimu,
	.dialog = &dialog_tasks_reimu,
	.shot_mode = PLR_SHOT_REIMU_DREAM,
	.procs = {
		.property = reimu_common_property,
		.bomb = reimu_dream_bomb,
		.bombbg = reimu_dream_bomb_bg,
		.shot = reimu_dream_shot,
		.power = reimu_dream_power,
		.init = reimu_dream_init,
		.preload = reimu_dream_preload,
		.think = reimu_dream_think,
	},
};
