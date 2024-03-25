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
#include "marisa.h"
#include "renderer/api.h"
#include "common_tasks.h"
#include "util/glm.h"
#include "stagedraw.h"

#define SHOT_FORWARD_DAMAGE 60
#define SHOT_FORWARD_DELAY 6

#define SHOT_SLAVE_DAMAGE 50
#define SHOT_SLAVE_DELAY 5

#define BOMB_NUM_ORBITERS 5

#define HAKKERO_RETRACT_TIME 6

typedef struct MarisaBController {
	struct {
		Sprite *fairy_circle;
		Sprite *lightningball;
		Sprite *maristar_orbit;
		Sprite *stardust;
	} sprites;

	Player *plr;
	cmplx slave_ref_pos;  // follows player

	struct {
		float beams_alpha;
	} bomb;

	COEVENTS_ARRAY(
		slaves_expired
	) events;
} MarisaBController;

DEFINE_ENTITY_TYPE(MarisaBSlave, {
	Sprite *sprite;
	ShaderProgram *shader;
	cmplx pos;
	uint alive;
});

DEFINE_ENTITY_TYPE(MarisaBOrbiter, {
	MarisaBController *ctrl;
	Sprite *sprite;
	cmplx pos;
	cmplx offset;
	Color particle_color;
	Color circle_color;
});

DEFINE_ENTITY_TYPE(MarisaBBeams, {
	BoxedMarisaBOrbiter orbiters[BOMB_NUM_ORBITERS];
	int time;
	float alpha;
});

static void marisa_star_draw_slave(EntityInterface *ent) {
	MarisaBSlave *slave = ENT_CAST(ent, MarisaBSlave);

	ShaderCustomParams shader_params = { 0 };
	// shader_params.color = *RGBA(0.2, 0.4, 0.5, slave->flare_alpha * 0.75);
	float t = global.frames;

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = slave->sprite,
		.shader_ptr = slave->shader,
		.pos.as_cmplx = slave->pos,
		.rotation.angle = t * 0.05f,
		// .color = color_lerp(RGB(0.2, 0.4, 0.5), RGB(1.0, 1.0, 1.0), 0.25 * powf(psinf(t / 6.0f), 2.0f) * slave->flare_alpha),
		.color = RGB(0.2, 0.4, 0.5),
		.shader_params = &shader_params,
	});
}

static Color *marisa_star_slave_projectile_color(Color *c, real focus, real brightener) {
	static const Color unfocused = { 0.3, 0.8, 1.0, 0.2 };
	static const Color focused   = { 1.0, 0.8, 0.3, 0.2 };
	*c = unfocused;
	color_lerp(c, &focused, focus);
	return color_add(c, RGBA(brightener, brightener, brightener, brightener));
}

