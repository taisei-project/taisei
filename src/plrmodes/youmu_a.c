/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "youmu.h"

#include "audio/audio.h"
#include "dialog/youmu.h"
#include "global.h"
#include "i18n/i18n.h"
#include "plrmodes.h"
#include "renderer/api.h"
#include "stagedraw.h"
#include "util/graphics.h"

#define SHOT_SELF_DELAY 6
#define SHOT_SELF_DAMAGE 60

#define SHOT_MYON_HALFDELAY 3
#define SHOT_MYON_DAMAGE 25

typedef struct YoumuAController YoumuAController;
typedef struct YoumuAMyon YoumuAMyon;

struct YoumuAMyon {
	struct {
		Sprite *trail;
		Sprite *smoke;
		Sprite *stardust;
	} sprites;

	cmplx pos;
	cmplx dir;
	real focus_factor;
};

struct YoumuAController {
	struct {
		Sprite *arc;
		Sprite *blast_huge_halo;
		Sprite *petal;
		Sprite *stain;
	} sprites;

	Player *plr;
	YoumuAMyon myon;
	YoumuBombBGData bomb_bg;
};

static Color *myon_color(Color *c, float f, float opacity, float alpha) {
	*c = *RGBA_MUL_ALPHA(0.8+0.2*f, 0.9-0.4*sqrt(f), 1.0-0.35*f*f, opacity);
	c->a *= alpha;
	return c;
}

static cmplx myon_tail_dir(YoumuAMyon *myon) {
	cmplx dir = myon->dir * cdir(0.1 * sin(global.frames * 0.05));
	real f = myon->focus_factor;
	return lerp(f * f, 1, 0.5) * dir;
}

static void myon_draw_trail_func(Projectile *p, int t, ProjDrawRuleArgs args) {
	float focus_factor = args[0].as_float[0];
	float opacity = args[0].as_float[1];

	float fadein = clamp(t / 10.0, 0, 1);
	float s = 1 - projectile_timeout_factor(p);

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);

	float a = opacity * fadein;
	myon_color(&spbuf.color, focus_factor, a * s * s, 0);
	sp.scale.as_cmplx *= fadein * (2 - s);

	r_draw_sprite(&sp);
}

static ProjDrawRule myon_draw_trail(float focus_factor, float opacity) {
	return (ProjDrawRule) {
		.func = myon_draw_trail_func,
		.args[0].as_float = { focus_factor, opacity },
	};
}

TASK(youmu_mirror_myon_trail, { YoumuAMyon *myon; cmplx pos; }) {
	YoumuAMyon *myon = ARGS.myon;

	Projectile *p = TASK_BIND(PARTICLE(
		.angle = rng_angle(),
		.draw_rule = pdraw_timeout_scale(2, 0.01),
		.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE | PFLAG_PLRSPECIALPARTICLE,
		.layer = LAYER_PARTICLE_LOW,
		.move = move_linear(0),
		.pos = ARGS.pos,
		.sprite_ptr = myon->sprites.trail,
		.timeout = 40,
	));

	p->angle_delta = 0.03 * (1 - 2 * (p->birthtime & 1));

	for(int t = 0;; ++t) {
		real f = myon->focus_factor;
		myon_color(&p->color, f, powf(1 - min(1, t / p->timeout), 2), 0.95f);
		p->pos += 0.05 * (myon->pos - p->pos) * cdir(sin((t - global.frames * 2) * 0.1) * M_PI/8);
		p->move.velocity = 3 * myon_tail_dir(myon);
		YIELD;
	}
}

static void myon_spawn_trail(YoumuAMyon *myon, int t) {
	cmplx pos = myon->pos + 3 * cdir(global.frames * 0.07);
	real f = myon->focus_factor;
	cmplx stardust_v = (2 + f) * myon_tail_dir(myon) * cdir(M_PI/16*sin(1.33*t));

	if(player_should_shoot(&global.plr)) {
		RNG_ARRAY(R, 7);

		PARTICLE(
			.angle = vrng_angle(R[2]),
			.angle_delta = 0.03 * (1 - 2 * (global.frames & 1)),
			.draw_rule = myon_draw_trail(f, 0.7),
			.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
			.pos = pos + vrng_range(R[0], 0, 10) * vrng_dir(R[1]),
			.scale = 0.2,
			.sprite_ptr = myon->sprites.smoke,
			.timeout = 30,
		);
	}

	INVOKE_TASK(youmu_mirror_myon_trail, myon, pos);

	RNG_ARRAY(R, 4);

	PARTICLE(
		.angle = vrng_angle(R[3]),
		.angle_delta = 0.03 * (1 - 2 * (global.frames & 1)),
		.draw_rule = myon_draw_trail(f, 0.5),
		.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
		.layer = LAYER_PARTICLE_LOW | 1,
		.move = move_linear(stardust_v),
		.pos = pos + vrng_range(R[0], 0, 5) * vrng_dir(R[1]),
		.scale = vrng_range(R[2], 0.2, 0.3),
		.sprite_ptr = myon->sprites.stardust,
		.timeout = 40,
	);
}

