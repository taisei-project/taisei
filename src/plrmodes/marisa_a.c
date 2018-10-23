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
#include "marisa.h"
#include "renderer/api.h"
#include "stagedraw.h"

// args are pain
static Enemy *laser_renderer;

typedef struct MarisaLaserData {
	struct {
		complex first;
		complex last;
	} trace_hit;
	complex prev_pos;
	float lean;
} MarisaLaserData;

static void draw_laser_beam(complex src, complex dst, double size, double step, double t, Texture *tex, Uniform *u_length) {
	complex dir = dst - src;
	complex center = (src + dst) * 0.5;

	r_mat_push();

	r_mat_translate(creal(center), cimag(center), 0);
	r_mat_rotate_deg(180/M_PI*carg(dir), 0, 0, 1);
	r_mat_scale(cabs(dir), size, 1);

	r_mat_mode(MM_TEXTURE);
	r_mat_identity();
	r_mat_translate(-cimag(src) / step + t, 0, 0);
	r_mat_scale(cabs(dir) / step, 1, 1);
	r_mat_mode(MM_MODELVIEW);

	r_uniform_sampler("tex", tex);
	r_uniform_float(u_length, cabs(dir) / step);
	r_draw_quad();

	r_mat_mode(MM_TEXTURE);
	r_mat_identity();
	r_mat_mode(MM_MODELVIEW);

	r_mat_pop();
}

static void trace_laser(Enemy *e, complex vel, float damage) {
	ProjCollisionResult col;
	ProjectileList lproj = { .first = NULL };

	MarisaLaserData *ld = REF(e->args[3]);

	PROJECTILE(
		.dest = &lproj,
		.pos = e->pos,
		.size = 28*(1+I),
		.type = PlrProj,
		.damage = damage,
		.rule = linear,
		.args = { vel },
	);

	bool first_found = false;
	int timeofs = 0;
	int col_types = PCOL_ENTITY;

	struct enemy_col {
		LIST_INTERFACE(struct enemy_col);
		Enemy *enemy;
		int original_hp;
	} *prev_collisions = NULL;

	while(lproj.first) {
		timeofs = trace_projectile(lproj.first, &col, col_types | PCOL_VOID, timeofs);
		struct enemy_col *c = NULL;

		if(!first_found) {
			ld->trace_hit.first = col.location;
			first_found = true;
		}

		if(col.type & col_types) {
			tsrand_fill(3);
			PARTICLE(
				.sprite = "flare",
				.pos = col.location,
				.rule = linear,
				.timeout = 3 + 5 * afrand(2),
				.draw_rule = Shrink,
				.args = { (2+afrand(0)*6)*cexp(I*M_PI*2*afrand(1)) },
				.flags = PFLAG_NOREFLECT,
			);

			if(col.type == PCOL_ENTITY && col.entity->type == ENT_ENEMY) {
				c = malloc(sizeof(struct enemy_col));
				c->enemy = ENT_CAST(col.entity, Enemy);
				list_push(&prev_collisions, c);
			} else {
				col_types &= ~col.type;
			}

			col.fatal = false;
		}

		apply_projectile_collision(&lproj, lproj.first, &col, NULL);

		if(c) {
			c->original_hp = ((Enemy*)col.entity)->hp;
			((Enemy*)col.entity)->hp = ENEMY_IMMUNE;
		}
	}

	for(struct enemy_col *c = prev_collisions, *next; c; c = next) {
		next = c->next;
		c->enemy->hp = c->original_hp;
		list_unlink(&prev_collisions, c);
		free(c);
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
	FBPair *fbp_fg = stage_get_fbpair(FBPAIR_FG);

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

	r_framebuffer(fbp_aux->back);
	r_clear(CLEAR_COLOR, RGBA(0, 0, 0, 0), 1);
	r_shader("max_to_alpha");
	draw_framebuffer_tex(fbp_aux->front, VIEWPORT_W, VIEWPORT_H);
	fbpair_swap(fbp_aux);

	r_framebuffer(fbp_fg->back);
	r_shader_standard();
	r_color4(1, 1, 1, 1);
	draw_framebuffer_tex(fbp_aux->front, VIEWPORT_W, VIEWPORT_H);
	r_shader_ptr(shader);

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

	r_shader("sprite_default");
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

static void marisa_laser_flash_draw(Projectile *p, int t) {
	Animation *fire = get_ani("fire");
	AniSequence *seq = get_ani_sequence(fire, "main");
	Sprite *spr = animation_get_frame(fire, seq, p->birthtime);

	Color *c = color_mul_scalar(COLOR_COPY(&p->color), 1 - t / p->timeout);
	c->r *= (1 - t / p->timeout);

	complex pos = p->pos;
	pos += p->args[0] * 10;

	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = spr,
		.color = c,
		.pos = { creal(pos), cimag(pos)},
		.rotation.angle = p->angle + M_PI/2,
		.scale.both = 0.40,
	});
}

