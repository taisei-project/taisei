/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "marisa.h"

#include "audio/audio.h"
#include "common_tasks.h"
#include "dialog/marisa.h"
#include "global.h"
#include "intl/intl.h"
#include "plrmodes.h"
#include "renderer/api.h"
#include "stagedraw.h"
#include "util/graphics.h"

#define SHOT_FORWARD_DAMAGE 60
#define SHOT_FORWARD_DELAY 6
#define SHOT_LASER_DAMAGE 13

#define HAKKERO_RETRACT_TIME 4

typedef struct MarisaALaser MarisaALaser;

DEFINE_ENTITY_TYPE(MarisaASlave, {
	Sprite *sprite;
	ShaderProgram *shader;
	cmplx pos;
	real lean;
	real shot_angle;
	real flare_alpha;
	uint alive;
});

DEFINE_ENTITY_TYPE(MarisaAMasterSpark, {
	cmplx pos;
	cmplx dir;
	real alpha;
});

struct MarisaALaser {
	LIST_INTERFACE(MarisaALaser);
	cmplx pos;
	struct {
		cmplx first;
		cmplx last;
	} trace_hit;
	real alpha;
};

DEFINE_ENTITY_TYPE(MarisaAController, {
	Player *plr;
	LIST_ANCHOR(MarisaALaser) lasers;

	COEVENTS_ARRAY(
		slaves_expired
	) events;
});

static void trace_laser(MarisaALaser *laser, cmplx vel, real damage) {
	ProjCollisionResult col;
	ProjectileList lproj = { .first = NULL };

	PROJECTILE(
		.dest = &lproj,
		.pos = laser->pos,
		.size = 28*(1+I),
		.type = PROJ_PLAYER,
		.damage = damage,
		.flags = PFLAG_NOSPAWNFLARE,
		.move = move_linear(vel),
	);

	bool first_found = false;
	int timeofs = 0;
	int col_types = PCOL_ENTITY;

	struct enemy_col {
		Enemy *enemy;
		EnemyFlag original_flags;
	} enemy_collisions[64] = {};  // 64 collisions ought to be enough for everyone

	int num_enemy_collissions = 0;

	while(lproj.first) {
		timeofs = trace_projectile(lproj.first, &col, col_types | PCOL_VOID, timeofs);
		struct enemy_col *c = NULL;

		if(col.type & col_types) {
			RNG_ARRAY(R, 3);
			PARTICLE(
				.sprite = "flare",
				.pos = col.location,
				.move = move_linear(vrng_range(R[1], 2, 8) * vrng_dir(R[2])),
				.timeout = vrng_range(R[0], 3, 8),
				.draw_rule = pdraw_timeout_scale(2, 0.01),
				.flags = PFLAG_NOREFLECT,
				.layer = LAYER_PARTICLE_HIGH,
			);

			col.fatal = false;

			if(col.type == PCOL_ENTITY && col.entity->type == ENT_TYPE_ID(Enemy)) {
				assert(num_enemy_collissions < ARRAY_SIZE(enemy_collisions));
				if(num_enemy_collissions < ARRAY_SIZE(enemy_collisions)) {
					Enemy *e = ENT_CAST(col.entity, Enemy);
					c = enemy_collisions + num_enemy_collissions++;
					c->enemy = e;
					if(e->flags & EFLAG_IMPENETRABLE) {
						col.fatal = true;
					}
				}
			} else {
				col_types &= ~col.type;
			}
		}

		apply_projectile_collision(&lproj, lproj.first, &col);

		if(c) {
			c->original_flags = (ENT_CAST(col.entity, Enemy))->flags;
			(ENT_CAST(col.entity, Enemy))->flags |= EFLAG_NO_HIT;
		}

		if(!first_found) {
			if(lproj.first) {
				lproj.first->damage *= 0.5;
			}

			laser->trace_hit.first = col.location;
			first_found = true;
		}
	}

	assume(num_enemy_collissions < ARRAY_SIZE(enemy_collisions));

	for(int i = 0; i < num_enemy_collissions; ++i) {
		enemy_collisions[i].enemy->flags = enemy_collisions[i].original_flags;
	}

	laser->trace_hit.last = col.location;
}

static float set_alpha(Uniform *u_alpha, float a) {
	if(a) {
		r_uniform_float(u_alpha, a);
	}

	return a;
}

static float set_alpha_dimmed(Uniform *u_alpha, float a) {
	return set_alpha(u_alpha, a * a * 0.5);
}