static void myon_draw_proj_trail(Projectile *p, int t, ProjDrawRuleArgs args) {
	float time_progress = projectile_timeout_factor(p);
	float s = 2 * time_progress;
	float a = min(1, s) * (1 - time_progress);

	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	color_mul_scalar(&spbuf.color, a);
	sp.scale.as_cmplx *= s;
	r_draw_sprite(&sp);
}

TASK(youmu_mirror_myon_proj, { cmplx pos; cmplx vel; real dmg; const Color *clr; ShaderProgram *shader; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.color = ARGS.clr,
		.damage = ARGS.dmg,
		.layer = LAYER_PLAYER_SHOT | 0x10,
		.move = move_linear(ARGS.vel),
		.pos = ARGS.pos,
		.proto = pp_youmu,
		.shader_ptr = ARGS.shader,
		.type = PROJ_PLAYER,
	));

	Color trail_color = p->color;
	trail_color.a = 0;
	color_mul_scalar(&trail_color, 0.075);

	Sprite *trail_sprite = res_sprite("part/boss_shadow");
	MoveParams trail_move = move_linear(ARGS.vel * 0.8);

	for(int t = 1;; ++t) {
		YIELD;

		// TODO: Optimize this. The trail can be made static, either pre-rendered
		// or drawn in the projectile's custom draw rule. The opacity change can
		// live in the draw rule as well. Then a separate task per shot is not needed.

		p->opacity = 1.0f - powf(1.0f - min(1.0f, t / 10.0f), 2.0f);

		PARTICLE(
			.sprite_ptr = trail_sprite,
			.pos = p->pos,
			.color = &trail_color,
			.draw_rule = myon_draw_proj_trail,
			.timeout = 10,
			.move = trail_move,
			.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
			.angle = p->angle,
			.scale = 0.6,
		);
	}
}

static inline void youmu_mirror_myon_proj(cmplx pos, cmplx vel, real dmg, const Color *clr, ShaderProgram *shader) {
	INVOKE_TASK(youmu_mirror_myon_proj, pos, vel, dmg, clr, shader);
}

static void myon_proj_color(Color *clr, real focus_factor) {
	Color intermediate = { 1.0, 1.0, 1.0, 1.0 };
	focus_factor = smooth(focus_factor);

	if(focus_factor < 0.5) {
		*clr = *RGB(0.4, 0.6, 0.6);
		color_lerp(
			clr,
			&intermediate,
			focus_factor * 2
		);
	} else {
		Color mc;
		*clr = intermediate;
		color_lerp(
			clr,
			myon_color(&mc, focus_factor, 1, 1),
			(focus_factor - 0.5) * 2
		);
	}
}

