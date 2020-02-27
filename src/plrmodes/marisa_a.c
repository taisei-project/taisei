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
#include "stagedraw.h"

// args are pain
static int _laser_renderer_ref;
#define laser_renderer ((Enemy*)REF(_laser_renderer_ref))

typedef struct MarisaLaserData {
	struct {
		cmplx first;
		cmplx last;
	} trace_hit;
	cmplx prev_pos;
	float lean;
} MarisaLaserData;

static void draw_laser_beam(cmplx src, cmplx dst, double size, double step, double t, Texture *tex, Uniform *u_length) {
	cmplx dir = dst - src;
	cmplx center = (src + dst) * 0.5;

	r_mat_mv_push();

	r_mat_mv_translate(creal(center), cimag(center), 0);
	r_mat_mv_rotate(carg(dir), 0, 0, 1);
	r_mat_mv_scale(cabs(dir), size, 1);

	r_mat_tex_push_identity();
	r_mat_tex_translate(-cimag(src) / step + t, 0, 0);
	r_mat_tex_scale(cabs(dir) / step, 1, 1);

	r_uniform_sampler("tex", tex);
	r_uniform_float(u_length, cabs(dir) / step);
	r_draw_quad();

	r_mat_tex_pop();
	r_mat_mv_pop();
}