static void marisa_laser_draw_slave(EntityInterface *ent) {
	MarisaASlave *slave = ENT_CAST(ent, MarisaASlave);

	ShaderCustomParams shader_params;
	shader_params.color = *RGBA(0.2, 0.4, 0.5, slave->flare_alpha * 0.75);
	float t = global.frames;

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = slave->sprite,
		.shader_ptr = slave->shader,
		.pos.as_cmplx = slave->pos,
		.rotation.angle = t * 0.05f,
		.color = color_lerp(RGB(0.2, 0.4, 0.5), RGB(1.0, 1.0, 1.0), 0.25 * powf(psinf(t / 6.0f), 2.0f) * slave->flare_alpha),
		.shader_params = &shader_params,
	});
}

static void draw_laser_beam(cmplx src, cmplx dst, real size, real step, real t, Texture *tex, Uniform *u_length) {
	cmplx dir = dst - src;
	cmplx center = (src + dst) * 0.5;

	r_mat_mv_push();

	r_mat_mv_translate(re(center), im(center), 0);
	r_mat_mv_rotate(carg(dir), 0, 0, 1);
	r_mat_mv_scale(cabs(dir), size, 1);

	r_mat_tex_push_identity();
	r_mat_tex_translate(-im(src) / step + t, 0, 0);
	r_mat_tex_scale(cabs(dir) / step, 1, 1);

	r_uniform_sampler("tex", tex);
	r_uniform_float(u_length, cabs(dir) / step);
	r_draw_quad();

	r_mat_tex_pop();
	r_mat_mv_pop();
}

static void marisa_laser_draw_lasers(EntityInterface *ent) {
	MarisaAController *ctrl = ENT_CAST(ent, MarisaAController);
	real t = global.frames;

	ShaderProgram *shader = res_shader("marisa_laser");
	Uniform *u_clr0 = r_shader_uniform(shader, "color0");
	Uniform *u_clr1 = r_shader_uniform(shader, "color1");
	Uniform *u_clr_phase = r_shader_uniform(shader, "color_phase");
	Uniform *u_clr_freq = r_shader_uniform(shader, "color_freq");
	Uniform *u_alpha = r_shader_uniform(shader, "alphamod");
	Uniform *u_length = r_shader_uniform(shader, "laser_length");
	Texture *tex0 = res_texture("part/marisa_laser0");
	Texture *tex1 = res_texture("part/marisa_laser1");
	FBPair *fbp_aux = stage_get_fbpair(FBPAIR_FG_AUX);
	Framebuffer *target_fb = r_framebuffer_current();

	r_shader_ptr(shader);
	r_uniform_vec4(u_clr0,       0.5, 0.5, 0.5, 0.0);
	r_uniform_vec4(u_clr1,       0.8, 0.8, 0.8, 0.0);
	r_uniform_float(u_clr_phase, -1.5 * t/M_PI);
	r_uniform_float(u_clr_freq,  10.0);
	r_framebuffer(fbp_aux->back);
	r_clear(BUFFER_COLOR, RGBA(0, 0, 0, 0), 1);
	r_color4(1, 1, 1, 1);

	r_blend(r_blend_compose(
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_ONE, BLENDOP_MAX,
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_ONE, BLENDOP_MAX
	));

	for(MarisaALaser *laser = ctrl->lasers.first; laser; laser = laser->next) {
		if(set_alpha(u_alpha, laser->alpha)) {
			draw_laser_beam(laser->pos, laser->trace_hit.last, 32, 128, -0.02 * t, tex1, u_length);
		}
	}

	r_blend(BLEND_PREMUL_ALPHA);
	fbpair_swap(fbp_aux);

	stage_draw_begin_noshake();

	r_framebuffer(fbp_aux->back);
	r_clear(BUFFER_COLOR, RGBA(0, 0, 0, 0), 1);
	r_shader("max_to_alpha");
	draw_framebuffer_tex(fbp_aux->front, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(fbp_aux);

	r_framebuffer(target_fb);
	r_shader_standard();
	r_color4(1, 1, 1, 1);
	draw_framebuffer_tex(fbp_aux->front, VIEWPORT_W, VIEWPORT_H);
	r_shader_ptr(shader);

	stage_draw_end_noshake();

	r_uniform_vec4(u_clr0, 0.5, 0.0, 0.0, 0.0);
	r_uniform_vec4(u_clr1, 1.0, 0.0, 0.0, 0.0);

	for(MarisaALaser *laser = ctrl->lasers.first; laser; laser = laser->next) {
		if(set_alpha_dimmed(u_alpha, laser->alpha)) {
			draw_laser_beam(laser->pos, laser->trace_hit.first, 40, 128, t * -0.12, tex0, u_length);
		}
	}

	r_uniform_vec4(u_clr0, 2.0, 1.0, 2.0, 0.0);
	r_uniform_vec4(u_clr1, 0.1, 0.1, 1.0, 0.0);

	for(MarisaALaser *laser = ctrl->lasers.first; laser; laser = laser->next) {
		if(set_alpha_dimmed(u_alpha, laser->alpha)) {
			draw_laser_beam(laser->pos, laser->trace_hit.first, 42, 200, t * -0.03, tex0, u_length);
		}
	}

	SpriteParams sp = {
		.sprite_ptr = res_sprite("part/smoothdot"),
		.shader_ptr = res_shader("sprite_default"),
	};

	for(MarisaALaser *laser = ctrl->lasers.first; laser; laser = laser->next) {
		float a = laser->alpha * 0.8f;
		cmplx spread;

		sp.pos.as_cmplx = laser->trace_hit.first;

		spread = rng_dir(); spread *= rng_range(0, 3);
		sp.pos.as_cmplx += spread;
		sp.color = color_mul_scalar(RGBA(0.8, 0.3, 0.5, 0), a);
		r_draw_sprite(&sp);

		spread = rng_dir(); spread *= rng_range(0, 3);
		sp.pos.as_cmplx += spread;
		sp.color = color_mul_scalar(RGBA(0.2, 0.7, 0.5, 0), a);
		r_draw_sprite(&sp);
	}
}

static void marisa_laser_flash_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	float o = 1 - t / p->timeout;
	color_mul_scalar(&spbuf.color, o);
	spbuf.color.r *= o;
	r_draw_sprite(&sp);
}

