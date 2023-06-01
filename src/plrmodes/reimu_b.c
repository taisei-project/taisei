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
#include "common_tasks.h"

#define SHOT_FORWARD_DMG 60
#define SHOT_FORWARD_DELAY 6

#define SHOT_SLAVE_DMG 42
#define SHOT_SLAVE_PRE_DELAY 3
#define SHOT_SLAVE_POST_DELAY 3

#define BOMB_PROJECTILE_DAMAGE 75
#define BOMB_PROJECTILE_FIRE_DELAY 4
#define BOMB_PROJECTILE_PERIODIC_AREA_DAMAGE 50
#define BOMB_PROJECTILE_PERIODIC_AREA_RADIUS (GAP_LENGTH * 0.5)
#define BOMB_PROJECTILE_PERIODIC_DELAY 3
#define BOMB_PROJECTILE_IMPACT_AREA_DAMAGE 50
#define BOMB_PROJECTILE_IMPACT_AREA_RADIUS GAP_LENGTH

#define ORB_RETRACT_TIME 4

#define GAP_LENGTH 128
#define GAP_WIDTH 16
#define GAP_OFFSET 8
#define GAP_LIMIT 10

// #define GAP_LENGTH 160
// #define GAP_WIDTH 160
// #define GAP_OFFSET 82

#define NUM_GAPS 4

typedef struct ReimuBGap ReimuBGap;

struct ReimuBGap {
	ReimuBGap *link;     // bullets entering this gap will exit from the linked gap
	cmplx pos;           // position of the gap's center in viewport space
	cmplx orientation;   // normalized, points to the side the gap is 'attached' to
	cmplx parallel_axis; // normalized, parallel the gap, perpendicular to orientation
};

DEFINE_ENTITY_TYPE(ReimuBSlave, {
	Sprite *sprite;
	ShaderProgram *shader;
	cmplx pos;
	uint alive;
});

DEFINE_ENTITY_TYPE(ReimuBController, {
	Player *plr;
	ShaderProgram *yinyang_shader;
	Sprite *yinyang_sprite;

	union {
		ReimuBGap array[NUM_GAPS];
		struct {
			ReimuBGap left, top, right, bottom;
		};
	} gaps;

	COEVENTS_ARRAY(
		slaves_expired
	) events;

	real bomb_alpha;
});

static void reimu_dream_gap_bomb_projectile_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	sp.scale.as_cmplx = 0.75 * clamp(t / 5.0, 0.1, 1.0) * (1 + I);
	r_draw_sprite(&sp);
}

TASK(reimu_dream_gap_bomb_projectile_impact, { BoxedProjectile p; Sprite *impact_sprite; }) {
	Projectile *p = TASK_BIND(ARGS.p);

	if(p->collision && p->collision->type == PCOL_VOID) {
		// Auto-removed out of bounds; don't explode.
		return;
	}

	real range = BOMB_PROJECTILE_IMPACT_AREA_RADIUS;
	real damage = BOMB_PROJECTILE_IMPACT_AREA_DAMAGE;

	PARTICLE(
		.angle = rng_angle(),
		.color = &p->color,
		.draw_rule = pdraw_timeout_scalefade(0, 3 * range / ARGS.impact_sprite->w, 1, 0),
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
		.layer = LAYER_BOSS + 2,
		.pos = p->pos,
		.sprite_ptr = ARGS.impact_sprite,
		.timeout = 20,
	);

	stage_clear_hazards_at(p->pos, range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
	ent_area_damage(p->pos, range, &(DamageInfo) { damage, DMG_PLAYER_BOMB }, NULL, NULL);
}

TASK(reimu_dream_gap_bomb_projectile, {
	cmplx pos;
	cmplx vel;
	const Color *color;
	Sprite *sprite;
	Sprite *impact_sprite;
}) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.angle = rng_angle(),
		.color = ARGS.color,
		.damage = 75,
		.damage_type = DMG_PLAYER_BOMB,
		.draw_rule = reimu_dream_gap_bomb_projectile_draw,
		.flags = PFLAG_MANUALANGLE,
		.move = move_linear(ARGS.vel),
		.pos = ARGS.pos,
		.size = 32 * (1 + I),
		.sprite_ptr = ARGS.sprite,
		.type = PROJ_PLAYER,
		.max_viewport_dist = BOMB_PROJECTILE_PERIODIC_AREA_RADIUS
	));

	INVOKE_TASK_WHEN(&p->events.killed, reimu_dream_gap_bomb_projectile_impact,
		ENT_BOX(p),
		ARGS.impact_sprite
	);

	for(;;) {
		WAIT(BOMB_PROJECTILE_PERIODIC_DELAY);
		real range = BOMB_PROJECTILE_PERIODIC_AREA_RADIUS;
		real damage = BOMB_PROJECTILE_PERIODIC_AREA_DAMAGE;
		stage_clear_hazards_at(p->pos, range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
		ent_area_damage(p->pos, range, &(DamageInfo) { damage, DMG_PLAYER_BOMB }, NULL, NULL);
	}
}