TASK(youmu_mirror_myon_shot, { YoumuAController *ctrl; }) {
	YoumuAController *ctrl = ARGS.ctrl;
	YoumuAMyon *myon = &ctrl->myon;
	Player *plr = ctrl->plr;
	ShaderProgram *shader = res_shader("sprite_youmu_myon_shot");

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);

		const real dmg_center = SHOT_MYON_DAMAGE;
		const real dmg_side = SHOT_MYON_DAMAGE;
		const real speed = -10;
		const int power_rank = player_get_effective_power(plr) / 100;

		real spread;
		cmplx forward;
		Color clr;

		{
			myon_proj_color(&clr, myon->focus_factor);
			forward = speed * myon->dir;
			spread = (psin(global.frames * 1.2) * 0.5 + 0.5) * 0.1;

			youmu_mirror_myon_proj(myon->pos, forward, dmg_center, &clr, shader);

			if(power_rank >= 2) {
				youmu_mirror_myon_proj(myon->pos, forward * cdir(+2 * spread), dmg_side, &clr, shader);
				youmu_mirror_myon_proj(myon->pos, forward * cdir(-2 * spread), dmg_side, &clr, shader);
			}

			if(power_rank >= 4) {
				youmu_mirror_myon_proj(myon->pos, forward * cdir(+4 * spread), dmg_side, &clr, shader);
				youmu_mirror_myon_proj(myon->pos, forward * cdir(-4 * spread), dmg_side, &clr, shader);
			}
		}

		WAIT(SHOT_MYON_HALFDELAY);

		{
			myon_proj_color(&clr, myon->focus_factor);
			forward = speed * myon->dir;
			spread = (psin(global.frames * 2.0) * 0.5 + 0.5) * 0.1;

			if(power_rank >= 1) {
				youmu_mirror_myon_proj(myon->pos, forward * cdir(+1 * spread), dmg_side, &clr, shader);
				youmu_mirror_myon_proj(myon->pos, forward * cdir(-1 * spread), dmg_side, &clr, shader);
			}

			if(power_rank >= 3) {
				youmu_mirror_myon_proj(myon->pos, forward * cdir(+3 * spread), dmg_side, &clr, shader);
				youmu_mirror_myon_proj(myon->pos, forward * cdir(-3 * spread), dmg_side, &clr, shader);
			}
		}

		WAIT(SHOT_MYON_HALFDELAY);
	}
}

static cmplx fit_velocity(cmplx smoothed, int entries, cmplx history[entries]) {
	cmplx bestfit = history[0];
	real mindst = cabs2(bestfit - smoothed);

	for(int i = 1; i < entries; ++i) {
		real d = cabs2(history[i] - smoothed);
		if(d < mindst) {
			bestfit = history[i];
			mindst = d;
		}
	}

	return bestfit;
}

TASK(youmu_mirror_myon, { YoumuAController *ctrl; }) {
	YoumuAController *ctrl = ARGS.ctrl;
	YoumuAMyon *myon = &ctrl->myon;
	Player *plr = ctrl->plr;

	myon->sprites.trail = res_sprite("part/myon");
	myon->sprites.smoke = res_sprite("part/smoke");
	myon->sprites.stardust = res_sprite("part/stardust");

	const real distance = 40;
	const real focus_rate = 1.0/30.0;

	real focus_factor = 0.0;
	bool fixed_position = false;

	/*
	 * Myon's target offset is based on a weighted average of the player's inputs
	 * in the last N frames. When the player stops moving, we snap the smoothed value
	 * to a real recent input. This allows keyboard players to aim it diagonally
	 * without frame-perfect inputs.
	 *
	 * This algorithm was pulled out of my ass without any theoretical justification,
	 * and it's probably dumb, but it works well enough so i don't care.
	 *
	 * Numpy code to generate the weights:
	 *
	 * 		w = np.array(range(N - 1, -1, -1))
	 * 		w = 0.5 * (np.tanh( 2*np.pi * (w/len(w) - 0.5) ) + 1)
	 * 		w /= np.sum(w)
	 */
	static const cmplx pvel_history_weights[12] = {
		0.1807944744847358,   0.1790414880132086,   0.1742275298832384,
		0.1618282865310685,   0.1345428375193760,   0.0908782920594558,
		0.0472137465995357,   0.0199282975878431,   0.0075290542356732,
		0.0027150961057031,   0.0009621096341759,   0.0003387873459861,
	};
	cmplx pvel_history[ARRAY_SIZE(pvel_history_weights)] = { };

	cmplx offset_dir = -I;

	myon->pos = plr->pos;
	myon->dir = I;

	INVOKE_SUBTASK(youmu_mirror_myon_shot, ctrl);

	for(int t = 0;; ++t) {
		for(int i = ARRAY_SIZE(pvel_history) - 1; i > 0; --i) {
			pvel_history[i] = pvel_history[i - 1];
		}

		pvel_history[0] = cnormalize(plr->uncapped_velocity);

		if(pvel_history[0]) {
			cmplx smoothed = 0;

			for(int i = 0; i < ARRAY_SIZE(pvel_history); ++i) {
				smoothed += pvel_history[i] * pvel_history_weights[1];
			}

			smoothed = cnormalize(smoothed);

			offset_dir = -smoothed;
		} else if(pvel_history[1]) {
			// just stopped - snap to a real recent input
			offset_dir = -fit_velocity(-offset_dir, ARRAY_SIZE(pvel_history), pvel_history);
		}

		real follow_factor = 0.15;

		if(plr->inputflags & INFLAG_FOCUS) {
			fixed_position = true;
			approach_p(&focus_factor, 1, focus_rate);
		} else {
			approach_p(&focus_factor, 0, focus_rate);

			if(fixed_position) {
				focus_factor = 0;
				offset_dir = -I;
				follow_factor *= 3;

				if(plr->inputflags & INFLAGS_MOVE) {
					fixed_position = false;
				}
			}
		}

		cmplx target = plr->pos + distance * cnormalize(offset_dir);
		real follow_speed = smoothmin(10, follow_factor * max(0, cabs(target - myon->pos)), 10);
		cmplx v = cnormalize(target - myon->pos) * follow_speed * (1 - focus_factor) * (1 - focus_factor);

		real s = sign(re(myon->pos) - re(plr->pos));
		if(!s) {
			s = sign(sin(t / 10.0));
		}

		real rot = clamp(0.005 * cabs(plr->pos - myon->pos) - M_PI/6, 0, M_PI/8);
		v *= cdir(rot * s);

		myon->pos += v;
		myon->focus_factor = focus_factor;

		if(!(plr->inputflags & INFLAG_SHOT) || !(plr->inputflags & INFLAG_FOCUS)) {
			if(plr->pos != myon->pos) {
				myon->dir = cnormalize(plr->pos - myon->pos);
			}
		}

		myon_spawn_trail(myon, t);
		YIELD;
	}
}

