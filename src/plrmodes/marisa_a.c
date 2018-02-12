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

// args are pain
static float global_magicstar_alpha;

typedef struct MarisaLaserData {
	struct {
		complex first;
		complex last;
	} trace_hit;
	complex prev_pos;
	float lean;
} MarisaLaserData;

static void draw_laser_beam(complex src, complex dst, double size, double step, double t, Texture *tex, int u_length) {
	complex dir = dst - src;
	complex center = (src + dst) * 0.5;

	render_push();

	render_translate(creal(center), cimag(center), 0);
	render_rotate_deg(180/M_PI*carg(dir), 0, 0, 1);
	render_scale(cabs(dir), size, 1);

	render_matrix_mode(MM_TEXTURE);
	render_identity();
	render_translate(-cimag(src) / step + t, 0, 0);
	render_scale(cabs(dir) / step, 1, 1);
	render_matrix_mode(MM_MODELVIEW);

	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	glUniform1f(u_length, cabs(dir) / step);
	draw_quad();

	render_matrix_mode(MM_TEXTURE);
	render_identity();
	render_matrix_mode(MM_MODELVIEW);

	render_pop();
}

static void trace_laser(Enemy *e, complex vel, int damage) {
	ProjCollisionResult col;
	Projectile *lproj = NULL;

	MarisaLaserData *ld = REF(e->args[3]);

	PROJECTILE(
		.dest = &lproj,
		.pos = e->pos,
		.size = 28*(1+I),
		.type = PlrProj + damage,
		.rule = linear,
		.args = { vel },
	);

	bool first_found = false;
	int timeofs = 0;
	int col_types = (PCOL_ENEMY | PCOL_BOSS);

	struct enemy_col {
		LIST_INTERFACE(struct enemy_col);
		Enemy *enemy;
		int original_hp;
	} *prev_collisions = NULL;

	while(lproj) {
		timeofs = trace_projectile(lproj, &col, col_types | PCOL_VOID, timeofs);
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
				.rule = timeout_linear,
				.draw_rule = Shrink,
				.args = {
					3 + 5 * afrand(2),
					(2+afrand(0)*6)*cexp(I*M_PI*2*afrand(1))
				},
				.flags = PFLAG_NOREFLECT,
			);

			if(col.type == PCOL_ENEMY) {
				c = malloc(sizeof(struct enemy_col));
				c->enemy = col.entity;
				list_push(&prev_collisions, c);
			} else {
				col_types &= ~col.type;
			}

			col.fatal = false;
		}

		apply_projectile_collision(&lproj, lproj, &col);

		if(col.type == PCOL_BOSS) {
			assert(!col.fatal);
		}

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

static float set_alpha(int u_alpha, float a) {
	if(a) {
		glUniform1f(u_alpha, a);
	}

	return a;
}

static float set_alpha_dimmed(int u_alpha, float a) {
	return set_alpha(u_alpha, a * a * 0.5);
}

static void draw_magic_star(complex pos, double a, Color c1, Color c2) {
	if(a <= 0) {
		return;
	}

	Color mul = rgba(1, 1, 1, a);
	c1 = multiply_colors(c1, mul);
	c2 = multiply_colors(c2, mul);

	Sprite *spr = get_sprite("part/magic_star");
	Shader *shader = recolor_get_shader();
	ColorTransform ct;
	glUseProgram(shader->prog);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	render_push();
		render_translate(creal(pos), cimag(pos), -1);
		render_push();
			static_clrtransform_bullet(c1, &ct);
			recolor_apply_transform(&ct);
			render_rotate_deg(global.frames * 3, 0, 0, 1);
			draw_sprite_p(0, 0, spr);
		render_pop();
		render_push();
			static_clrtransform_bullet(c2, &ct);
			recolor_apply_transform(&ct);
			render_rotate_deg(global.frames * -3, 0, 0, 1);
			draw_sprite_p(0, 0, spr);
		render_pop();
	render_pop();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(0);
}