TASK(reimu_dream_bomb_noise) {
	for(;;WAIT(16)) {
		play_sfx("boon");
	}
}

TASK(reimu_dream_bomb_background, { ReimuBController *ctrl; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	CoEvent *draw_event = &stage_get_draw_events()->background_drawn;

	for(;;) {
		WAIT_EVENT_OR_DIE(draw_event);
		reimu_common_bomb_bg(ctrl->plr, ctrl->bomb_alpha);
	}
}

TASK(reimu_dream_bomb, { ReimuBController *ctrl; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	BoxedTask noise_task = cotask_box(INVOKE_SUBTASK(reimu_dream_bomb_noise));
	INVOKE_SUBTASK(reimu_dream_bomb_background, ctrl);

	Sprite *spr_proj = res_sprite("proj/glowball");
	Sprite *spr_impact = res_sprite("part/blast");

	YIELD;

	int t = 0;
	do {
		Color *pcolor = HSLA(t/30.0, 0.5, 0.5, 0.5);

		for(int i = 0; i < NUM_GAPS; ++i) {
			ReimuBGap *gap = ctrl->gaps.array + i;
			INVOKE_TASK(reimu_dream_gap_bomb_projectile,
				.pos = gap->pos + gap->parallel_axis * GAP_LENGTH * 0.25 * rng_sreal(),
				.vel = -20 * gap->orientation,
				.color = pcolor,
				.sprite = spr_proj,
				.impact_sprite = spr_impact
			);
		}

		stage_shake_view(4);
		t += WAIT(BOMB_PROJECTILE_FIRE_DELAY);
	} while(player_is_bomb_active(plr));

	CANCEL_TASK(noise_task);

	while(ctrl->bomb_alpha > 0) {
		// keep bg renderer alive
		YIELD;
	}
}

TASK(reimu_dream_bomb_handler, { ReimuBController *ctrl; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	BoxedTask bomb_task = { 0 };

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.bomb_used);
		CANCEL_TASK(bomb_task);
		play_sfx("bomb_marisa_a");
		bomb_task = cotask_box(INVOKE_SUBTASK(reimu_dream_bomb, ctrl));
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
		ReimuBGap *gap = ctrl->gaps.array + i;
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
	ReimuBController *ctrl = ENT_CAST(gap_renderer_ent, ReimuBController);

	float gaps[NUM_GAPS][2];
	float angles[NUM_GAPS];
	int links[NUM_GAPS];

	for(int i = 0; i < NUM_GAPS; ++i) {
		ReimuBGap *gap = ctrl->gaps.array + i;
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
		r_clear(BUFFER_COLOR, RGBA(0, 0, 0, 0), 1);
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
		ReimuBGap *gap = ctrl->gaps.array + i;
		yinyang.pos.as_cmplx = gap->pos + gap->parallel_axis * -0.5 * GAP_LENGTH;
		r_draw_sprite(&yinyang);
		yinyang.pos.as_cmplx = gap->pos + gap->parallel_axis * +0.5 * GAP_LENGTH;
		r_draw_sprite(&yinyang);
	}

	reimu_dream_draw_gap_lights(ctrl, global.frames, ctrl->bomb_alpha * ctrl->bomb_alpha);
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
		.flags = PFLAG_MANUALANGLE,
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
		.flags = PFLAG_MANUALANGLE,
	);
}

static void reimu_dream_bullet_warp(ReimuBController *ctrl, Projectile *p, int *warp_count) {
	if(*warp_count < 1) {
		return;
	}

	real p_long_side = fmax(creal(p->size), cimag(p->size));
	cmplx half = 0.25 * (1 + I);
	Rect p_bbox = { p->pos - p_long_side * half, p->pos + p_long_side * half };

	for(int i = 0; i < NUM_GAPS; ++i) {
		ReimuBGap *gap = ctrl->gaps.array + i;
		real a = carg(-gap->orientation/p->move.velocity);

		if(fabs(a) < M_TAU/3) {
			continue;
		}

		Rect gap_bbox, overlap = { };
		cmplx gap_size = (GAP_LENGTH + I * GAP_WIDTH) * gap->parallel_axis;
		cmplx p0 = gap->pos - gap_size * 0.5;
		cmplx p1 = gap->pos + gap_size * 0.5;
		gap_bbox.top_left = fmin(creal(p0), creal(p1)) + I * fmin(cimag(p0), cimag(p1));
		gap_bbox.bottom_right = fmax(creal(p0), creal(p1)) + I * fmax(cimag(p0), cimag(p1));

		if(rect_rect_intersection(p_bbox, gap_bbox, true, false, &overlap)) {
			cmplx o = (overlap.top_left + overlap.bottom_right) / 2;
			real fract;

			if(creal(gap_size) > cimag(gap_size)) {
				fract = 1 - creal(o - gap_bbox.top_left) / creal(gap_size);
			} else {
				fract = 1 - cimag(o - gap_bbox.top_left) / cimag(gap_size);
			}

			ReimuBGap *ngap = gap->link;
			o = ngap->pos + ngap->parallel_axis * GAP_LENGTH * (1 - fract - 0.5);

			reimu_dream_spawn_warp_effect(gap->pos + gap->parallel_axis * GAP_LENGTH * (fract - 0.5), false);
			reimu_dream_spawn_warp_effect(o, true);

			cmplx new_vel = cabs(p->move.velocity) * -ngap->orientation;

			p->move.acceleration *= cnormalize(new_vel/p->move.velocity);
			p->move.velocity = new_vel;
			p->pos = o - p->move.velocity;

			--*warp_count;
		}
	}
}

static void reimu_dream_draw_slave(EntityInterface *ent) {
	ReimuBSlave *slave = ENT_CAST(ent, ReimuBSlave);
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = slave->sprite,
		.shader_ptr = slave->shader,
		.pos.as_cmplx = slave->pos,
		.rotation.angle = global.frames * -6 * DEG2RAD,
		.color = RGB(0.95, 0.75, 1.0),
		.scale.both = 0.5,
	});
}