static void marisa_laser_show_laser(MarisaAController *ctrl, MarisaALaser *laser) {
	alist_append(&ctrl->lasers, laser);
}

static void marisa_laser_hide_laser(MarisaAController *ctrl, MarisaALaser *laser) {
	alist_unlink(&ctrl->lasers, laser);
}

TASK(marisa_laser_fader, {
	MarisaAController *ctrl;
	const MarisaALaser *ref_laser;
}) {
	MarisaAController *ctrl = ARGS.ctrl;
	MarisaALaser fader = *ARGS.ref_laser;

	marisa_laser_show_laser(ctrl, &fader);

	while(fader.alpha > 0) {
		YIELD;
		approach_p(&fader.alpha, 0, 0.1);
	}

	marisa_laser_hide_laser(ctrl, &fader);
}

static void marisa_laser_fade_laser(MarisaAController *ctrl, MarisaALaser *laser) {
	marisa_laser_hide_laser(ctrl, laser);
	INVOKE_TASK(marisa_laser_fader, ctrl, laser);
}

TASK(marisa_laser_slave_shot_cleanup, {
	MarisaAController *ctrl;
	MarisaALaser **active_laser;
}) {
	MarisaALaser *active_laser = *ARGS.active_laser;
	if(active_laser) {
		marisa_laser_fade_laser(ARGS.ctrl, active_laser);
	}
}

TASK(marisa_laser_slave_shot, {
	MarisaAController *ctrl;
	BoxedMarisaASlave slave;
}) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	MarisaASlave *slave = TASK_BIND(ARGS.slave);

	Animation *fire_anim = res_anim("fire");
	AniSequence *fire_anim_seq = get_ani_sequence(fire_anim, "main");

	MarisaALaser *active_laser = NULL;
	INVOKE_TASK_AFTER(&TASK_EVENTS(THIS_TASK)->finished, marisa_laser_slave_shot_cleanup, ctrl, &active_laser);

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		assert(player_should_shoot(plr));

		// We started shooting - register a laser for rendering

		MarisaALaser laser = {};
		marisa_laser_show_laser(ctrl, &laser);
		active_laser = &laser;

		while(player_should_shoot(plr)) {
			real angle = slave->shot_angle * (1.0 + 0.7 * psin(global.frames/15.0));

			laser.pos = slave->pos;
			approach_p(&laser.alpha, 1.0, 0.2);

			cmplx dir = -cdir(angle + slave->lean + M_PI/2);
			trace_laser(&laser, 5 * dir, SHOT_LASER_DAMAGE);

			Sprite *spr = animation_get_frame(fire_anim, fire_anim_seq, global.frames);

			PARTICLE(
				.sprite_ptr = spr,
				.size = 1+I,
				.pos = laser.pos + dir * 10,
				.color = color_mul_scalar(RGBA(2, 0.2, 0.5, 0), 0.2),
				.draw_rule = marisa_laser_flash_draw,
				.move = move_linear(dir),
				.timeout = 8,
				.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
				.scale = 0.4,
			);

			YIELD;
		}

		// We stopped shooting - remove the laser, spawn fader

		marisa_laser_fade_laser(ctrl, &laser);
		active_laser = NULL;
	}
}

