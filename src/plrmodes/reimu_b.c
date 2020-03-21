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

#define SHOT_FORWARD_DMG 60
#define SHOT_FORWARD_DELAY 6

#define SHOT_SLAVE_DMG 42
#define SHOT_SLAVE_PRE_DELAY 3
#define SHOT_SLAVE_POST_DELAY 3

#define ORB_RETRACT_TIME 4

#define GAP_LENGTH 128
#define GAP_WIDTH 16
#define GAP_OFFSET 8
#define GAP_LIMIT 10

// #define GAP_LENGTH 160
// #define GAP_WIDTH 160
// #define GAP_OFFSET 82

#define NUM_GAPS 4
#define FOR_EACH_GAP(gap) for(Enemy *gap = global.plr.slaves.first; gap; gap = gap->next) if(gap->logic_rule == reimu_dream_gap)

typedef struct ReimuGap ReimuGap;
typedef struct ReimuBController ReimuBController;

struct ReimuGap {
	ReimuGap *link;      // bullets entering this gap will exit from the linked gap
	cmplx pos;           // position of the gap's center in viewport space
	cmplx orientation;   // normalized, points to the side the gap is 'attached' to
	cmplx parallel_axis; // normalized, parallel the gap, perpendicular to orientation
};

typedef struct ReimuBController {
	ENTITY_INTERFACE_NAMED(ReimuBController, gap_renderer);
	Player *plr;
	ShaderProgram *yinyang_shader;
	Sprite *yinyang_sprite;

	union {
		ReimuGap array[NUM_GAPS];
		struct {
			ReimuGap left, top, right, bottom;
		};
	} gaps;

	COEVENTS_ARRAY(
		slaves_expired
	) events;

	real bomb_alpha;
} ReimuBController;

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

static void reimu_dream_draw_gap_lights(ReimuBController *ctrl, int time, real strength) {
	if(strength <= 0) {
		return;
	}

	r_shader("reimu_gap_light");
	r_uniform_sampler("tex", "gaplight");
	r_uniform_float("time", time / 60.0f);
	r_uniform_float("strength", strength);

	real len = GAP_LENGTH * 3 * sqrt(log1p(strength + 1) / 0.693f);

	for(int i = 0; i < NUM_GAPS; ++i) {
		ReimuGap *gap = ctrl->gaps.array + i;
		cmplx center = gap->pos - gap->orientation * (len * 0.5 - GAP_WIDTH * 0.6);

		r_mat_mv_push();
		r_mat_mv_translate(creal(center), cimag(center), 0);
		r_mat_mv_rotate(carg(gap->orientation) + M_PI, 0, 0, 1);
		r_mat_mv_scale(len, GAP_LENGTH, 1);
		r_draw_quad();
		r_mat_mv_pop();
	}
}

static void reimu_dream_draw_gaps(EntityInterface *gap_renderer_ent) {
	ReimuBController *ctrl = ENT_CAST_CUSTOM(gap_renderer_ent, ReimuBController);

	float gaps[NUM_GAPS][2];
	float angles[NUM_GAPS];
	int links[NUM_GAPS];

	for(int i = 0; i < NUM_GAPS; ++i) {
		ReimuGap *gap = ctrl->gaps.array + i;
		gaps[i][0] = creal(gap->pos);
		gaps[i][1] = cimag(gap->pos);
		angles[i] = -carg(gap->orientation);
		links[i] = gap->link - ctrl->gaps.array;
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
		r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);
	} else {
		r_shader("reimu_gap");
		r_uniform_int("draw_background", false);
		r_blend(BLEND_PREMUL_ALPHA);
	}

	r_uniform_vec2("viewport", VIEWPORT_W, VIEWPORT_H);
	r_uniform_float("time", global.frames / 60.0f);
	r_uniform_vec2("gap_size", GAP_WIDTH/2.0f, GAP_LENGTH/2.0f);
	r_uniform_vec2_array("gaps[0]", 0, NUM_GAPS, gaps);
	r_uniform_float_array("gap_angles[0]", 0, NUM_GAPS, angles);
	r_uniform_int_array("gap_links[0]", 0, NUM_GAPS, links);
	draw_framebuffer_tex(framebuffers->front, VIEWPORT_W, VIEWPORT_H);

	r_blend(BLEND_PREMUL_ALPHA);

	SpriteParams yinyang = {
		.sprite_ptr = ctrl->yinyang_sprite,
		.shader_ptr = ctrl->yinyang_shader,
		.rotation.angle = global.frames * -6 * DEG2RAD,
		.color = RGB(0.95, 0.75, 1.0),
		.scale.both = 0.5,
	};

	for(int i = 0; i < NUM_GAPS; ++i) {
		ReimuGap *gap = ctrl->gaps.array + i;
		yinyang.pos.as_cmplx = gap->pos + gap->parallel_axis * -0.5 * GAP_LENGTH;
		r_draw_sprite(&yinyang);
		yinyang.pos.as_cmplx = gap->pos + gap->parallel_axis * +0.5 * GAP_LENGTH;
		r_draw_sprite(&yinyang);
	}

	reimu_dream_draw_gap_lights(ctrl, global.frames, ctrl->bomb_alpha * ctrl->bomb_alpha);
}

