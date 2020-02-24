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

static int marisa_star_projectile(Projectile *p, int t) {
	if(t == EVENT_BIRTH) {
		return ACTION_ACK;
	}

	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
		return ACTION_ACK;
	}

	Enemy *e = NULL;

	if(creal(p->args[1]) >= 0) {
		e = (Enemy*)REF(p->args[1]);
	}

	if(e != NULL && !player_should_shoot(&global.plr, true)) {
		free_ref(p->args[1]);
		p->args[1] = -1;
		e = NULL;
	}

	if(e == NULL) {
		p->pos += p->pos0;
		p->pos0 *= 1.005;
		p->angle += 0.1;
		return ACTION_NONE;
	}

	//float c = 0.3 * psin(t * 0.2);
	//p->color = *RGB(1 - c, 0.7 + 0.3 * psin(t * 0.1), 0.9 + c/3);


	float freq = 0.1;

	double focus = 1 - abs(global.plr.focus) / 30.0;

	double focusfac = 1;
	if(focus > 0) {
		focusfac = t*0.015-1/focus;
		focusfac = tanh(sqrt(fabs(focusfac)));
	}

	double centerfac = tanh(t/10.); // interpolate between player center and slave center
	cmplx center = global.plr.pos*(1-centerfac) + e->args[2]*centerfac;

	double brightener = -1/(1+sqrt(0.03*fabs(creal(p->pos-center))));
	p->color = *RGBA(0.3+(1-focus)*0.7+brightener,0.8+brightener,1.0-(1-focus)*0.7+brightener,0.2+brightener);

	double verticalfac = - 5*t*(1+0.01*t) + 10*t/(0.01*t+1);
	p->pos0 = p->pos;
	p->pos = center + focusfac*cbrt(0.1*t)*creal(p->args[0])* 70 * sin(freq*t+cimag(p->args[0])) + I*verticalfac;
	p->pos0 = p->pos - p->pos0;
	p->angle = carg(p->pos0);

	if(t%(2+(int)round(2*rng_real())) == 0) {  // please never write stuff like this ever again
		PARTICLE(
			.sprite = "stardust",
			.pos = p->pos,
			.color = RGBA(0.5*(1-focus),0,0.5*focus,0),
			.timeout = 5,
			.angle = t*0.1,
			.draw_rule = pdraw_timeout_scalefade(0, 1.4, 1, 0),
			.flags = PFLAG_NOREFLECT,
		);
	}

	return ACTION_NONE;
}

static int marisa_star_slave(Enemy *e, int t) {
	for(int i = 0; i < 2; ++i) {
		if(player_should_shoot(&global.plr, true) && !((global.frames+2*i) % 5)) {
			float fac = e->args[0]/M_PI/2;
			cmplx v = (1-2*i);
			v = creal(v)/cabs(v);
			v *= 1-0.9*fac;
			v -= I*0.04*t*(4-3*fac);

			//v *= cexp(I*i*M_PI/20*sign(v));

			PROJECTILE(
				.proto = pp_maristar,
				.pos = e->pos,
				.color = RGBA(0.3,0.8,1.0,0.6),
				.rule = marisa_star_projectile,
				.args = { v, add_ref(e) },
				.type = PROJ_PLAYER,
				.damage = e->args[3],
				.shader = "sprite_default",
			);
		}
	}

	double angle = 0.05*global.frames+e->args[0];
	if(cos(angle) < 0) {
		e->ent.draw_layer = LAYER_BACKGROUND;
	} else {
		e->ent.draw_layer = LAYER_PLAYER_SLAVE;
	}

	cmplx d = global.plr.pos-e->args[2];
	e->args[2] += (2+2*!global.plr.focus)*d/(cabs(d)+2);
	e->pos = e->args[2] + 80*sin(angle)+45*I;

	return 1;
}


static int marisa_star_orbit_star(Projectile *p, int t) { // XXX: because growfade is the worst
	if(t >= 0) {
		p->args[0] += p->args[0]/cabs(p->args[0])*0.15;
		p->angle += 0.1;
	}

	return linear(p,t);
}

static Color marisa_slaveclr(int i, float low) {
	int numslaves = 5;
	assert(i >= 0 && i < numslaves);

	return *HSLA(i/(float)numslaves, 1, low, 1);
}

static int marisa_star_orbit(Enemy *e, int t) {
	Color color = marisa_slaveclr(rint(creal(e->args[0])), 0.6);

	float tb = player_get_bomb_progress(&global.plr);
	if(t == EVENT_BIRTH) {
		global.shake_view = 8;
		return 1;
	} else if(t == EVENT_DEATH) {
		global.shake_view = 0;
	}
	if(t < 0) {
		return 1;
	}

	if(tb >= 1 || !player_is_bomb_active(&global.plr)) {
		return ACTION_DESTROY;
	}

	double r = 100*pow(tanh(t/20.),2);
	cmplx dir = e->args[1]*r*cexp(I*(sqrt(1000+t*t+0.03*t*t*t))*0.04);
	e->pos = global.plr.pos+dir;

	float fadetime = 3./4;

	if(tb >= fadetime) {
		color.a = 1 - (tb - fadetime) / (1 - fadetime);
	}

	color_mul_alpha(&color);
	color.a = 0;

	if(t%1 == 0) {
		Color *color2 = COLOR_COPY(&color);
		color_mul_scalar(color2, 0.5);

		PARTICLE(
			.sprite_ptr = get_sprite("part/maristar_orbit"),
			.pos = e->pos,
			.color = color2,
			.timeout = 10,
			.angle = t*0.1,
			.draw_rule = pdraw_timeout_scalefade(0, 1 + 4 * tb, 1, 0),
			.flags = PFLAG_NOREFLECT,
		);
	}

	if(t%(10-t/30) == 0 && tb < fadetime) {
		PARTICLE(
			.sprite = "maristar_orbit",
			.pos = e->pos,
			.color = &color,
			.rule = marisa_star_orbit_star,
			.draw_rule = pdraw_timeout_scalefade(0, 6, 1, 0),
			.timeout = 150,
			.flags = PFLAG_NOREFLECT,
			.args = { -5*dir/cabs(dir) },
		);
	}

	return 1;
}