TASK(reimu_dream_needle, {
	ReimuBController *ctrl;
	cmplx pos;
	cmplx vel;
	ShaderProgram *shader;
}) {
	ReimuBController *ctrl = ARGS.ctrl;
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_needle2,
		.pos = ARGS.pos,
		.color = RGBA_MUL_ALPHA(1, 1, 1, 0.35),
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage = SHOT_SLAVE_DMG,
		.shader_ptr = ARGS.shader,
	));

	Color *trail_color = color_mul_scalar(RGBA(0.75, 0.5, 1, 0), 0.15);
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
	BoxedReimuBSlave slave;
	cmplx vel;
}) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	ReimuBSlave *slave = TASK_BIND(ARGS.slave);
	cmplx vel = ARGS.vel;
	ShaderProgram *shader = res_shader("sprite_particle");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		WAIT(SHOT_SLAVE_PRE_DELAY);
		INVOKE_TASK(reimu_dream_needle,
			.ctrl = ctrl,
			.pos = slave->pos,
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
	ReimuBSlave *slave = TASK_HOST_ENT(ReimuBSlave);
	slave->ent.draw_layer = LAYER_PLAYER_SLAVE;
	slave->ent.draw_func = reimu_dream_draw_slave;
	slave->sprite = res_sprite("yinyang"),
	slave->shader = res_shader("sprite_yinyang"),
	slave->pos = plr->pos;
	slave->alive = 1;

	INVOKE_SUBTASK_WHEN(&ctrl->events.slaves_expired, common_set_bitflags,
		.pflags = &slave->alive,
		.mask = 0, .set = 0
	);

	real angle = ARGS.angle_offset + M_PI/2;
	cmplx offset = ARGS.offset;

	INVOKE_SUBTASK(reimu_dream_slave_shot,
		.ctrl = ctrl,
		.slave = ENT_BOX(slave),
		.vel = 20 * ARGS.shot_dir
	);

	do {
		cmplx ofs = (plr->inputflags & INFLAG_FOCUS) ? cswap(offset) : offset;
		ofs = cwmul(ofs, cdir(angle));
		capproach_asymptotic_p(&slave->pos, plr->pos + ofs, 0.5, 1e-5);
		angle += 0.1;
		YIELD;
	} while(slave->alive);
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
		ReimuBGap *gap = ctrl->gaps.array + i;
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
			ReimuBGap *gap = ctrl->gaps.array + i;
			cmplx target_pos = reimu_dream_gap_pos(vp, ref_pos, gap->orientation, gap->parallel_axis, ofs_factors);
			capproach_asymptotic_p(&gap->pos, target_pos, pos_approach_rate, 1e-5);
		}
	}
}