static int marisa_laser_slave(Enemy *e, int t) {
	if(t == EVENT_BIRTH) {
		return 1;
	}

	if(t == EVENT_DEATH) {
		if(!global.game_over && creal(laser_renderer->args[0])) {
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

	complex target_pos = global.plr.pos + (1 - global.plr.focus/30.0)*e->pos0 + (global.plr.focus/30.0)*e->args[0];
	e->pos += (target_pos - e->pos) * 0.5;

	MarisaLaserData *ld = REF(e->args[3]);
	complex pdelta = e->pos - ld->prev_pos;
	ld->prev_pos = e->pos;
	ld->lean += (-0.01 * creal(pdelta) - ld->lean) * 0.2;

	if(player_should_shoot(&global.plr, true)) {
		float angle = creal(e->args[2]);
		float f = smoothreclamp(global.plr.focus, 0, 30, 0, 1);
		f = smoothreclamp(f, 0, 1, 0, 1);
		float factor = (1.0 + 0.7 * psin(t/15.0)) * -(1-f) * !!angle;

		complex dir = -cexp(I*(angle*factor + ld->lean + M_PI/2));
		trace_laser(e, 5 * dir, creal(e->args[1]));

		PARTICLE(
			.size = 1+I,
			.pos = e->pos,
			.color = color_mul_scalar(RGBA(2, 0.2, 0.5, 0), 0.2),
			.rule = linear,
			.draw_rule = marisa_laser_flash_draw,
			.timeout = 8,
			.args = { dir, 0, 0.6 + 0.2*I, },
			.flags = PFLAG_NOREFLECT | PFLAG_REQUIREDPARTICLE,
			// .layer = LAYER_PARTICLE_LOW,
		);
	}

	return 1;
}

static void masterspark_visual(Enemy *e, int t2, bool render) {
	if(!render) {
		return;
	}

	float t = player_get_bomb_progress(&global.plr, NULL);
	float fade = 1;

	if(t < 1./6) {
		fade = t*6;
		fade = pow(fade, 1.0/3.0);
	}

	if(t > 4./5) {
		fade = 1-t*5 + 4;
		fade = pow(fade, 5);
	}

	marisa_common_masterspark_draw(global.plr.pos - 30 * I, 800 + I * VIEWPORT_H * 1.25, carg(e->args[0]), t2, fade);
}

static int masterspark_star(Projectile *p, int t) {
	if(t >= 0) {
		p->args[0] += 0.1*p->args[0]/cabs(p->args[0]);
		p->angle += 0.1;
	}

	return linear(p, t);
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

	e->args[0] *= cexp(I*(0.005*creal(global.plr.velocity) + nfrand() * 0.005));
	complex diroffset = e->args[0];

	float t = player_get_bomb_progress(&global.plr, NULL);

	if(t >= 3.0/4.0) {
		global.shake_view = 8 * (1 - t * 4 + 3);
	} else if(t2 % 2 == 0) {
		complex dir = -cexp(1.5*I*sin(t2*M_PI*1.12))*I;
		Color *c = HSLA(-t*5.321,1,0.5,0.5*frand());

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
			.args= { (10 * dir - 10*I)*diroffset, 4 },
			.angle = nfrand(),
			.draw_rule = GrowFade,
			.flags = flags,
		);
		dir = -conj(dir);
		PARTICLE(
			.sprite = "maristar_orbit",
			.pos = global.plr.pos+40*dir,
			.color = c,
			.rule = masterspark_star,
			.timeout = 50,
			.args = { (10 * dir - 10*I)*diroffset, 4 },
			.angle = nfrand(),
			.draw_rule = GrowFade,
			.flags = flags,
		);
		PARTICLE(
			.sprite = "smoke",
			.pos = global.plr.pos-40*I,
			.color = HSLA(2*t,1,2,0), //RGBA(0.3, 0.6, 1, 0),
			.rule = linear,
			.timeout = 50,
			.args = { -7*dir + 7*I, 6 },
			.angle = nfrand(),
			.draw_rule = GrowFade,
			.flags = flags,
		);
	}

	if(t >= 1 || !player_is_bomb_active(&global.plr)) {
		return ACTION_DESTROY;
	}

	return 1;
}

static void marisa_laser_bombbg(Player *plr) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	float t = player_get_bomb_progress(&global.plr, NULL);
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

static Enemy* marisa_laser_spawn_slave(Player *plr, complex pos, complex a0, complex a1, complex a2, complex a3) {
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
	laser_renderer = create_enemy_p(&plr->slaves, 0, ENEMY_IMMUNE, marisa_laser_renderer_visual, marisa_laser_renderer, 0, 0, 0, 0);
	laser_renderer->ent.draw_layer = LAYER_PLAYER_SHOT;
	marisa_laser_respawn_slaves(plr, plr->power);
}

static void marisa_laser_think(Player *plr) {
	assert(laser_renderer != NULL);
	assert(laser_renderer->logic_rule == marisa_laser_renderer);
	player_placeholder_bomb_logic(plr);
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
	.name = "Laser Sign",
	.character = &character_marisa,
	.dialog = &dialog_marisa,
	.shot_mode = PLR_SHOT_MARISA_LASER,
	.procs = {
		.property = marisa_laser_property,
		.bomb = marisa_laser_bomb,
		.bombbg = marisa_laser_bombbg,
		.shot = marisa_laser_shot,
		.power = marisa_laser_power,
		.preload = marisa_laser_preload,
		.init = marisa_laser_init,
		.think = marisa_laser_think,
	},
};