static void reimu_dream_bomb(Player *p) {
	play_sound("bomb_marisa_a");
}

static void reimu_dream_bomb_bg(Player *p) {
	// float a = gap_renderer->args[0];
	// reimu_common_bomb_bg(p, a);
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

static void reimu_dream_bullet_warp(ReimuBController *ctrl, Projectile *p, int *warp_count) {
	if(*warp_count < 1) {
		return;
	}

	real p_long_side = max(creal(p->size), cimag(p->size));
	cmplx half = 0.25 * (1 + I);
	Rect p_bbox = { p->pos - p_long_side * half, p->pos + p_long_side * half };

	for(int i = 0; i < NUM_GAPS; ++i) {
		ReimuGap *gap = ctrl->gaps.array + i;
		real a = (carg(-gap->orientation) - carg(p->move.velocity));

		if(fabs(a) < M_TAU/3) {
			continue;
		}

		Rect gap_bbox, overlap;
		cmplx gap_size = (GAP_LENGTH + I * GAP_WIDTH) * gap->parallel_axis;
		cmplx p0 = gap->pos - gap_size * 0.5;
		cmplx p1 = gap->pos + gap_size * 0.5;
		gap_bbox.top_left = min(creal(p0), creal(p1)) + I * min(cimag(p0), cimag(p1));
		gap_bbox.bottom_right = max(creal(p0), creal(p1)) + I * max(cimag(p0), cimag(p1));

		if(rect_rect_intersection(p_bbox, gap_bbox, true, false, &overlap)) {
			cmplx o = (overlap.top_left + overlap.bottom_right) / 2;
			real fract;

			if(creal(gap_size) > cimag(gap_size)) {
				fract = 1 - creal(o - gap_bbox.top_left) / creal(gap_size);
			} else {
				fract = 1 - cimag(o - gap_bbox.top_left) / cimag(gap_size);
			}

			ReimuGap *ngap = gap->link;
			o = ngap->pos + ngap->parallel_axis * GAP_LENGTH * (1 - fract - 0.5);

			reimu_dream_spawn_warp_effect(gap->pos + gap->parallel_axis * GAP_LENGTH * (fract - 0.5), false);
			reimu_dream_spawn_warp_effect(o, true);

			cmplx new_vel = -cabs(p->move.velocity) * ngap->orientation;
			real angle_diff = carg(new_vel) - carg(p->move.velocity);

			p->move.velocity *= cdir(angle_diff);
			p->move.acceleration *= cdir(angle_diff);
			p->pos = o - p->move.velocity;

			--*warp_count;
		}
	}
}

static void reimu_dream_shot(Player *p) {
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

TASK(reimu_dream_needle, {
	ReimuBController *ctrl;
	cmplx pos;
	cmplx vel;
	ShaderProgram *shader;
}) {
	ReimuBController *ctrl = ARGS.ctrl;
	Projectile *p = TASK_BIND_UNBOXED(PROJECTILE(
		.proto = pp_needle2,
		.pos = ARGS.pos,
		.color = RGBA_MUL_ALPHA(1, 1, 1, 0.35),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage = SHOT_SLAVE_DMG,
		.shader_ptr = ARGS.shader,
	));

	Color *trail_color = color_mul_scalar(RGBA(0.75, 0.5, 1, 0), 0.35);
	int warp_cnt = 1;

	for(;;) {
		reimu_dream_bullet_warp(ctrl, p, &warp_cnt);

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

TASK(reimu_dream_slave_shot, {
	ReimuBController *ctrl;
	BoxedEnemy e;
	cmplx vel;
}) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	Enemy *e = TASK_BIND(ARGS.e);
	cmplx vel = ARGS.vel;
	ShaderProgram *shader = r_shader_get("sprite_particle");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		WAIT(SHOT_SLAVE_PRE_DELAY);
		INVOKE_TASK(reimu_dream_needle,
			.ctrl = ctrl,
			.pos = e->pos,
			.vel = (plr->inputflags & INFLAG_FOCUS) ? cswap(vel) : vel,
			.shader = shader
		);
		WAIT(SHOT_SLAVE_POST_DELAY);
	}
}

TASK(reimu_dream_slave, {
	ReimuBController *ctrl;
	cmplx offset;
	real angle_offset;
	cmplx shot_dir;
}) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	Enemy *e = TASK_BIND_UNBOXED(create_enemy_p(&plr->slaves, 0, ENEMY_IMMUNE, reimu_dream_slave_visual, NULL, 0, 0, 0, 0));

	e->ent.draw_layer = LAYER_PLAYER_SLAVE;
	e->pos = plr->pos;

	INVOKE_TASK_WHEN(&ctrl->events.slaves_expired, reimu_common_slave_expire,
		.player = ENT_BOX(plr),
		.slave = ENT_BOX(e),
		.slave_main_task = THIS_TASK,
		.retract_time = ORB_RETRACT_TIME
	);

	real angle = ARGS.angle_offset + M_PI/2;
	cmplx offset = ARGS.offset;

	INVOKE_SUBTASK(reimu_dream_slave_shot,
		.ctrl = ctrl,
		.e = ENT_BOX(e),
		.vel = 20 * ARGS.shot_dir
	);

	for(;;) {
		cmplx ofs = (plr->inputflags & INFLAG_FOCUS) ? cswap(offset) : offset;
		ofs = cwmul(ofs, cdir(angle));
		capproach_asymptotic_p(&e->pos, plr->pos + ofs, 0.5, 1e-5);
		angle += 0.1;
		YIELD;
	}
}

static void reimu_dream_respawn_slaves(ReimuBController *ctrl, int num_slaves) {
	coevent_signal(&ctrl->events.slaves_expired);
	cmplx shot_dir = -I;

	for(int i = 0; i < num_slaves; ++i, shot_dir *= -1) {
		INVOKE_TASK(reimu_dream_slave,
			.ctrl = ctrl,
			.offset = 48 + 32*I,
			.angle_offset = (i * M_TAU) / num_slaves,
			.shot_dir = shot_dir
		);
	}
}

static cmplx reimu_dream_gaps_reference_pos(ReimuBController *ctrl, cmplx vp) {
	cmplx rp = ctrl->plr->pos;

	if(!(ctrl->plr->inputflags & INFLAG_FOCUS)) {
		rp = CMPLX(creal(rp), cimag(vp) - cimag(rp));
	} else {
		// rp = CMPLX(creal(vp) - creal(rp), cimag(rp));
	}

	return rp;
}

static cmplx reimu_dream_gap_pos(cmplx vp, cmplx ref_pos, cmplx orientation, cmplx parallel_axis, cmplx ofs_factors) {
	cmplx p = cwmul(vp, orientation) + cwmul(ref_pos, parallel_axis);
	cmplx ofs = (1 + I) * GAP_OFFSET + (GAP_LENGTH * 0.5 + GAP_LIMIT) * cwmul(ofs_factors, parallel_axis);
	return cwclamp(p, ofs, vp - ofs);
}

TASK(reimu_dream_process_gaps, { ReimuBController *ctrl; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	cmplx vp = VIEWPORT_W + VIEWPORT_H * I;
	cmplx ref_pos = reimu_dream_gaps_reference_pos(ctrl, vp);
	cmplx ofs_factors = 1 + I;

	for(int i = 0; i < NUM_GAPS; ++i) {
		ReimuGap *gap = ctrl->gaps.array + i;
		gap->pos = reimu_dream_gap_pos(vp, ref_pos, gap->orientation, gap->parallel_axis, ofs_factors);
		gap->pos += (2 * GAP_WIDTH + GAP_OFFSET) * gap->orientation;
	}

	for(;;) {
		YIELD;
		bool is_focused = plr->inputflags & INFLAG_FOCUS;
		real pos_approach_rate = 0.1 * (0.1 + 0.9 * (1 - sqrt(ctrl->bomb_alpha)));

		ref_pos = reimu_dream_gaps_reference_pos(ctrl, vp);
		ofs_factors = 1 + I * !is_focused;  // no extra clamping of vertical gaps if focused

		for(int i = 0; i < NUM_GAPS; ++i) {
			ReimuGap *gap = ctrl->gaps.array + i;
			cmplx target_pos = reimu_dream_gap_pos(vp, ref_pos, gap->orientation, gap->parallel_axis, ofs_factors);
			capproach_asymptotic_p(&gap->pos, target_pos, pos_approach_rate, 1e-5);
		}
	}
}

TASK(reimu_dream_ofuda, { ReimuBController *ctrl; cmplx pos; cmplx vel; ShaderProgram *shader; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Projectile *ofuda = TASK_BIND_UNBOXED(PROJECTILE(
		.proto = pp_ofuda,
		.pos = ARGS.pos,
		.color = RGBA_MUL_ALPHA(1, 1, 1, 0.5),
		.move = move_asymptotic(1.5 * ARGS.vel, ARGS.vel, 0.8),
		.type = PROJ_PLAYER,
		.damage = SHOT_FORWARD_DMG,
		.shader_ptr = ARGS.shader,
	));

	int warp_cnt = 1;

	for(;;) {
		reimu_dream_bullet_warp(ctrl, ofuda, &warp_cnt);
		reimu_common_ofuda_swawn_trail(ofuda);
		YIELD;
	}
}

TASK(reimu_dream_shot_forward, { ReimuBController *ctrl; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	real dir = 1;
	ShaderProgram *ofuda_shader = r_shader_get("sprite_particle");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		play_loop("generic_shot");

		for(int i = -1; i < 2; i += 2) {
			cmplx shot_dir = i * ((plr->inputflags & INFLAG_FOCUS) ? 1 : I);
			cmplx spread_dir = shot_dir * cexp(I*M_PI*0.5);

			for(int j = -1; j < 2; j += 2) {
				INVOKE_TASK(reimu_dream_ofuda,
					.ctrl = ctrl,
					.pos = plr->pos + 10 * j * spread_dir,
					.vel = 20.0 * shot_dir,
					.shader = ofuda_shader
   				);
			}
		}

		dir = -dir;
		WAIT(SHOT_FORWARD_DELAY);
	}
}

TASK(reimu_dream_power_handler, { ReimuBController *ctrl; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	int old_power = plr->power / 100;

	reimu_dream_respawn_slaves(ctrl, old_power * 2);

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.power_changed);
		int new_power = plr->power / 100;
		if(old_power != new_power) {
			reimu_dream_respawn_slaves(ctrl, new_power * 2);
			old_power = new_power;
		}
	}
}

TASK(reimu_dream_controller_tick, { ReimuBController *ctrl; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	for(;;) {
		if(player_is_bomb_active(plr)) {
			global.shake_view_fade = max(global.shake_view_fade, 5);
			approach_p(&ctrl->bomb_alpha, 1.0, 0.1);
		} else {
			approach_p(&ctrl->bomb_alpha, 0.0, 0.025);
		}

		YIELD;
	}
}

TASK(reimu_dream_controller, { BoxedPlayer plr; }) {
	ReimuBController ctrl = { 0 };
	ctrl.plr = TASK_BIND(ARGS.plr);
	COEVENT_INIT_ARRAY(ctrl.events);

	ctrl.yinyang_shader = r_shader_get("sprite_yinyang");
	ctrl.yinyang_sprite = get_sprite("yinyang");

	ctrl.gap_renderer.draw_func = reimu_dream_draw_gaps;
	ctrl.gap_renderer.draw_layer = LAYER_PLAYER_FOCUS;
	ent_register(&ctrl.gap_renderer, ENT_CUSTOM);

	ctrl.gaps.left.link = &ctrl.gaps.top;
	ctrl.gaps.top.link = &ctrl.gaps.left;
	ctrl.gaps.right.link = &ctrl.gaps.bottom;
	ctrl.gaps.bottom.link = &ctrl.gaps.right;

	ctrl.gaps.left.orientation = -1;
	ctrl.gaps.right.orientation = 1;
	ctrl.gaps.top.orientation = -I;
	ctrl.gaps.bottom.orientation = I;

	ctrl.gaps.left.parallel_axis = ctrl.gaps.right.parallel_axis = I;
	ctrl.gaps.top.parallel_axis = ctrl.gaps.bottom.parallel_axis = 1;

	INVOKE_SUBTASK(reimu_dream_controller_tick, &ctrl);
	INVOKE_SUBTASK(reimu_dream_process_gaps, &ctrl);
	INVOKE_SUBTASK(reimu_dream_shot_forward, &ctrl);
	INVOKE_SUBTASK(reimu_dream_power_handler, &ctrl);

	WAIT_EVENT(&TASK_EVENTS(THIS_TASK)->finished);
	COEVENT_CANCEL_ARRAY(ctrl.events);
	ent_unregister(&ctrl.gap_renderer);
}

static void reimu_dream_init(Player *plr) {
	reimu_common_bomb_buffer_init();
	INVOKE_TASK(reimu_dream_controller, ENT_BOX(plr));
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
		.init = reimu_dream_init,
		.preload = reimu_dream_preload,
	},
};