static void marisa_laser_slave_visual(Enemy *e, int t, bool render) {
	if(!render) {
		marisa_common_slave_visual(e, t, render);
		return;
	}

	float laser_alpha = global.plr.slaves->args[0];
	float star_alpha = global.plr.slaves->args[1] * global_magicstar_alpha;

	draw_magic_star(e->pos, 0.75 * star_alpha,
		rgb(1.0, 0.1, 0.1),
		rgb(0.0, 0.1, 1.1)
	);

	marisa_common_slave_visual(e, t, render);

	if(laser_alpha <= 0) {
		return;
	}

	MarisaLaserData *ld = REF(e->args[3]);

	render_color4(1, 1, 1, laser_alpha);
	render_push();
	render_translate(creal(ld->trace_hit.first), cimag(ld->trace_hit.first), 0);
	draw_sprite(0, 0, "part/smoothdot");
	render_pop();
	render_color4(1, 1, 1, 1);
}

static void marisa_laser_fader_visual(Enemy *e, int t, bool render) {
}

static float get_laser_alpha(Enemy *e, float a) {
	if(e->visual_rule == marisa_laser_fader_visual) {
		return e->args[1];
	}

	return min(a, min(1, (global.frames - e->birthtime) * 0.1));
}

#define FOR_EACH_SLAVE(e) for(Enemy *e = global.plr.slaves; e; e = e->next) if(e != renderer)
#define FOR_EACH_REAL_SLAVE(e) FOR_EACH_SLAVE(e) if(e->visual_rule == marisa_laser_slave_visual)