static void youmu_mirror_bomb_damage_callback(EntityInterface *victim, cmplx victim_origin, void *arg) {
	YoumuAController *ctrl = arg;

	cmplx ofs_dir = rng_dir();
	victim_origin += ofs_dir * rng_range(0, 15);

	RNG_ARRAY(R, 6);

	PARTICLE(
		.sprite_ptr = ctrl->sprites.blast_huge_halo,
		.pos = victim_origin,
		.color = RGBA(vrng_range(R[0], 0.6, 0.7), 0.8, vrng_range(R[1], 0.7, 0.775), vrng_range(R[2], 0, 0.5)),
		.timeout = 30,
		.draw_rule = pdraw_timeout_scalefade(0, 0.5, 1, 0),
		.layer = LAYER_PARTICLE_HIGH | 0x4,
		.angle = vrng_angle(R[3]),
		.flags = PFLAG_REQUIREDPARTICLE | PFLAG_MANUALANGLE,
	);

	if(global.frames & 2) {
		return;
	}

	real t = rng_real();
	RNG_NEXT(R);

	PARTICLE(
		.sprite_ptr = ctrl->sprites.petal,
		.pos = victim_origin,
		.draw_rule = pdraw_petal_random(),
		.color = RGBA(sin(5*t) * t, cos(5*t) * t, 0.5 * t, 0),
		.move = move_asymptotic_simple(
			vrng_sign(R[0]) * vrng_range(R[1], 3, 3 + 5 * t) * cdir(M_PI*8*t),
			5
		),
		.layer = LAYER_PARTICLE_PETAL,
	);
}

static void youmu_mirror_bomb_particles(YoumuAController *ctrl, cmplx pos, cmplx vel, int t, ProjFlags pflags) {
	if(t >= 240) {
		return;
	}

	PARTICLE(
		.color = RGBA(0.9, 0.8, 1.0, 0.0),
		.draw_rule = pdraw_timeout_fade(1, 0),
		.flags = pflags,
		.move = move_linear(2 * rng_dir()),
		.pos = pos,
		.sprite_ptr = ctrl->sprites.arc,
		.timeout = 30,
	);

	RNG_ARRAY(R, 2);

	PARTICLE(
		.angle = vrng_angle(R[0]),
		.color = RGBA(0.2, 0.1, 1.0, 0.0),
		.draw_rule = pdraw_timeout_scalefade(0, 3, 1, 0),
		.flags = pflags,
		.move = move_accelerated(
			-1 * vel * cdir(0.2 * vrng_real(R[1])) / 30,
			0.1 * vel * I * sin(t/4.0) / 30
		),
		.pos = pos,
		.sprite_ptr = ctrl->sprites.stain,
		.timeout = 50,
	);
}