TASK(reimu_dream_ofuda, { ReimuBController *ctrl; cmplx pos; cmplx vel; ShaderProgram *shader; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Projectile *ofuda = TASK_BIND(PROJECTILE(
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
	ShaderProgram *ofuda_shader = res_shader("sprite_particle");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		play_sfx_loop("generic_shot");

		for(int i = -1; i < 2; i += 2) {
			cmplx shot_dir = i * ((plr->inputflags & INFLAG_FOCUS) ? 1 : I);
			cmplx spread_dir = shot_dir * I;

			for(int j = -1; j < 2; j += 2) {
				INVOKE_TASK(reimu_dream_ofuda,
					.ctrl = ctrl,
					.pos = plr->pos + 10 * j * spread_dir,
					.vel = 20.0 * shot_dir,
					.shader = ofuda_shader
   				);
			}
		}

		WAIT(SHOT_FORWARD_DELAY);
	}
}

TASK(reimu_dream_power_handler, { ReimuBController *ctrl; }) {
	ReimuBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	int old_power = player_get_effective_power(plr) / 100;

	reimu_dream_respawn_slaves(ctrl, old_power * 2);

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.effective_power_changed);
		int new_power = player_get_effective_power(plr) / 100;
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
			stage_shake_view(1);
			approach_p(&ctrl->bomb_alpha, 1.0, 0.1);
		} else {
			approach_p(&ctrl->bomb_alpha, 0.0, 0.025);
		}

		YIELD;
	}
}

TASK(reimu_dream_controller, { BoxedPlayer plr; }) {
	ReimuBController *ctrl = TASK_HOST_ENT(ReimuBController);
	ctrl->plr = TASK_BIND(ARGS.plr);
	TASK_HOST_EVENTS(ctrl->events);

	ctrl->yinyang_shader = res_shader("sprite_yinyang");
	ctrl->yinyang_sprite = res_sprite("yinyang");

	ctrl->ent.draw_func = reimu_dream_draw_gaps;
	ctrl->ent.draw_layer = LAYER_PLAYER_FOCUS;

	ctrl->gaps.left.link = &ctrl->gaps.top;
	ctrl->gaps.top.link = &ctrl->gaps.left;
	ctrl->gaps.right.link = &ctrl->gaps.bottom;
	ctrl->gaps.bottom.link = &ctrl->gaps.right;

	ctrl->gaps.left.orientation = -1;
	ctrl->gaps.right.orientation = 1;
	ctrl->gaps.top.orientation = -I;
	ctrl->gaps.bottom.orientation = I;

	ctrl->gaps.left.parallel_axis = ctrl->gaps.right.parallel_axis = I;
	ctrl->gaps.top.parallel_axis = ctrl->gaps.bottom.parallel_axis = 1;

	INVOKE_SUBTASK(reimu_dream_controller_tick, ctrl);
	INVOKE_SUBTASK(reimu_dream_process_gaps, ctrl);
	INVOKE_SUBTASK(reimu_dream_shot_forward, ctrl);
	INVOKE_SUBTASK(reimu_dream_power_handler, ctrl);
	INVOKE_SUBTASK(reimu_dream_bomb_handler, ctrl);

	STALL;
}

static void reimu_dream_init(Player *plr) {
	reimu_common_bomb_buffer_init();
	INVOKE_TASK(reimu_dream_controller, ENT_BOX(plr));
}

static void reimu_dream_preload(ResourceGroup *rg) {
	const int flags = RESF_DEFAULT;

	res_group_preload(rg, RES_SPRITE, flags,
		"yinyang",
		"proj/ofuda",
		"proj/needle2",
		"proj/glowball",
		"part/myon",
		"part/stardust",
	NULL);

	res_group_preload(rg, RES_TEXTURE, flags,
		"runes",
		"gaplight",
	NULL);

	res_group_preload(rg, RES_SHADER_PROGRAM, flags,
		"sprite_yinyang",
		"reimu_gap",
		"reimu_gap_light",
		"reimu_bomb_bg",
	NULL);

	res_group_preload(rg, RES_SFX, flags | RESF_OPTIONAL,
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
		.init = reimu_dream_init,
		.preload = reimu_dream_preload,
	},
};