static void trace_laser(Enemy *e, cmplx vel, float damage) {
	ProjCollisionResult col;
	ProjectileList lproj = { .first = NULL };

	MarisaLaserData *ld = REF(e->args[3]);

	PROJECTILE(
		.dest = &lproj,
		.pos = e->pos,
		.size = 28*(1+I),
		.type = PROJ_PLAYER,
		.damage = damage,
		.rule = linear,
		.args = { vel },
	);

	bool first_found = false;
	int timeofs = 0;
	int col_types = PCOL_ENTITY;

	struct enemy_col {
		Enemy *enemy;
		int original_hp;
	} enemy_collisions[64] = { 0 };  // 64 collisions ought to be enough for everyone

	int num_enemy_collissions = 0;

	while(lproj.first) {
		timeofs = trace_projectile(lproj.first, &col, col_types | PCOL_VOID, timeofs);
		struct enemy_col *c = NULL;

		if(!first_found) {
			ld->trace_hit.first = col.location;
			first_found = true;
		}

		if(col.type & col_types) {
			RNG_ARRAY(R, 3);
			PARTICLE(
				.sprite = "flare",
				.pos = col.location,
				.rule = linear,
				.timeout = vrng_range(R[0], 3, 8),
				.draw_rule = pdraw_timeout_scale(2, 0.01),
				.args = { vrng_range(R[1], 2, 8) * vrng_dir(R[2]) },
				.flags = PFLAG_NOREFLECT,
				.layer = LAYER_PARTICLE_HIGH,
			);

			if(col.type == PCOL_ENTITY && col.entity->type == ENT_ENEMY) {
				assert(num_enemy_collissions < ARRAY_SIZE(enemy_collisions));
				if(num_enemy_collissions < ARRAY_SIZE(enemy_collisions)) {
					c = enemy_collisions + num_enemy_collissions++;
					c->enemy = ENT_CAST(col.entity, Enemy);
				}
			} else {
				col_types &= ~col.type;
			}

			col.fatal = false;
		}

		apply_projectile_collision(&lproj, lproj.first, &col);

		if(c) {
			c->original_hp = (ENT_CAST(col.entity, Enemy))->hp;
			(ENT_CAST(col.entity, Enemy))->hp = ENEMY_IMMUNE;
		}
	}

	assume(num_enemy_collissions < ARRAY_SIZE(enemy_collisions));

	for(int i = 0; i < num_enemy_collissions; ++i) {
		enemy_collisions[i].enemy->hp = enemy_collisions[i].original_hp;
	}

	ld->trace_hit.last = col.location;
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

static void marisa_laser_slave_visual(Enemy *e, int t, bool render) {
	if(!render) {
		marisa_common_slave_visual(e, t, render);
		return;
	}

	float laser_alpha = laser_renderer->args[0];

	ShaderCustomParams shader_params;
	shader_params.color = *RGBA(0.2, 0.4, 0.5, laser_renderer->args[1] * 0.75);

	r_draw_sprite(&(SpriteParams) {
		.sprite = "hakkero",
		.shader = "sprite_hakkero",
		.pos = { creal(e->pos), cimag(e->pos) },
		.rotation.angle = t * 0.05,
		.color = color_lerp(RGB(0.2, 0.4, 0.5), RGB(1.0, 1.0, 1.0), 0.25 * pow(psin(t / 6.0), 2) * laser_renderer->args[1]),
		.shader_params = &shader_params,
	});

	if(laser_alpha <= 0) {
		return;
	}

	MarisaLaserData *ld = REF(e->args[3]);

	r_draw_sprite(&(SpriteParams) {
		.sprite = "part/smoothdot",
		.color = RGBA_MUL_ALPHA(1, 1, 1, laser_alpha),
		.pos = { creal(ld->trace_hit.first), cimag(ld->trace_hit.first) },
	});
}

static void marisa_laser_fader_visual(Enemy *e, int t, bool render) {
}

static float get_laser_alpha(Enemy *e, float a) {
	if(e->visual_rule == marisa_laser_fader_visual) {
		return e->args[1];
	}

	return min(a, min(1, (global.frames - e->birthtime) * 0.1));
}

#define FOR_EACH_SLAVE(e) for(Enemy *e = global.plr.slaves.first; e; e = e->next) if(e->visual_rule == marisa_laser_fader_visual || e->visual_rule == marisa_laser_slave_visual)
#define FOR_EACH_REAL_SLAVE(e) FOR_EACH_SLAVE(e) if(e->visual_rule == marisa_laser_slave_visual)

static void marisa_laser_renderer_visual(Enemy *renderer, int t, bool render) {
	if(!render) {
		return;
	}

	double a = creal(renderer->args[0]);
	ShaderProgram *shader = r_shader_get("marisa_laser");
	Uniform *u_clr0 = r_shader_uniform(shader, "color0");
	Uniform *u_clr1 = r_shader_uniform(shader, "color1");
	Uniform *u_clr_phase = r_shader_uniform(shader, "color_phase");
	Uniform *u_clr_freq = r_shader_uniform(shader, "color_freq");
	Uniform *u_alpha = r_shader_uniform(shader, "alphamod");
	Uniform *u_length = r_shader_uniform(shader, "laser_length");
	Texture *tex0 = get_tex("part/marisa_laser0");
	Texture *tex1 = get_tex("part/marisa_laser1");
	FBPair *fbp_aux = stage_get_fbpair(FBPAIR_FG_AUX);
	Framebuffer *target_fb = r_framebuffer_current();

	r_shader_ptr(shader);
	r_uniform_vec4(u_clr0,       0.5, 0.5, 0.5, 0.0);
	r_uniform_vec4(u_clr1,       0.8, 0.8, 0.8, 0.0);
	r_uniform_float(u_clr_phase, -1.5 * t/M_PI);
	r_uniform_float(u_clr_freq,  10.0);
	r_framebuffer(fbp_aux->back);
	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);
	r_color4(1, 1, 1, 1);

	r_blend(r_blend_compose(
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_ONE, BLENDOP_MAX,
		BLENDFACTOR_SRC_COLOR, BLENDFACTOR_ONE, BLENDOP_MAX
	));

	FOR_EACH_SLAVE(e) {
		if(set_alpha(u_alpha, get_laser_alpha(e, a))) {
			MarisaLaserData *ld = REF(e->args[3]);
			draw_laser_beam(e->pos, ld->trace_hit.last, 32, 128, -0.02 * t, tex1, u_length);
		}
	}

	r_blend(BLEND_PREMUL_ALPHA);
	fbpair_swap(fbp_aux);

	stage_draw_begin_noshake();

	r_framebuffer(fbp_aux->back);
	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);
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

	FOR_EACH_SLAVE(e) {
		if(set_alpha_dimmed(u_alpha, get_laser_alpha(e, a))) {
			MarisaLaserData *ld = REF(e->args[3]);
			draw_laser_beam(e->pos, ld->trace_hit.first, 40, 128, t * -0.12, tex0, u_length);
		}
	}

	r_uniform_vec4(u_clr0, 2.0, 1.0, 2.0, 0.0);
	r_uniform_vec4(u_clr1, 0.1, 0.1, 1.0, 0.0);

	FOR_EACH_SLAVE(e) {
		if(set_alpha_dimmed(u_alpha, get_laser_alpha(e, a))) {
			MarisaLaserData *ld = REF(e->args[3]);
			draw_laser_beam(e->pos, ld->trace_hit.first, 42, 200, t * -0.03, tex0, u_length);
		}
	}
}