TASK(marisa_laser_slave, {
	MarisaAController *ctrl;
	cmplx unfocused_offset;
	cmplx focused_offset;
	real shot_angle;
}) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	MarisaASlave *slave = TASK_HOST_ENT(MarisaASlave);
	slave->alive = 1;
	slave->ent.draw_func = marisa_laser_draw_slave;
	slave->ent.draw_layer = LAYER_PLAYER_SLAVE;
	slave->pos = plr->pos;
	slave->shader = res_shader("sprite_hakkero");
	slave->sprite = res_sprite("hakkero");

	INVOKE_SUBTASK_WHEN(&ctrl->events.slaves_expired, common_set_bitflags,
		.pflags = &slave->alive,
		.mask = 0,
		.set = 0
	);

	BoxedTask shot_task = cotask_box(INVOKE_SUBTASK(marisa_laser_slave_shot,
		.ctrl = ctrl,
		.slave = ENT_BOX(slave)
	));

	/*                     unfocused              focused              */
	cmplx offsets[]    = { ARGS.unfocused_offset, ARGS.focused_offset, };
	real shot_angles[] = { ARGS.shot_angle,       0                    };

	real formation_switch_rate = 0.3;
	real lean_rate = 0.1;
	real follow_rate = 0.5;
	real lean_strength = 0.01;
	real epsilon = 1e-5;

	cmplx offset = 0;
	cmplx prev_pos = slave->pos;

	while(slave->alive) {
		bool focused = plr->inputflags & INFLAG_FOCUS;

		approach_asymptotic_p(&slave->shot_angle, shot_angles[focused], formation_switch_rate, epsilon);
		capproach_asymptotic_p(&offset, offsets[focused], formation_switch_rate, epsilon);

		capproach_asymptotic_p(&slave->pos, plr->pos + offset, follow_rate, epsilon);
		approach_asymptotic_p(&slave->lean, -lean_strength * re(slave->pos - prev_pos), lean_rate, epsilon);
		prev_pos = slave->pos;

		if(player_should_shoot(plr)) {
			approach_asymptotic_p(&slave->flare_alpha, 1.0, 0.2, epsilon);
		} else {
			approach_asymptotic_p(&slave->flare_alpha, 0.0, 0.08, epsilon);
		}

		YIELD;
	}

	CANCEL_TASK(shot_task);
	plrutil_slave_retract(ENT_BOX(plr), &slave->pos, HAKKERO_RETRACT_TIME);
}

static real marisa_laser_masterspark_width(real progress) {
	real w = 1;

	if(progress < 1./6) {
		w = progress * 6;
		w = pow(w, 1.0/3.0);
	}

	if(progress > 4./5) {
		w = 1 - progress * 5 + 4;
		w = pow(w, 5);
	}

	return w;
}

static void marisa_laser_draw_masterspark(EntityInterface *ent) {
	MarisaAMasterSpark *ms = ENT_CAST(ent, MarisaAMasterSpark);
	marisa_common_masterspark_draw(1, &(MarisaBeamInfo) {
		ms->pos,
		800 + I * VIEWPORT_H * 1.25,
		carg(ms->dir),
		global.frames
	}, ms->alpha);
}