static void youmu_mirror_draw_speed_trail(Projectile *p, int t, ProjDrawRuleArgs args) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	float nt = 1 - projectile_timeout_factor(p);
	float s = 1 + (1 - nt);
	sp.scale.as_cmplx *= s;
	sp.rotation.angle -= M_PI/2;
	spbuf.shader_params.vector[0] = -2 * nt * nt;
	spbuf.color.r *= nt * nt;
	spbuf.color.g *= nt * nt;
	spbuf.color.b *= nt;
	r_draw_sprite(&sp);
}

TASK(youmu_mirror_bomb_postprocess, { YoumuAMyon *myon; }) {
	YoumuAMyon *myon = ARGS.myon;
	CoEvent *pp_event = &stage_get_draw_events()->postprocess_before_overlay;

	ShaderProgram *shader = res_shader("youmua_bomb");
	Uniform *u_tbomb = r_shader_uniform(shader, "tbomb");
	Uniform *u_myon = r_shader_uniform(shader, "myon");
	Uniform *u_fill_overlay = r_shader_uniform(shader, "fill_overlay");

	for(;;) {
		WAIT_EVENT_OR_DIE(pp_event);

		float t = player_get_bomb_progress(&global.plr);
		float f = max(0, 1 - 10 * t);
		cmplx myonpos = CMPLX(re(myon->pos)/VIEWPORT_W, 1 - im(myon->pos)/VIEWPORT_H);

		FBPair *fbpair = stage_get_postprocess_fbpair();
		r_framebuffer(fbpair->back);

		r_state_push();
		r_shader_ptr(shader);
		r_uniform_float(u_tbomb, t);
		r_uniform_vec2_complex(u_myon, myonpos);
		r_uniform_vec4(u_fill_overlay, f, f, f, f);
		draw_framebuffer_tex(fbpair->front, VIEWPORT_W, VIEWPORT_H);
		r_state_pop();

		fbpair_swap(fbpair);
	}
}

TASK(youmu_mirror_bomb, { YoumuAController *ctrl; }) {
	YoumuAController *ctrl = ARGS.ctrl;
	YoumuAMyon *myon = &ctrl->myon;
	Player *plr = ctrl->plr;

	INVOKE_SUBTASK(youmu_common_bomb_background, ENT_BOX(plr), &ctrl->bomb_bg);
	INVOKE_SUBTASK(youmu_mirror_bomb_postprocess, myon);

	cmplx vel = -30 * myon->dir;
	cmplx pos = myon->pos;

	ProjFlags pflags = PFLAG_MANUALANGLE | PFLAG_NOREFLECT;
	int t = 0;

	ShaderProgram *silhouette_shader = res_shader("sprite_silhouette");

	YIELD;

	do {
		pos += vel;

		cmplx aim = cclampabs((plr->pos - pos) * 0.01, 1);
		vel += aim;
		vel *= 1 - cabs(plr->pos - pos) * 0.0001;

		youmu_mirror_bomb_particles(ctrl, pos, vel, t, pflags);
		pflags ^= PFLAG_REQUIREDPARTICLE;

		// roughly matches the shader effect
		real bombtime = player_get_bomb_progress(plr);
		real envelope = bombtime * (1 - bombtime);
		real range = 200 / (1 + pow(0.08 / envelope, 5));

		ent_area_damage(pos, range, &(DamageInfo) { 200, DMG_PLAYER_BOMB }, youmu_mirror_bomb_damage_callback, ctrl);
		stage_clear_hazards_at(pos, range, CLEAR_HAZARDS_ALL | CLEAR_HAZARDS_NOW);

		myon->pos = pos;
		myon->dir = cnormalize(vel);
		++t;

		PARTICLE(
			.color = RGBA(0.5, 1, 1, 0),
			.draw_rule = youmu_mirror_draw_speed_trail,
			.flags = PFLAG_NOREFLECT,
			.layer = LAYER_PARTICLE_HIGH,
			.pos = plr->pos,
			.shader_ptr = silhouette_shader,
			.sprite_ptr = aniplayer_get_frame(&plr->ani),
			.timeout = 15,
		);

		YIELD;
	} while(player_is_bomb_active(plr));
}