static int marisa_laser_fader(Enemy *e, int t) {
	if(t == EVENT_DEATH) {
		MarisaLaserData *ld = REF(e->args[3]);
		free(ld);
		free_ref(e->args[3]);
		return ACTION_DESTROY;
	}

	e->args[1] = approach(e->args[1], 0.0, 0.1);

	if(creal(e->args[1]) == 0) {
		return ACTION_DESTROY;
	}

	return 1;
}

static Enemy* spawn_laser_fader(Enemy *e, double alpha) {
	MarisaLaserData *ld = calloc(1, sizeof(MarisaLaserData));
	memcpy(ld, (MarisaLaserData*)REF(e->args[3]), sizeof(MarisaLaserData));

	return create_enemy_p(&global.plr.slaves, e->pos, ENEMY_IMMUNE, marisa_laser_fader_visual, marisa_laser_fader,
		e->args[0], alpha, e->args[2], add_ref(ld));
}

static int marisa_laser_renderer(Enemy *renderer, int t) {
	if(player_should_shoot(&global.plr, true) && renderer->next) {
		renderer->args[0] = approach(renderer->args[0], 1.0, 0.2);
		renderer->args[1] = approach(renderer->args[1], 1.0, 0.2);
		renderer->args[2] = 1;
	} else {
		if(creal(renderer->args[2])) {
			FOR_EACH_REAL_SLAVE(e) {
				spawn_laser_fader(e, renderer->args[0]);
			}

			renderer->args[2] = 0;
		}

		renderer->args[0] = 0;
		renderer->args[1] = approach(renderer->args[1], 0.0, 0.1);
	}

	return 1;
}

#undef FOR_EACH_SLAVE
#undef FOR_EACH_REAL_SLAVE

static void marisa_laser_flash_draw(Projectile *p, int t, ProjDrawRuleArgs args) {
	SpriteParamsBuffer spbuf;
	SpriteParams sp = projectile_sprite_params(p, &spbuf);
	float o = 1 - t / p->timeout;
	color_mul_scalar(&spbuf.color, o);
	spbuf.color.r *= o;
	r_draw_sprite(&sp);
}

static int marisa_laser_slave(Enemy *e, int t) {
	if(t == EVENT_BIRTH) {
		return 1;
	}

	if(t == EVENT_DEATH) {
		if(!(global.gameover > 0) && laser_renderer && creal(laser_renderer->args[0])) {
			spawn_laser_fader(e, laser_renderer->args[0]);
		}

		MarisaLaserData *ld = REF(e->args[3]);
		free_ref(e->args[3]);
		free(ld);
		return 1;
	}

	if(t < 0) {
		return 1;
	}

	cmplx target_pos = global.plr.pos + (1 - global.plr.focus/30.0)*e->pos0 + (global.plr.focus/30.0)*e->args[0];
	e->pos += (target_pos - e->pos) * 0.5;

	MarisaLaserData *ld = REF(e->args[3]);
	cmplx pdelta = e->pos - ld->prev_pos;
	ld->prev_pos = e->pos;
	ld->lean += (-0.01 * creal(pdelta) - ld->lean) * 0.2;

	if(player_should_shoot(&global.plr, true)) {
		float angle = creal(e->args[2]);
		float f = smoothreclamp(global.plr.focus, 0, 30, 0, 1);
		f = smoothreclamp(f, 0, 1, 0, 1);
		float factor = (1.0 + 0.7 * psin(t/15.0)) * -(1-f) * !!angle;

		cmplx dir = -cexp(I*(angle*factor + ld->lean + M_PI/2));
		trace_laser(e, 5 * dir, creal(e->args[1]));

		Animation *fire = get_ani("fire");
		AniSequence *seq = get_ani_sequence(fire, "main");
		Sprite *spr = animation_get_frame(fire, seq, global.frames);

		PARTICLE(
			.sprite_ptr = spr,
			.size = 1+I,
			.pos = e->pos + dir * 10,
			.color = color_mul_scalar(RGBA(2, 0.2, 0.5, 0), 0.2),
			.rule = linear,
			.draw_rule = marisa_laser_flash_draw,
			.timeout = 8,
			.args = { dir, 0, 0.6 + 0.2*I, },
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
			.scale = 0.4,
			// .layer = LAYER_PARTICLE_LOW,
		);
	}

	return 1;
}