static void marisa_laser_renderer_visual(Enemy *renderer, int t, bool render) {
	if(!render) {
		return;
	}

	double a = creal(renderer->args[0]);
	Shader *shader = get_shader("marisa_laser");
	int u_clr0 = uniloc(shader, "color0");
	int u_clr1 = uniloc(shader, "color1");
	int u_clr_phase = uniloc(shader, "color_phase");
	int u_clr_freq = uniloc(shader, "color_freq");
	int u_alpha = uniloc(shader, "alphamod");
	int u_length = uniloc(shader, "length");
	// int u_cutoff = uniloc(shader, "cutoff");
	Texture *tex0 = get_tex("part/marisa_laser0");
	Texture *tex1 = get_tex("part/marisa_laser1");

	glUseProgram(shader->prog);
	glUniform4f(u_clr0, 1, 1, 1, 0.5);
	glUniform4f(u_clr1, 1, 1, 1, 0.8);
	glUniform1f(u_clr_phase, -1.5 * t/M_PI);
	glUniform1f(u_clr_freq, 10.0);
	glBlendFunc(GL_SRC_COLOR, GL_ONE);
	glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo_pairs.rgba.front->fbo);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0, 0, 0, 1);
	glBlendEquation(GL_MAX);

	FOR_EACH_SLAVE(e) {
		if(set_alpha(u_alpha, get_laser_alpha(e, a))) {
			MarisaLaserData *ld = REF(e->args[3]);
			draw_laser_beam(e->pos, ld->trace_hit.last, 32, 128, -0.02 * t, tex1, u_length);
		}
	}

	glBlendEquation(GL_FUNC_ADD);
	glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo_pairs.fg.back->fbo);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(0);
	draw_fbo_viewport(resources.fbo_pairs.rgba.front);
	glUseProgram(shader->prog);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glUniform4f(u_clr0, 1.0, 0.0, 0.0, 0.5);
	glUniform4f(u_clr1, 1.0, 0.0, 0.0, 1.0);

	FOR_EACH_SLAVE(e) {
		if(set_alpha_dimmed(u_alpha, get_laser_alpha(e, a))) {
			MarisaLaserData *ld = REF(e->args[3]);
			draw_laser_beam(e->pos, ld->trace_hit.first, 40, 128, t * -0.12, tex0, u_length);
		}
	}

	glUniform4f(u_clr0, 0.1, 0.5, 1.0, 2.0);
	glUniform4f(u_clr1, 0.1, 0.1, 1.0, 1.0);

	FOR_EACH_SLAVE(e) {
		if(set_alpha_dimmed(u_alpha, get_laser_alpha(e, a))) {
			MarisaLaserData *ld = REF(e->args[3]);
			draw_laser_beam(e->pos, ld->trace_hit.first, 42, 200, t * -0.03, tex0, u_length);
		}
	}

	glUseProgram(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

static int marisa_laser_slave(Enemy *e, int t) {
	if(t == EVENT_BIRTH) {
		return 1;
	}

	if(t == EVENT_DEATH && !global.game_over && global.plr.slaves && creal(global.plr.slaves->args[0])) {
		spawn_laser_fader(e, global.plr.slaves->args[0]);

		MarisaLaserData *ld = REF(e->args[3]);
		free(ld);
		free_ref(e->args[3]);
		return 1;
	}

	if(t < 0) {
		return 1;
	}

	e->pos = global.plr.pos + (1 - global.plr.focus/30.0)*e->pos0 + (global.plr.focus/30.0)*e->args[0];

	MarisaLaserData *ld = REF(e->args[3]);
	complex pdelta = e->pos - ld->prev_pos;
	ld->prev_pos = e->pos;
	ld->lean += (-0.01 * creal(pdelta) - ld->lean) * 0.2;

	if(player_should_shoot(&global.plr, true)) {
		float angle = creal(e->args[2]);
		float f = smoothreclamp(global.plr.focus, 0, 30, 0, 1);
		f = smoothreclamp(f, 0, 1, 0, 1);
		float factor = (1.0 + 0.7 * psin(t/15.0)) * -(1-f) * !!angle;

		trace_laser(e, -5 * cexp(I*(angle*factor + ld->lean + M_PI/2)), creal(e->args[1]));
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
	fade = sqrt(fade);
	}

	if(t > 3./4) {
		fade = 1-t*4 + 3;
	fade *= fade;
	}

	render_push();
	render_translate(creal(global.plr.pos),cimag(global.plr.pos)-40-VIEWPORT_H/2,0);
	render_scale(fade*800,VIEWPORT_H,1);
	marisa_common_masterspark_draw(t*BOMB_RECOVERY);
	render_pop();
}

static int masterspark_star(Projectile *p, int t) {
	p->args[1] += 0.1*p->args[1]/cabs(p->args[1]);
	return timeout_linear(p,t);
}

static int masterspark(Enemy *e, int t2) {
	if(t2 == EVENT_BIRTH) {
		global.shake_view = 8;
		return 1;
	} else if(t2 == EVENT_DEATH) {
		global.shake_view=0;
		return 1;
	}

	if(t2 < 0)
		return 1;

	float t = player_get_bomb_progress(&global.plr, NULL);
	if(t2%2==0 && t < 3./4) {
		complex dir = -cexp(1.2*I*nfrand())*I;
		Color c = rgb(0.7+0.3*sin(t*30),0.7+0.3*cos(t*30),0.7+0.3*cos(t*3));
		PARTICLE(
			.sprite="maristar_orbit",
			.pos=global.plr.pos+40*dir,
			.color=c,
			.rule=masterspark_star,
			.args={50, 10*dir-10*I,3},
			.angle=nfrand(),
			.flags=PFLAG_DRAWADD,
			.draw_rule=GrowFade
		);
		dir = -conj(dir);
		PARTICLE(
			.sprite="maristar_orbit",
			.pos=global.plr.pos+40*dir,
			.color=c,
			.rule=masterspark_star,
			.args={50, 10*dir-10*I,3},
			.angle=nfrand(),
			.flags=PFLAG_DRAWADD,
			.draw_rule=GrowFade
		);
		PARTICLE(
			.sprite="smoke",
			.pos=global.plr.pos-40*I,
			.color=rgb(0.9,1,1),
			.rule=timeout_linear,
			.args={50, -5*dir,3},
			.angle=nfrand(),
			.flags=PFLAG_DRAWADD,
			.draw_rule=GrowFade
		);
	}

	if(t >= 1 || global.frames - global.plr.recovery >= 0) {
		return ACTION_DESTROY;
	}

	return 1;
}

static void marisa_laser_bombbg(Player *plr) {
	float t = player_get_bomb_progress(&global.plr, NULL);
	float fade = 1;

	if(t < 1./6)
		fade = t*6;

	if(t > 3./4)
		fade = 1-t*4 + 3;

	render_color4(1,1,1,0.8*fade);
	fill_viewport(sin(t*0.3),t*3*(1+t*3),1,"marisa_bombbg");
	render_color4(1,1,1,1);
}

static void marisa_laser_bomb(Player *plr) {
	play_sound("bomb_marisa_a");
	create_enemy_p(&plr->slaves, 40.0*I, ENEMY_BOMB, masterspark_visual, masterspark, 280,0,0,0);
}

static void marisa_laser_respawn_slaves(Player *plr, short npow) {
	Enemy *e = plr->slaves, *tmp;
	double dmg = 8;

	while(e != 0) {
		tmp = e;
		e = e->next;
		if(tmp->logic_rule == marisa_laser_slave) {
			delete_enemy(&plr->slaves, tmp);
		}
	}

	if(npow / 100 == 1) {
		create_enemy_p(&plr->slaves, -40.0*I, ENEMY_IMMUNE, marisa_laser_slave_visual, marisa_laser_slave, -40.0*I, dmg, 0, 0);
	}

	if(npow >= 200) {
		create_enemy_p(&plr->slaves, 25-5.0*I, ENEMY_IMMUNE, marisa_laser_slave_visual, marisa_laser_slave, 8-40.0*I, dmg,   -M_PI/30, 0);
		create_enemy_p(&plr->slaves, -25-5.0*I, ENEMY_IMMUNE, marisa_laser_slave_visual, marisa_laser_slave, -8-40.0*I, dmg,  M_PI/30, 0);
	}

	if(npow / 100 == 3) {
		create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, marisa_laser_slave_visual, marisa_laser_slave, -50.0*I, dmg, 0, 0);
	}

	if(npow >= 400) {
		create_enemy_p(&plr->slaves, 17-30.0*I, ENEMY_IMMUNE, marisa_laser_slave_visual, marisa_laser_slave, 4-45.0*I, dmg, M_PI/60, 0);
		create_enemy_p(&plr->slaves, -17-30.0*I, ENEMY_IMMUNE, marisa_laser_slave_visual, marisa_laser_slave, -4-45.0*I, dmg, -M_PI/60, 0);
	}

	for(e = plr->slaves; e; e = e->next) {
		if(e->logic_rule == marisa_laser_slave) {
			MarisaLaserData *ld = calloc(1, sizeof(MarisaLaserData));
			ld->prev_pos = e->pos + plr->pos;
			e->args[3] = add_ref(ld);
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
	create_enemy_p(&plr->slaves, 0, ENEMY_IMMUNE, marisa_laser_renderer_visual, marisa_laser_renderer, 0, 0, 0, 0);
	marisa_laser_respawn_slaves(plr, plr->power);
}

static void marisa_laser_think(Player *plr) {
	Enemy *laser_renderer = plr->slaves;
	assert(laser_renderer != NULL);
	assert(laser_renderer->logic_rule == marisa_laser_renderer);

	if(creal(laser_renderer->args[0]) > 0) {
		bool found = false;

		for(Projectile *p = global.projs; p && !found; p = p->next) {
			if(p->type != EnemyProj) {
				continue;
			}

			for(Enemy *slave = laser_renderer->next; slave; slave = slave->next) {
				if(slave->visual_rule == marisa_laser_slave_visual && cabs(slave->pos - p->pos) < 30) {
					found = true;
					break;
				}
			}
		}

		if(found) {
			global_magicstar_alpha = approach(global_magicstar_alpha, 0.25, 0.15);
		} else {
			global_magicstar_alpha = approach(global_magicstar_alpha, 1.00, 0.08);
		}
	} else {
		global_magicstar_alpha = 1.0;
	}
}

static double marisa_laser_speed_mod(Player *plr, double speed) {
	if(global.frames - plr->recovery < 0) {
		speed /= 5.0;
	}

	return speed;
}

static void marisa_laser_shot(Player *plr) {
	marisa_common_shot(plr, 175 - 4 * (plr->power / 100));
}

static void marisa_laser_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"proj/marisa",
		"part/maristar_orbit",
		"part/magic_star",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"marisa_bombbg",
		"part/marisa_laser0",
		"part/marisa_laser1",
	NULL);

	preload_resources(RES_SHADER, flags,
		"marisa_laser",
		"masterspark",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
		"bomb_marisa_a",
	NULL);
}

PlayerMode plrmode_marisa_a = {
	.name = "Laser Sign",
	.character = &character_marisa,
	.shot_mode = PLR_SHOT_MARISA_LASER,
	.procs = {
		.bomb = marisa_laser_bomb,
	.bombbg = marisa_laser_bombbg,
		.shot = marisa_laser_shot,
		.power = marisa_laser_power,
		.speed_mod = marisa_laser_speed_mod,
		.preload = marisa_laser_preload,
		.init = marisa_laser_init,
		.think = marisa_laser_think,
	},
};