TASK(marisa_star_slave_projectile, {
	MarisaBController *ctrl;
	BoxedMarisaBSlave slave;
	cmplx pos;
	cmplx vel;
	real damage;
	ShaderProgram *shader;
}) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	MarisaBSlave *slave = NOT_NULL(ENT_UNBOX(ARGS.slave));

	if(!slave->alive || !player_should_shoot(plr)) {
		return;
	}

	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_maristar,
		.pos = ARGS.pos,
		.move = move_linear(ARGS.vel),
		.type = PROJ_PLAYER,
		.damage = ARGS.damage,
		.shader_ptr = ARGS.shader,
		.max_viewport_dist = 128,
	));

	real freq = 0.1;
	cmplx vel = ARGS.vel;
	real focus = !!(plr->inputflags & INFLAG_FOCUS);

	cmplx prev_pos = p->pos;
	cmplx next_pos = p->pos;

	for(int t = 0;; ++t) {
		slave = ENT_UNBOX(ARGS.slave);

		if(slave == NULL || !slave->alive || !player_should_shoot(plr)) {
			break;
		}

		approach_asymptotic_p(&focus, !!(plr->inputflags & INFLAG_FOCUS), 0.2, 1e-3);

		real focusfac = 1;

		if(focus < 1) {
			focusfac = t * 0.015 - 1 / (1 - focus);
			focusfac = tanh(sqrt(fabs(focusfac)));
		}

		cmplx center = clerp(plr->pos, ctrl->slave_ref_pos, tanh(t / 10.0));

		real brightener = -1 / (1 + sqrt(0.03 * fabs(re(p->pos - center))));
		marisa_star_slave_projectile_color(&p->color, focus, brightener);

		real verticalfac = - 5 * t * (1 + 0.01 * t) + 10 * t / (0.01 * t + 1);

		prev_pos = p->pos;
		next_pos = center + focusfac * cbrt(0.1 * t) * re(vel) * 70 * sin(freq * t + im(vel)) + verticalfac*I;
		p->move.velocity = next_pos - prev_pos;

		if(t%(2+(int)round(2*rng_real())) == 0) {  // please never write stuff like this ever again
			PARTICLE(
				.sprite_ptr = ctrl->sprites.stardust,
				.pos = next_pos,
				.color = RGBA(0.5 * focus, 0, 0.5 * (1 - focus), 0),
				.timeout = 5,
				.angle = rng_angle(),
				.angle_delta = 0.1 * rng_sreal(),
				.draw_rule = pdraw_timeout_scalefade(0, 1.4, 1, 0),
				.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
			);
		}

		YIELD;
	}

	p->move.retention = 1.005;
}

TASK(marisa_star_slave_shot, {
	MarisaBController *ctrl;
	BoxedMarisaBSlave slave;
	real phase;
}) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	MarisaBSlave *slave = TASK_BIND(ARGS.slave);

	ShaderProgram *shot_shader = res_shader("sprite_particle");
	real nphase = ARGS.phase / M_TAU;
	real damage = SHOT_SLAVE_DAMAGE;

	int t = 0;
	for(;;) {
		t += WAIT_EVENT_OR_DIE(&plr->events.shoot).frames;

		for(int i = 0; i < 2; ++i) {
			cmplx v = (1 - 2 * i);
			v *= 1 - 0.9 * nphase;
			v -= I * 0.04 * t * (4 - 3 * nphase);

			INVOKE_TASK(marisa_star_slave_projectile,
				.ctrl = ctrl,
				.slave = ENT_BOX(slave),
				.pos = slave->pos,
				.vel = v,
				.damage = damage,
				.shader = shot_shader
			);

			t += WAIT(2);
		}

		static_assert(SHOT_SLAVE_DELAY >= 4, "SHOT_SLAVE_DELAY is too low");
		t += WAIT(SHOT_SLAVE_DELAY - 4);
	}
}

TASK(marisa_star_slave, {
	MarisaBController *ctrl;
	real phase;
}) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	MarisaBSlave *slave = TASK_HOST_ENT(MarisaBSlave);
	slave->alive = 1;
	slave->pos = plr->pos;
	slave->shader = res_shader("sprite_hakkero");
	slave->sprite = res_sprite("hakkero");

	slave->ent.draw_func = marisa_star_draw_slave;
	slave->ent.draw_layer = LAYER_PLAYER_SLAVE;

	INVOKE_SUBTASK_WHEN(&ctrl->events.slaves_expired, common_set_bitflags,
		.pflags = &slave->alive,
		.mask = 0,
		.set = 0
	);

	BoxedTask shot_task = cotask_box(INVOKE_SUBTASK(marisa_star_slave_shot,
		.ctrl = ctrl,
		.slave = ENT_BOX(slave),
		.phase = ARGS.phase
	));

	real angle_step = 0.05;
	real angle = angle_step * global.frames + ARGS.phase;

	for(int t = 0; slave->alive; t += WAIT(1)) {
		cmplx target_pos = ctrl->slave_ref_pos + 80 * sin(angle) + 45*I;
		slave->pos = clerp(plr->pos, target_pos, glm_ease_quad_out(min(1, (real)t/HAKKERO_RETRACT_TIME)));
		slave->ent.draw_layer = cos(angle) < 0 ? LAYER_BACKGROUND : LAYER_PLAYER_SLAVE;
		angle += angle_step;
	}

	CANCEL_TASK(shot_task);
	plrutil_slave_retract(ENT_BOX(plr), &slave->pos, HAKKERO_RETRACT_TIME);
}