static void marisa_laser_masterspark_damage(MarisaAMasterSpark *ms) {
	// lazy inefficient approximation of the beam parabola

	float r = 96 * ms->alpha;
	float growth = 0.25;
	cmplx v = ms->dir * cdir(M_PI * -0.5);
	cmplx p = ms->pos + v * r;

	Rect vp_rect, seg_rect;
	vp_rect.top_left = 0;
	vp_rect.bottom_right = CMPLX(VIEWPORT_W, VIEWPORT_H);

	int iter = 0;
	int maxiter = 10;

	do {
		stage_clear_hazards_at(p, r, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);
		ent_area_damage(p, r, &(DamageInfo) { 80, DMG_PLAYER_BOMB }, NULL, NULL);

		p += v * r;
		r *= 1 + growth;
		growth *= 0.75;

		cmplx o = (1 + I);
		seg_rect.top_left = p - o * r;
		seg_rect.bottom_right = p + o * r;

		++iter;
	} while(rect_rect_intersect(seg_rect, vp_rect, true, true) && iter < maxiter);
	// log_debug("%i", iter);
}

TASK(marisa_laser_bomb_background, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	CoEvent *draw_event = &stage_get_draw_events()->background_drawn;

	for(;;) {
		WAIT_EVENT_OR_DIE(draw_event);
		float t = player_get_bomb_progress(plr);
		float fade = 1;

		if(t < 1.0f / 6.0f) {
			fade = t * 6.0f;
		}

		if(t > 3.0f / 4.0f) {
			fade = 1.0f - t * 4.0f + 3.0f;
		}

		if(fade <= 0.0f) {
			break;
		}

		r_state_push();
		r_color4(0.8f * fade, 0.8f * fade, 0.8f * fade, 0.8f * fade);
		fill_viewport(sinf(t * 0.3f), t * 3.0f * (1.0f + t * 3.0f), 1.0f, "marisa_bombbg");
		r_state_pop();
	};
}

TASK(marisa_laser_bomb_masterspark, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	MarisaAMasterSpark *ms = TASK_HOST_ENT(MarisaAMasterSpark);
	ms->dir = 1;
	ms->ent.draw_func = marisa_laser_draw_masterspark;
	ms->ent.draw_layer = LAYER_PLAYER_FOCUS - 1;

	int t = 0;

	Sprite *star_spr = res_sprite("part/maristar_orbit");
	Sprite *smoke_spr = res_sprite("part/smoke");

	BoxedTask bg_task = cotask_box(INVOKE_SUBTASK(marisa_laser_bomb_background, ctrl));

	YIELD;

	do {
		real bomb_progress = player_get_bomb_progress(plr);
		ms->alpha = marisa_laser_masterspark_width(bomb_progress);
		ms->dir *= cdir(0.005 * (re(plr->velocity) + 2 * rng_sreal()));
		ms->pos = plr->pos - 30 * I;

		marisa_laser_masterspark_damage(ms);

		if(bomb_progress >= 3.0/4.0) {
			stage_shake_view(8 * (1 - bomb_progress * 4 + 3));
			goto skip_particles;
		}

		stage_shake_view(8);

		uint pflags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE;

		if(t % 4 == 0) {
			pflags |= PFLAG_REQUIREDPARTICLE;
		}

		cmplx dir = -cdir(1.5 * sin(t * M_PI * 1.12)) * I;
		Color *c = HSLA(-bomb_progress * 5.321, 1, 0.5, rng_range(0, 0.5));
		cmplx pos = plr->pos + 40 * dir;

		for(int i = 0; i < 2; ++i) {
			cmplx v = 10 * (dir - I);
			PARTICLE(
				.angle = rng_angle(),
				.angle_delta = 0.1,
				.color = c,
				.draw_rule = pdraw_timeout_scalefade(0, 5, 1, 0),
				.flags = pflags,
				.move = move_accelerated(v, 0.1 * cnormalize(v)),
				.pos = pos,
				.sprite_ptr = star_spr,
				.timeout = 50,
			);
			dir = -conj(dir);
		}

		PARTICLE(
			.sprite_ptr = smoke_spr,
			.pos = plr->pos - 40*I,
			.color = HSLA(2 * bomb_progress, 1, 2, 0),
			.timeout = 50,
			.move = move_linear(7 * (I - dir)),
			.angle = rng_angle(),
			.draw_rule = pdraw_timeout_scalefade(0, 7, 1, 0),
			.flags = pflags,
		);

skip_particles:
		++t;
		YIELD;
	} while(player_is_bomb_active(plr));

	// should have faded out by now, but just in case…
	CANCEL_TASK(bg_task);

	while(ms->alpha > 0) {
		approach_p(&ms->alpha, 0, 0.2);
		YIELD;
	}
}

TASK(marisa_laser_bomb_handler, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	BoxedTask bomb_task = {};

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.bomb_used);
		CANCEL_TASK(bomb_task);
		play_sfx("bomb_marisa_a");
		bomb_task = cotask_box(INVOKE_SUBTASK(marisa_laser_bomb_masterspark, ctrl));
	}
}