#define NUM_MARISTAR_SLAVES 5
static void marisa_star_orbit_visual(Enemy *e, int t, bool render) {
	if(!render) {
		return;
	}

	float tb = player_get_bomb_progress(&global.plr);
	Color color = marisa_slaveclr(rint(creal(e->args[0])), 0.2);

	float fade = 1;

	if(tb < 1./6) {
		fade = tb*6;
		fade = sqrt(fade);
	}

	if(tb > 3./4) {
		fade = 1-tb*4 + 3;
		fade *= fade;
	}

	color_mul_scalar(&color, fade);

	if(e->args[0] == 0) {
		MarisaBeamInfo beams[NUM_MARISTAR_SLAVES];
		for(int i = 0; i < NUM_MARISTAR_SLAVES; i++) {
			beams[i].origin = global.plr.pos + (e->pos-global.plr.pos)*cexp(I*2*M_PI/NUM_MARISTAR_SLAVES*i);
			beams[i].size = 250*fade+VIEWPORT_H*1.5*I;
			beams[i].angle = carg(e->pos - global.plr.pos) + M_PI/2 + 2*M_PI/NUM_MARISTAR_SLAVES*i;
			beams[i].t = global.plr.bombtotaltime * tb;
		}
		marisa_common_masterspark_draw(NUM_MARISTAR_SLAVES, beams, fade);
	}

	color.a = 0;

	SpriteParams sp = { 0 };
	sp.pos.x = creal(e->pos);
	sp.pos.y = cimag(e->pos);
	sp.color = &color;
	sp.rotation = (SpriteRotationParams) {
		.angle = t * 10 * DEG2RAD,
		.vector = { 0, 0, 1 },
	};
	sp.sprite = "fairy_circle";
	r_draw_sprite(&sp);
	sp.sprite = "part/lightningball";
	sp.scale.both = 0.6;
	r_draw_sprite(&sp);
}

static void marisa_star_bomb(Player *plr) {
	play_sound("bomb_marisa_b");

	for(int i = 0; i < NUM_MARISTAR_SLAVES; i++) {
		cmplx dir = cexp(2*I*M_PI/NUM_MARISTAR_SLAVES*i);
		Enemy *e = create_enemy2c(plr->pos, ENEMY_BOMB, marisa_star_orbit_visual, marisa_star_orbit, i ,dir);
		e->ent.draw_layer = LAYER_PLAYER_FOCUS - 1;
	}
}
#undef NUM_MARISTAR_SLAVES

static void marisa_star_bombbg(Player *plr) {
	if(!player_is_bomb_active(plr)) {
		return;
	}

	float t = player_get_bomb_progress(&global.plr);

	ShaderProgram *s = r_shader_get("maristar_bombbg");
	r_shader_ptr(s);
	r_uniform_float("t", t);
	r_uniform_float("decay", 1);
	r_uniform_vec2("plrpos", creal(global.plr.pos)/VIEWPORT_W, cimag(global.plr.pos)/VIEWPORT_H);
	fill_viewport(0,0,1,"marisa_bombbg");
	r_shader_standard();
}

static void marisa_star_respawn_slaves(Player *plr, short npow) {
	double dmg = 56;

	for(Enemy *e = plr->slaves.first, *next; e; e = next) {
		next = e->next;

		if(e->hp == ENEMY_IMMUNE) {
			delete_enemy(&plr->slaves, e);
		}
	}


	int numslaves = npow/100;
	dmg /= sqrt(numslaves);
	for(int i = 0; i < numslaves; i++) {
		create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, 2*M_PI*i/numslaves, 0, global.plr.pos, dmg);
	}

	for(Enemy *e = plr->slaves.first; e; e = e->next) {
		e->ent.draw_layer = LAYER_PLAYER_SLAVE;
	}
}

static void marisa_star_power(Player *plr, short npow) {
	if(plr->power / 100 == npow / 100) {
		return;
	}

	marisa_star_respawn_slaves(plr, npow);
}

static void marisa_star_init(Player *plr) {
	marisa_star_respawn_slaves(plr, plr->power);
}

static void marisa_star_shot(Player *plr) {
	int p = plr->power / 100;
	marisa_common_shot(plr, 175 - 10*p);
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

static void marisa_star_preload(void) {
	const int flags = RESF_DEFAULT;

	preload_resources(RES_SPRITE, flags,
		"hakkero",
		"masterspark_ring",
		"part/maristar_orbit",
		"part/stardust",
		"proj/marisa",
		"proj/maristar",
	NULL);

	preload_resources(RES_TEXTURE, flags,
		"marisa_bombbg",
	NULL);

	preload_resources(RES_SHADER_PROGRAM, flags,
		"masterspark",
		"maristar_bombbg",
		"sprite_hakkero",
	NULL);

	preload_resources(RES_SFX, flags | RESF_OPTIONAL,
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
		.property = marisa_star_property,
		.bomb = marisa_star_bomb,
		.bombbg = marisa_star_bombbg,
		.shot = marisa_star_shot,
		.power = marisa_star_power,
		.preload = marisa_star_preload,
		.init = marisa_star_init,
		.think = player_placeholder_bomb_logic,
	},
};