static void marisa_star_respawn_slaves(MarisaBController *ctrl, int numslaves) {
	Player *plr = ctrl->plr;
	coevent_signal(&ctrl->events.slaves_expired);
	ctrl->slave_ref_pos = plr->pos;

	for(int i = 0; i < numslaves; i++) {
		INVOKE_TASK(marisa_star_slave,
			.ctrl = ctrl,
			.phase = M_TAU * i / numslaves
		);
	}
}

TASK(marisa_star_power_handler, { MarisaBController *ctrl; }) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	int old_power = player_get_effective_power(plr) / 100;

	marisa_star_respawn_slaves(ctrl, old_power);

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.effective_power_changed);
		int new_power = player_get_effective_power(plr) / 100;
		if(old_power != new_power) {
			marisa_star_respawn_slaves(ctrl, new_power);
			old_power = new_power;
		}
	}
}

static void marisa_star_draw_orbiter(EntityInterface *ent) {
	MarisaBOrbiter *orbiter = ENT_CAST(ent, MarisaBOrbiter);
	MarisaBController *ctrl = orbiter->ctrl;

	SpriteParams sp = { 0 };
	sp.pos.as_cmplx = orbiter->pos;
	sp.color = color_mul_scalar(COLOR_COPY(&orbiter->circle_color), ctrl->bomb.beams_alpha);
	sp.rotation.angle = global.frames * 10 * DEG2RAD;
	sp.sprite_ptr = ctrl->sprites.fairy_circle;
	r_draw_sprite(&sp);
	sp.sprite_ptr = ctrl->sprites.lightningball;
	sp.scale.both = 0.6;
	r_draw_sprite(&sp);
}

static void marisa_star_draw_beams(EntityInterface *ent) {
	MarisaBBeams *beams_ent = ENT_CAST(ent, MarisaBBeams);
	MarisaBeamInfo beams[BOMB_NUM_ORBITERS], *pbeam = beams;

	float a = beams_ent->alpha;

	for(int i = 0; i < ARRAY_SIZE(beams_ent->orbiters); i++) {
		MarisaBOrbiter *orbiter = ENT_UNBOX(beams_ent->orbiters[i]);

		if(orbiter == NULL) {
			continue;
		}

		pbeam->origin = orbiter->pos;
		pbeam->size = 250 * a + VIEWPORT_H * 1.5 * I;
		pbeam->angle = carg(orbiter->offset) + M_PI/2;
		pbeam->t = beams_ent->time;
		++pbeam;
	}

	marisa_common_masterspark_draw(pbeam - beams, beams, a);
}

TASK(marisa_star_orbiter_stars, { MarisaBController *ctrl; BoxedMarisaBOrbiter orbiter; Color *color; }) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;
	MarisaBOrbiter *orbiter = TASK_BIND(ARGS.orbiter);

	for(int t = 0; t < 300; t += WAIT(10 - t / 30)) {
		cmplx vel = -5 * cnormalize(orbiter->pos - plr->pos);
		PARTICLE(
			.sprite_ptr = ctrl->sprites.maristar_orbit,
			.pos = orbiter->pos,
			.color = ARGS.color,
			.draw_rule = pdraw_timeout_scalefade(0, 6, 1, 0),
			.timeout = 150,
			.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
			.angle = t,
			.angle_delta = 0.1,
			.move = move_accelerated(vel, cnormalize(vel) * 0.15),
		);
	}
}