static float masterspark_width(void) {
	float t = player_get_bomb_progress(&global.plr);
	float w = 1;

	if(t < 1./6) {
		w = t*6;
		w = pow(w, 1.0/3.0);
	}

	if(t > 4./5) {
		w = 1-t*5 + 4;
		w = pow(w, 5);
	}

	return w;
}

static void masterspark_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	float fade = masterspark_width();

	marisa_common_masterspark_draw(1, &(MarisaBeamInfo){global.plr.pos - 30 * I, 800 + I * VIEWPORT_H * 1.25, carg(e->args[0]), t}, fade);
}

static int masterspark_star(Projectile *p, int t) {
	if(t >= 0) {
		p->args[0] += 0.1*p->args[0]/cabs(p->args[0]);
		p->angle += 0.1;
	}

	return linear(p, t);
}

static void masterspark_damage(Enemy *e) {
	// lazy inefficient approximation of the beam parabola

	float r = 96 * masterspark_width();
	float growth = 0.25;
	cmplx v = e->args[0] * cexp(-I*M_PI*0.5);
	cmplx p = global.plr.pos - 30 * I + r * v;

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

static int masterspark(Enemy *e, int t2) {
	// FIXME: This may interact badly with other view shake effects...
	// We need a proper system for this stuff.

	if(t2 == EVENT_BIRTH) {
		global.shake_view = 8;
		return 1;
	} else if(t2 == EVENT_DEATH) {
		global.shake_view = 0;
		return 1;
	}

	if(t2 < 0)
		return 1;

	e->args[0] *= cexp(I*(0.005*creal(global.plr.velocity) + rng_sreal() * 0.005));
	cmplx diroffset = e->args[0];

	float t = player_get_bomb_progress(&global.plr);

	if(t >= 3.0/4.0) {
		global.shake_view = 8 * (1 - t * 4 + 3);
	} else if(t2 % 2 == 0) {
		cmplx dir = -cexp(1.5*I*sin(t2*M_PI*1.12))*I;
		Color *c = HSLA(-t*5.321, 1, 0.5, rng_range(0, 0.5));

		uint flags = PFLAG_NOREFLECT;

		if(t2 % 4 == 0) {
			flags |= PFLAG_REQUIREDPARTICLE;
		}

		PARTICLE(
			.sprite = "maristar_orbit",
			.pos = global.plr.pos+40*dir,
			.color = c,
			.rule = masterspark_star,
			.timeout = 50,
			.args= { (10 * dir - 10*I)*diroffset },
			.angle = rng_angle(),
			.draw_rule = pdraw_timeout_scalefade(0, 5, 1, 0),
			.flags = flags,
		);
		dir = -conj(dir);
		PARTICLE(
			.sprite = "maristar_orbit",
			.pos = global.plr.pos+40*dir,
			.color = c,
			.rule = masterspark_star,
			.timeout = 50,
			.args = { (10 * dir - 10*I)*diroffset },
			.angle = rng_angle(),
			.draw_rule = pdraw_timeout_scalefade(0, 5, 1, 0),
			.flags = flags,
		);
		PARTICLE(
			.sprite = "smoke",
			.pos = global.plr.pos-40*I,
			.color = HSLA(2*t,1,2,0), //RGBA(0.3, 0.6, 1, 0),
			.timeout = 50,
			.move = move_linear(-7*dir + 7*I),
			.angle = rng_angle(),
			.draw_rule = pdraw_timeout_scalefade(0, 7, 1, 0),
			.flags = flags | PFLAG_MANUALANGLE,
		);
	}

	if(t >= 1 || !player_is_bomb_active(&global.plr)) {
		return ACTION_DESTROY;
	}

	masterspark_damage(e);
	return 1;
}

static void marisa_laser_bombbg(Player *plr) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	float t = player_get_bomb_progress(&global.plr);
	float fade = 1;

	if(t < 1./6)
		fade = t*6;

	if(t > 3./4)
		fade = 1-t*4 + 3;

	r_color4(0.8 * fade, 0.8 * fade, 0.8 * fade, 0.8 * fade);
	fill_viewport(sin(t * 0.3), t * 3 * (1 + t * 3), 1, "marisa_bombbg");
	r_color4(1, 1, 1, 1);
}