TASK(youmu_mirror_bomb_handler, { YoumuAController *ctrl; }) {
	YoumuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	BoxedTask bomb_task = {};

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.bomb_used);
		CANCEL_TASK(bomb_task);
		play_sfx("bomb_youmu_b");
		bomb_task = cotask_box(INVOKE_SUBTASK(youmu_mirror_bomb, ctrl));
	}
}

TASK(youmu_mirror_shot_forward, { YoumuAController *ctrl; }) {
	YoumuAController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	ShaderProgram *shader = res_shader("sprite_particle");

	for(int t = 0;;) {
		WAIT_EVENT_OR_DIE(&plr->events.shoot);
		play_sfx_loop("generic_shot");

		cmplx v = -20 * I;
		int power_rank = player_get_effective_power(plr) / 100;

		real spread = M_PI/64 * (1 + 0.5 * psin(t/15.0));

		for(int side = -1; side < 2; side += 2) {
			cmplx origin = plr->pos + 10*side + 5*I;

			youmu_common_shot(origin, move_linear(v), SHOT_SELF_DAMAGE, shader);

			for(int p = 0; p < power_rank; ++p) {
				cmplx v2 = -(20 - p) * I * cdir(side * (1 + p) * spread);

				youmu_common_shot(
					origin,
					move_asymptotic_halflife(
						0.2 * v2 * cdir(M_PI * 0.25 * side),
						v2 * cdir(M_PI * -0.02 * side),
						5
					), SHOT_SELF_DAMAGE, shader
				);
			}
		}

		t += WAIT(SHOT_SELF_DELAY);
	}

}

TASK(youmu_mirror_controller, { BoxedPlayer plr; }) {
	YoumuAController *ctrl = TASK_MALLOC(sizeof(*ctrl));
	ctrl->plr = TASK_BIND(ARGS.plr);
	ctrl->sprites.arc = res_sprite("part/arc");
	ctrl->sprites.stain = res_sprite("part/stain");
	ctrl->sprites.petal = res_sprite("part/petal");
	ctrl->sprites.blast_huge_halo = res_sprite("part/blast_huge_halo");

	youmu_common_init_bomb_background(&ctrl->bomb_bg);

	INVOKE_SUBTASK(youmu_mirror_shot_forward, ctrl);
	INVOKE_SUBTASK(youmu_mirror_myon, ctrl);
	INVOKE_SUBTASK(youmu_mirror_bomb_handler, ctrl);

	STALL;
}

static void youmu_mirror_init(Player *plr) {
	INVOKE_TASK(youmu_mirror_controller, ENT_BOX(plr));
}

static void youmu_mirror_preload(ResourceGroup *rg) {
	const int flags = RESF_DEFAULT;

	res_group_preload(rg, RES_SPRITE, flags,
		"part/arc",
		"part/blast_huge_halo",
		"part/myon",
		"part/petal",
		"part/stain",
		"part/stardust",
		"proj/youmu",
	NULL);

	res_group_preload(rg, RES_TEXTURE, flags,
		"youmu_bombbg1",
	NULL);

	res_group_preload(rg, RES_SHADER_PROGRAM, flags,
		"sprite_youmu_myon_shot",
		"youmu_bomb_bg",
		"youmua_bomb",
	NULL);

	res_group_preload(rg, RES_SFX, flags | RESF_OPTIONAL,
		"bomb_youmu_b",
	NULL);
}

static double youmu_mirror_property(Player *plr, PlrProperty prop) {
	switch(prop) {
		case PLR_PROP_SPEED: {
			double s = youmu_common_property(plr, prop);

			if(player_is_bomb_active(plr)) {
				s *= 2.0;
			}

			return s;
		}

		default:
			return youmu_common_property(plr, prop);
	}
}

PlayerMode plrmode_youmu_a = {
	.name = N_("Soul Reflection"),
	.description = N_("Human and phantom act together towards a singular purpose. Your inner duality shall lend you a hand… or a tail."),
	.spellcard_name = N_("Soul Sign “Reality-Piercing Apparition”"),
	.character = &character_youmu,
	.dialog = &dialog_tasks_youmu,
	.shot_mode = PLR_SHOT_YOUMU_MIRROR,
	.procs = {
		.init = youmu_mirror_init,
		.preload = youmu_mirror_preload,
		.property = youmu_mirror_property,
	},
};