TASK(marisa_star_orbiter, { MarisaBController *ctrl; cmplx dir; real hue; BoxedMarisaBOrbiter *out_ref; }) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	MarisaBOrbiter *orbiter = TASK_HOST_ENT(MarisaBOrbiter);
	orbiter->ctrl = ctrl;
	orbiter->particle_color = *HSLA(ARGS.hue, 1, 0.6, 1);
	orbiter->circle_color = *HSLA(ARGS.hue, 0.9, 0.5, 0);
	orbiter->ent.draw_layer = LAYER_PLAYER_FOCUS - 1;
	orbiter->ent.draw_func = marisa_star_draw_orbiter;

	*ARGS.out_ref = ENT_BOX(orbiter);

	Color pcolor;

	BoxedTask stars_task = cotask_box(INVOKE_SUBTASK_DELAYED(1, marisa_star_orbiter_stars,
		.ctrl = ctrl,
		.orbiter = ENT_BOX(orbiter),
		.color = &pcolor
	));

	for(int t = 0;; t += WAIT(1)) {
		pcolor = orbiter->particle_color;

		real r = 100 * pow(tanh(t / 20.0), 2);
		orbiter->offset = ARGS.dir * r * cdir(sqrt(1000 + (t * t) * (1 + 0.03 * t)) * 0.04);
		orbiter->pos = plr->pos + orbiter->offset;

		real tb = player_get_bomb_progress(plr);
		real fadetime = 3.0 / 4.0;

		if(tb >= fadetime) {
			pcolor.a = 1 - (tb - fadetime) / (1 - fadetime);
			CANCEL_TASK(stars_task);
			stars_task = (BoxedTask) { 0 };
		}

		color_mul_alpha(&pcolor);
		pcolor.a = 0;

		PARTICLE(
			.sprite_ptr = ctrl->sprites.maristar_orbit,
			.pos = orbiter->pos,
			.color = color_mul_scalar(COLOR_COPY(&pcolor), 0.5),
			.timeout = 10,
			.angle = t * 0.1,
			.draw_rule = pdraw_timeout_scalefade(0, 1 + 4 * tb, 1, 0),
			.flags = PFLAG_NOREFLECT | PFLAG_MANUALANGLE,
		);
	}
}

TASK(marisa_star_bomb_background, { MarisaBController *ctrl; }) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	ShaderProgram *bg_shader = res_shader("maristar_bombbg");
	Uniform *u_t = r_shader_uniform(bg_shader, "t");
	Uniform *u_decay = r_shader_uniform(bg_shader, "decay");
	Uniform *u_plrpos = r_shader_uniform(bg_shader, "plrpos");
	Texture *bg_tex = res_texture("marisa_bombbg");

	for(;;) {
		WAIT_EVENT_OR_DIE(&stage_get_draw_events()->background_drawn);

		r_state_push();
		r_shader_ptr(bg_shader);
		r_uniform_float(u_t, player_get_bomb_progress(plr));
		r_uniform_float(u_decay, 1);
		r_uniform_vec2_complex(u_plrpos, cwmul(plr->pos, CMPLX(1.0/VIEWPORT_W, 1.0/VIEWPORT_H)));
		fill_viewport_p(0, 0, 1, 1, 0, bg_tex);
		r_state_pop();
	}
}

TASK(marisa_star_bomb, { MarisaBController *ctrl; }) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	INVOKE_SUBTASK(marisa_star_bomb_background, ctrl);

	YIELD;

	MarisaBBeams *beams = TASK_HOST_ENT(MarisaBBeams);
	beams->ent.draw_layer = LAYER_PLAYER_FOCUS - 1;
	beams->ent.draw_func = marisa_star_draw_beams;

	for(int i = 0; i < BOMB_NUM_ORBITERS; ++i) {
		INVOKE_SUBTASK(marisa_star_orbiter,
			.ctrl = ctrl,
			.dir = cdir(i * M_TAU / BOMB_NUM_ORBITERS),
			.hue = i / (real)BOMB_NUM_ORBITERS,
			.out_ref = beams->orbiters + i
		);
	}

	do {
		stage_shake_view(8);
		player_placeholder_bomb_logic(plr);

		float tb = player_get_bomb_progress(plr);
		float a = 1;

		if(tb < 1.0f / 6.0f) {
			a = tb * 6.0f;
			a = sqrtf(a);
		}

		if(tb > 3.0f / 4.0f) {
			a = 1.0f - tb * 4.0f + 3.0f;
			a *= a;
		}

		ctrl->bomb.beams_alpha = a;
		beams->alpha = a;
		++beams->time;
		YIELD;
	} while(player_is_bomb_active(plr));
}