static void marisa_laser_respawn_slaves(MarisaAController *ctrl, int power_rank) {
	coevent_signal(&ctrl->events.slaves_expired);

	if(power_rank == 1) {
		INVOKE_TASK(marisa_laser_slave, ctrl, -40.0*I, -40.0*I, 0);
	}

	if(power_rank >= 2) {
		INVOKE_TASK(marisa_laser_slave, ctrl,  25-5.0*I,  9-40.0*I,  M_PI/30);
		INVOKE_TASK(marisa_laser_slave, ctrl, -25-5.0*I, -9-40.0*I, -M_PI/30);
	}

	if(power_rank == 3) {
		INVOKE_TASK(marisa_laser_slave, ctrl, -30.0*I, -55.0*I, 0);
	}

	if(power_rank >= 4) {
		INVOKE_TASK(marisa_laser_slave, ctrl,  17-30.0*I,  18-55.0*I,  M_PI/60);
		INVOKE_TASK(marisa_laser_slave, ctrl, -17-30.0*I, -18-55.0*I, -M_PI/60);
	}
}

TASK(marisa_laser_power_handler, { MarisaAController *ctrl; }) {
	MarisaAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	int old_power = player_get_effective_power(plr) / 100;

	marisa_laser_respawn_slaves(ctrl, old_power);

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.effective_power_changed);
		int new_power = player_get_effective_power(plr) / 100;
		if(old_power != new_power) {
			marisa_laser_respawn_slaves(ctrl, new_power);
			old_power = new_power;
		}
	}
}

TASK(marisa_laser_controller, { BoxedPlayer plr; }) {
	MarisaAController *ctrl = TASK_HOST_ENT(MarisaAController);
	ctrl->plr = TASK_BIND(ARGS.plr);
	TASK_HOST_EVENTS(ctrl->events);

	ctrl->ent.draw_func = marisa_laser_draw_lasers;
	ctrl->ent.draw_layer = LAYER_PLAYER_SHOT_HIGH;

	INVOKE_SUBTASK(marisa_laser_power_handler, ctrl);
	INVOKE_SUBTASK(marisa_laser_bomb_handler, ctrl);
	INVOKE_SUBTASK(marisa_common_shot_forward, ARGS.plr, SHOT_FORWARD_DAMAGE, SHOT_FORWARD_DELAY);

	STALL;
}

static void marisa_laser_init(Player *plr) {
	INVOKE_TASK(marisa_laser_controller, ENT_BOX(plr));
}

static double marisa_laser_property(Player *plr, PlrProperty prop) {
	switch(prop) {
		case PLR_PROP_SPEED: {
			double s = marisa_common_property(plr, prop);

			if(player_is_bomb_active(plr)) {
				s /= 5.0;
			}

			return s;
		}

		default:
			return marisa_common_property(plr, prop);
	}
}

static void marisa_laser_preload(ResourceGroup *rg) {
	const int flags = RESF_DEFAULT;

	res_group_preload(rg, RES_SPRITE, flags,
		"proj/marisa",
		"part/maristar_orbit",
		"hakkero",
		"masterspark_ring",
	NULL);

	res_group_preload(rg, RES_TEXTURE, flags,
		"marisa_bombbg",
		"part/marisa_laser0",
		"part/marisa_laser1",
	NULL);

	res_group_preload(rg, RES_SHADER_PROGRAM, flags,
		"blur25",
		"blur5",
		"marisa_laser",
		"masterspark",
		"max_to_alpha",
		"sprite_hakkero",
	NULL);

	res_group_preload(rg, RES_ANIM, flags,
		"fire",
	NULL);

	res_group_preload(rg, RES_SFX, flags | RESF_OPTIONAL,
		"bomb_marisa_a",
	NULL);
}

PlayerMode plrmode_marisa_a = {
	.name = N_("Illusion Laser"),
	.description = N_("Magic missiles and lasers — simple and to the point. They’ve never let you down before, so why stop now?"),
	.spellcard_name = N_("Pure Love “Galactic Spark”"),
	.character = &character_marisa,
	.dialog = &dialog_tasks_marisa,
	.shot_mode = PLR_SHOT_MARISA_LASER,
	.procs = {
		.init = marisa_laser_init,
		.preload = marisa_laser_preload,
		.property = marisa_laser_property,
	},
};