static void marisa_laser_bomb(Player *plr) {
	play_sound("bomb_marisa_a");
	Enemy *e = create_enemy_p(&plr->slaves, 0.0*I, ENEMY_BOMB, masterspark_visual, masterspark, 1,0,0,0);
	e->ent.draw_layer = LAYER_PLAYER_FOCUS - 1;
}

static Enemy* marisa_laser_spawn_slave(Player *plr, cmplx pos, cmplx a0, cmplx a1, cmplx a2, cmplx a3) {
	Enemy *e = create_enemy_p(&plr->slaves, pos, ENEMY_IMMUNE, marisa_laser_slave_visual, marisa_laser_slave, a0, a1, a2, a3);
	e->pos = plr->pos;
	return e;
}

static void marisa_laser_respawn_slaves(Player *plr, short npow) {
	Enemy *e = plr->slaves.first, *tmp;
	double dmg = 8;

	while(e != 0) {
		tmp = e;
		e = e->next;
		if(tmp->logic_rule == marisa_laser_slave) {
			delete_enemy(&plr->slaves, tmp);
		}
	}

	if(npow / 100 == 1) {
		marisa_laser_spawn_slave(plr, -40.0*I, -40.0*I, dmg, 0, 0);
	}

	if(npow >= 200) {
		marisa_laser_spawn_slave(plr,  25-5.0*I,  9-40.0*I, dmg, -M_PI/30, 0);
		marisa_laser_spawn_slave(plr, -25-5.0*I, -9-40.0*I, dmg,  M_PI/30, 0);
	}

	if(npow / 100 == 3) {
		marisa_laser_spawn_slave(plr, -30.0*I, -55.0*I, dmg, 0, 0);
	}

	if(npow >= 400) {
		marisa_laser_spawn_slave(plr,  17-30.0*I,  18-55.0*I, dmg,  M_PI/60, 0);
		marisa_laser_spawn_slave(plr, -17-30.0*I, -18-55.0*I, dmg, -M_PI/60, 0);
	}

	for(e = plr->slaves.first; e; e = e->next) {
		if(e->logic_rule == marisa_laser_slave) {
			MarisaLaserData *ld = calloc(1, sizeof(MarisaLaserData));
			ld->prev_pos = e->pos;
			e->args[3] = add_ref(ld);
			e->ent.draw_layer = LAYER_PLAYER_SLAVE;
		}
	}
}

static void marisa_laser_power(Player *plr, short npow) {
	if(plr->power / 100 == npow / 100) {
		return;
	}

	marisa_laser_respawn_slaves(plr, npow);
}

static void marisa_laser_init(Player *plr) {
	Enemy *e = create_enemy_p(&plr->slaves, 0, ENEMY_IMMUNE, marisa_laser_renderer_visual, marisa_laser_renderer, 0, 0, 0, 0);
	e->ent.draw_layer = LAYER_PLAYER_SHOT_HIGH;
	_laser_renderer_ref = add_ref(e);
	marisa_laser_respawn_slaves(plr, plr->power);
}

static void marisa_laser_free(Player *plr) {
	free_ref(_laser_renderer_ref);
}

static void marisa_laser_shot(Player *plr) {
	marisa_common_shot(plr, 175 - 4 * (plr->power / 100));
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

static void marisa_laser_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"proj/marisa",
		"part/maristar_orbit",
		"hakkero",
		"masterspark_ring",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"marisa_bombbg",
		"part/marisa_laser0",
		"part/marisa_laser1",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"blur25",
		"blur5",
		"marisa_laser",
		"masterspark",
		"max_to_alpha",
		"sprite_hakkero",
	NULL);

	preload_resources(RES_ANIM, flags,
		"fire",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_marisa_a",
	NULL);
}

PlayerMode plrmode_marisa_a = {
	.name = "Illusion Laser",
	.description = "Magic missiles and lasers — simple and to the point. They’ve never let you down before, so why stop now?",
	.spellcard_name = "Pure Love “Galactic Spark”",
	.character = &character_marisa,
	.dialog = &dialog_tasks_marisa,
	.shot_mode = PLR_SHOT_MARISA_LASER,
	.procs = {
		.property = marisa_laser_property,
		.bomb = marisa_laser_bomb,
		.bombbg = marisa_laser_bombbg,
		.shot = marisa_laser_shot,
		.power = marisa_laser_power,
		.preload = marisa_laser_preload,
		.init = marisa_laser_init,
		.free = marisa_laser_free,
	},
};