TASK(marisa_star_bomb_handler, { MarisaBController *ctrl; }) {
	MarisaBController *ctrl = ARGS.ctrl;
	Player *plr = ctrl->plr;

	BoxedTask bomb_task = { 0 };

	for(;;) {
		WAIT_EVENT_OR_DIE(&plr->events.bomb_used);
		CANCEL_TASK(bomb_task);
		play_sfx("bomb_marisa_b");
		bomb_task = cotask_box(INVOKE_SUBTASK(marisa_star_bomb, ctrl));
	}
}

TASK(marisa_star_controller, { BoxedPlayer plr; }) {
	MarisaBController *ctrl = TASK_MALLOC(sizeof(*ctrl));
	ctrl->plr = TASK_BIND(ARGS.plr);
	TASK_HOST_EVENTS(ctrl->events);

	ctrl->sprites.fairy_circle = res_sprite("fairy_circle");
	ctrl->sprites.lightningball = res_sprite("part/lightningball");
	ctrl->sprites.maristar_orbit = res_sprite("part/maristar_orbit");
	ctrl->sprites.stardust = res_sprite("part/stardust");

	INVOKE_SUBTASK(marisa_star_power_handler, ctrl);
	INVOKE_SUBTASK(marisa_star_bomb_handler, ctrl);
	INVOKE_SUBTASK(marisa_common_shot_forward, ARGS.plr, SHOT_FORWARD_DAMAGE, SHOT_FORWARD_DELAY);

	ctrl->slave_ref_pos = ctrl->plr->pos;

	for(;;) {
		cmplx pdelta = ctrl->plr->pos - ctrl->slave_ref_pos;
		ctrl->slave_ref_pos += cclampabs(pdelta, 2 + 2 * !(ctrl->plr->inputflags & INFLAG_FOCUS));
		YIELD;
	}
}

static void marisa_star_init(Player *plr) {
	INVOKE_TASK(marisa_star_controller, { ENT_BOX(plr) });
}

static double marisa_star_property(Player *plr, PlrProperty prop) {
	switch(prop) {
		case PLR_PROP_SPEED: {
			double s = marisa_common_property(plr, prop);

			if(player_is_bomb_active(plr)) {
				s /= 4.0;
			}

			return s;
		}

		default:
			return marisa_common_property(plr, prop);
	}
}

static void marisa_star_preload(ResourceGroup *rg) {
	const int flags = RESF_DEFAULT;

	res_group_preload(rg, RES_SPRITE, flags,
		"hakkero",
		"masterspark_ring",
		"part/maristar_orbit",
		"part/stardust",
		"proj/marisa",
		"proj/maristar",
	NULL);

	res_group_preload(rg, RES_TEXTURE, flags,
		"marisa_bombbg",
	NULL);

	res_group_preload(rg, RES_SHADER_PROGRAM, flags,
		"masterspark",
		"maristar_bombbg",
		"sprite_hakkero",
	NULL);

	res_group_preload(rg, RES_SFX, flags | RESF_OPTIONAL,
		"bomb_marisa_b",
	NULL);
}

PlayerMode plrmode_marisa_b = {
	.name = "Stellar Vortex",
	.description = "As many bullets as there are stars in the sky. Some of them are bound to hit. That's called “homing”, right?",
	.spellcard_name = "Magic Sign “Stellar Vortex”",
	.character = &character_marisa,
	.dialog = &dialog_tasks_marisa,
	.shot_mode = PLR_SHOT_MARISA_STAR,
	.procs = {
		.init = marisa_star_init,
		.preload = marisa_star_preload,
		.property = marisa_star_property,
	},
};
