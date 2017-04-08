/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <float.h>

#include "plrmodes.h"
#include "player.h"
#include "global.h"
#include "stage.h"

static bool should_shoot(bool extra) {
	return (global.plr.inputflags & INFLAG_SHOT) &&
			(!extra || (global.frames - global.plr.recovery >= 0 && global.plr.deathtime >= -1));
}

/* Yōmu */

// Haunting Sign

complex youmu_homing_target(complex org, complex fallback) {
	double mindst = DBL_MAX;
	complex target = fallback;

	if(global.boss) {
		target = global.boss->pos;
		mindst = cabs(target - org);
	}

	for(Enemy *e = global.enemies; e; e = e->next) {
		if(e->hp == ENEMY_IMMUNE){
			continue;
		}

		double dst = cabs(e->pos - org);

		if(dst < mindst) {
			mindst = dst;
			target = e->pos;
		}
	}

	return target;
}

void youmu_homing_draw_common(Projectile *p, int t, float clrfactor, float alpha) {
	glColor4f(0.7f + 0.3f * clrfactor, 0.9f + 0.1f * clrfactor, 1, alpha);
	ProjDraw(p, t);
	glColor4f(1, 1, 1, 1);
}

void youmu_homing_draw_proj(Projectile *p, int t) {
	float a = clamp(1.0f - (float)t / p->args[2], 0, 1);
	youmu_homing_draw_common(p, t, a, 0.5f);
}

void youmu_homing_draw_trail(Projectile *p, int t) {
	float a = clamp(1.0f - (float)t / p->args[0], 0, 1);
	youmu_homing_draw_common(p, t, a, 0.15f * a);
}

void youmu_homing_trail(Projectile *p, complex v, int to) {
	Projectile *trail = create_projectile_p(&global.particles, p->tex, p->pos, 0, youmu_homing_draw_trail, timeout_linear, to, v, 0, 0);
	trail->type = PlrProj;
	trail->angle = p->angle;
}

int youmu_homing(Projectile *p, int t) { // a[0]: velocity, a[1]: aim (r: linear, i: accelerated), a[2]: timeout, a[3]: initial target
	if(t == EVENT_DEATH) {
		return 1;
	}

	if(t > creal(p->args[2])) {
		return ACTION_DESTROY;
	}

	p->args[3] = youmu_homing_target(p->pos, p->args[3]);

	double v = cabs(p->args[0]);
	complex aimdir = cexp(I*carg(p->args[3] - p->pos));

	p->args[0] += creal(p->args[1]) * aimdir;
	p->args[0] = v * cexp(I*carg(p->args[0])) + cimag(p->args[1]) * aimdir;

	p->angle = carg(p->args[0]);
	p->pos += p->args[0];

	youmu_homing_trail(p, 0.5 * p->args[0], 12);
	return 1;
}

int youmu_trap(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		create_particle1c("blast", p->pos, 0, Blast, timeout, 15);
		return 1;
	}

	double expiretime = creal(p->args[1]);

	if(t > expiretime) {
		return ACTION_DESTROY;
	}

	if(!(global.plr.inputflags & INFLAG_FOCUS)) {
		create_particle1c("blast", p->pos, 0, Blast, timeout, 20);
		create_particle1c("blast", p->pos, 0, Blast, timeout, 23);

		int cnt = creal(p->args[2]);
		int dmg = cimag(p->args[2]);
		int dur = 45 + 10 * nfrand(); // creal(p->args[3]) + nfrand() * cimag(p->args[3]);
		complex aim = p->args[3];

		for(int i = 0; i < cnt; ++i) {
			float a = (i / (float)cnt) * M_PI * 2;
			complex dir = cexp(I*(a));
			Projectile *proj = create_projectile4c("hghost", p->pos, 0, youmu_homing, 5 * dir, aim, dur, global.plr.pos);
			proj->type = PlrProj + dmg;
			proj->draw = youmu_homing_draw_proj;
		}

		return ACTION_DESTROY;
	}

	p->angle = global.frames + t;
	p->pos += p->args[0] * (0.01 + 0.99 * max(0, (10 - t) / 10.0));

	youmu_homing_trail(p, cexp(I*p->angle), 30);
	return 1;
}

void Slice(Projectile *p, int t) {
	if(t < creal(p->args[0])/20.0)
		p->args[1] += 1;

	if(t > creal(p->args[0])-10) {
		p->args[1] += 3;
		p->args[2] += 1;
	}

	float f = p->args[1]/p->args[0]*20.0;

	glColor4f(1,1,1,1.0 - p->args[2]/p->args[0]*20.0);

	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos),0);
	glRotatef(p->angle,0,0,1);
	glScalef(f,1,1);
	draw_texture(0,0,"part/youmu_slice");

	glPopMatrix();

	glColor4f(1,1,1,1);
}

void YoumuSlash(Enemy *e, int t) {
	fade_out(10.0/t+sin(t/10.0)*0.1);

}

int spin(Projectile *p, int t) {
	int i = timeout_linear(p, t);

	if(t < 0)
		return 1;

	p->args[3] += 0.06;
	p->angle = p->args[3];

	return i;
}

int youmu_slash(Enemy *e, int t) {
	if(t > creal(e->args[0]))
		return ACTION_DESTROY;
	if(t < 0)
		return 1;

	TIMER(&t);

	AT(0)
		global.plr.pos = VIEWPORT_W/5.0 + (VIEWPORT_H - 100)*I;

	FROM_TO(8,20,1)
		global.plr.pos = VIEWPORT_W + (VIEWPORT_H - 100)*I - exp(-_i/8.0+log(4*VIEWPORT_W/5.0));

	FROM_TO(30, 60, 10) {
		tsrand_fill(3);
		create_particle1c("youmu_slice", VIEWPORT_W/2.0 - 150 + 100*_i + VIEWPORT_H/2.0*I - 10-10.0*I + 20*afrand(0)+20.0*I*afrand(1), 0, Slice, timeout, 200)->angle = -10.0+20.0*afrand(2);
	}

	FROM_TO(40,200,1)
		if(frand() > 0.7) {
			tsrand_fill(6);
			create_particle2c("blast", VIEWPORT_W*afrand(0) + (VIEWPORT_H+50)*I, rgb(afrand(1),afrand(2),afrand(3)), Shrink, timeout_linear, 80, 3*(1-2.0*afrand(4))-14.0*I+afrand(5)*2.0*I);
		}

	int tpar = 30;
	if(t < 30)
		tpar = t;

	if(t < creal(e->args[0])-60 && frand() > 0.2) {
		tsrand_fill(3);
		create_particle2c("smoke", VIEWPORT_W*afrand(0) + (VIEWPORT_H+100)*I, rgba(0.4,0.4,0.4,afrand(1)*0.2 - 0.2 + 0.6*(tpar/30.0)), PartDraw, spin, 300, -7.0*I+afrand(2)*1.0*I);
	}
	return 1;
}

// Opposite Sign

int myon_particle_rule(Projectile *p, int t) {
	float a = 1.0;

	if(t > 0) {
		double mt = creal(p->args[0]);
		a *= (mt - t) / mt;
	}

	p->clr = mix_colors(
		rgba(0.85, 0.9, 1.0, 1.0),
		rgba(0.5, 0.7, 1.0, a),
	1 - pow(1 - a, 1 + psin((t + global.frames) * 0.1)));

	return timeout_linear(p, t);
}

void myon_particle_draw(Projectile *p, int t) {
	// hack to bypass the bullet color shader
	Color clr = p->clr;
	p->clr = 0;
	parse_color_call(clr, glColor4f);
	Shrink(p, t);
	glColor4f(1, 1, 1, 1);
	p->clr = clr;
}

void YoumuOppositeMyon(Enemy *e, int t) {
	float a = global.frames * 0.07;
	complex pos = e->pos + 3 * (cos(a) + I * sin(a));

	complex dir = cexp(I*(0.1 * sin(global.frames * 0.05) + carg(global.plr.pos - e->pos)));
	double v = 4 * cabs(global.plr.pos - e->pos) / (VIEWPORT_W * 0.5);

	create_particle2c("flare", pos, 0, myon_particle_draw, myon_particle_rule, 20, v*dir)->type = PlrProj;
}

static void youmu_opposite_myon_proj(char *tex, complex pos, double speed, double angle, double aoffs, double upfactor, int dmg) {
	complex dir = cexp(I*(M_PI/2 + aoffs)) * upfactor + cexp(I * (angle + aoffs)) * (1 - upfactor);
	dir = dir / cabs(dir);
	create_projectile1c(tex, pos, 0, linear, speed*dir)->type = PlrProj+dmg;
}

int youmu_opposite_myon(Enemy *e, int t) {
	if(t == EVENT_BIRTH)
		e->pos = e->pos0 + global.plr.pos;
	if(t < 0)
		return 1;

	Player *plr = &global.plr;
	float arg = carg(e->pos0);
	float rad = cabs(e->pos0);

	double nfocus = plr->focus / 30.0;

	if(!(plr->inputflags & INFLAG_FOCUS)) {
		if(plr->inputflags && !creal(e->args[0]))
			arg -= (carg(e->pos0)-carg(e->pos-plr->pos))*2;

		//if(global.frames - plr->prevmovetime <= 10 && global.frames == plr->movetime) {
		if(global.frames == plr->movetime) {
			int new = plr->curmove;
			int old = plr->prevmove;

			if(new == INFLAG_UP && old == INFLAG_DOWN) {
				arg = M_PI/2;
				e->args[0] = plr->movetime;
			} else if(new == INFLAG_DOWN && old == INFLAG_UP) {
				arg = 3*M_PI/2;
				e->args[0] = plr->movetime;
			} else if(new == INFLAG_LEFT && old == INFLAG_RIGHT) {
				arg = 0;
				e->args[0] = plr->movetime;
			} else if(new == INFLAG_RIGHT && old == INFLAG_LEFT) {
				arg = M_PI;
				e->args[0] = plr->movetime;
			}
		}
	}

	if(creal(e->args[0]) && plr->movetime != creal(e->args[0]))
		e->args[0] = 0;

	e->pos0 = rad * cexp(I*arg);
	complex target = plr->pos + e->pos0;
	e->pos += cexp(I*carg(target - e->pos)) * min(10, 0.07 * max(0, cabs(target - e->pos) - VIEWPORT_W * 0.5 * nfocus));

	if(should_shoot(true) && !(global.frames % 6) && global.plr.deathtime >= -1) {
		int v1 = -21;
		int v2 = -10;

		double r1 = (psin(global.frames * 2) * 0.5 + 0.5) * 0.1;
		double r2 = (psin(global.frames * 1.2) * 0.5 + 0.5) * 0.1;

		double a = carg(-e->pos0);
		double u = 1 - nfocus;

		int p = plr->power / 100;
		int dmg_center = 160 - 30 * p;
		int dmg_side = 23 + 2 * p;

		if(plr->power >= 100) {
			youmu_opposite_myon_proj("youmu",  e->pos, v1, a,  r1*1, u, dmg_side);
			youmu_opposite_myon_proj("youmu",  e->pos, v1, a, -r1*1, u, dmg_side);
		}

		if(plr->power >= 200) {
			youmu_opposite_myon_proj("hghost", e->pos, v2, a,  r2*2, 0, dmg_side);
			youmu_opposite_myon_proj("hghost", e->pos, v2, a, -r2*2, 0, dmg_side);
		}

		if(plr->power >= 300) {
			youmu_opposite_myon_proj("youmu",  e->pos, v1, a,  r1*3, u, dmg_side);
			youmu_opposite_myon_proj("youmu",  e->pos, v1, a, -r1*3, u, dmg_side);
		}

		if(plr->power >= 400) {
			youmu_opposite_myon_proj("hghost", e->pos, v2, a,  r2*4, 0, dmg_side);
			youmu_opposite_myon_proj("hghost", e->pos, v2, a, -r2*4, 0, dmg_side);
		}

		youmu_opposite_myon_proj("hghost", e->pos, v2, a, 0, 0, dmg_center);
	}

	return 1;
}

int youmu_split(Enemy *e, int t) {
	if(t < 0)
		return 1;

	if(t > creal(e->args[0]))
		return ACTION_DESTROY;

	TIMER(&t);

	FROM_TO(30,200,1) {
		tsrand_fill(2);
		create_particle2c("smoke", VIEWPORT_W/2 + VIEWPORT_H/2*I, rgba(0.4,0.4,0.4,afrand(0)*0.2+0.4), PartDraw, spin, 300, 6*cexp(I*afrand(1)*2*M_PI));
	}

	FROM_TO(100,170,10) {
		tsrand_fill(3);
		create_particle1c("youmu_slice", VIEWPORT_W/2.0 + VIEWPORT_H/2.0*I - 200-200.0*I + 400*afrand(0)+400.0*I*afrand(1), 0, Slice, timeout, 100-_i)->angle = 360.0*afrand(2);
	}


	FROM_TO(0, 220, 1) {
		float talt = atan((t-e->args[0]/2)/30.0)*10+atan(-e->args[0]/2);
		global.plr.pos = VIEWPORT_W/2.0 + (VIEWPORT_H-80)*I + VIEWPORT_W/3.0*sin(talt);
		global.plr.moving = true;
		global.plr.dir = 1.0/(pow(e->args[0]/2-t,2) + 1)*cos(talt) < 0;
	}

	return 1;
}

// Youmu Generic

void youmu_homing_power_shot(Player *plr, int p) {
	int d = -2;
	double spread = 4;
	complex aim = (0.5 + 0.1 * p) + (0.1 - p * 0.025) * I;
	double speed = 10;

	if(plr->power / 100 < p || (global.frames + d * p) % 12) {
		return;
	}

	Projectile **dst = &global.projs;
	Texture *t = get_tex("proj/hghost");

	for(int sign = -1; sign < 2; sign += 2) {
		create_projectile_p(dst, t, plr->pos, 0, youmu_homing_draw_proj, youmu_homing,
			speed * cexp(I*carg(sign*p*spread-speed*I)), aim, 60, VIEWPORT_W*0.5)->type = PlrProj+54;
	}
}

void youmu_shot(Player *plr) {
	if(should_shoot(false)) {
		if(!(global.frames % 4))
			play_sound("generic_shot");

		if(!(global.frames % 6)) {
			create_projectile1c("youmu", plr->pos + 10 - I*20, 0, linear, -20.0*I)->type = PlrProj+120;
			create_projectile1c("youmu", plr->pos - 10 - I*20, 0, linear, -20.0*I)->type = PlrProj+120;
		}

		if(plr->shot == YoumuHoming && should_shoot(true)) {
			if(plr->inputflags & INFLAG_FOCUS) {
				int pwr = plr->power / 100;

				if(!(global.frames % (45 - 4 * pwr))) {
					int pcnt = 11 + pwr * 4;
					int pdmg = 120 - 18 * 4 * (1 - pow(1 - pwr / 4.0, 1.5));
					complex aim = 0.75;

					create_projectile4c("youhoming", plr->pos, 0, youmu_trap, -30.0*I, 120, pcnt+pdmg*I, aim)->type = PlrProj+1000;
				}
			} else {
				if(!(global.frames % 6)) {
					create_projectile4c("hghost", plr->pos, 0, youmu_homing, -10.0*I, 0.25 + 0.1*I, 60, VIEWPORT_W*0.5)->type = PlrProj+120;
				}

				for(int p = 1; p <= PLR_MAX_POWER/100; ++p) {
					youmu_homing_power_shot(plr, p);
				}
			}
		}
	}

	if(plr->shot == YoumuOpposite && plr->slaves == NULL)
		create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, YoumuOppositeMyon, youmu_opposite_myon, 0, 0, 0, 0);
}

void youmu_bomb(Player *plr) {
	switch(plr->shot) {
		case YoumuOpposite:
			create_enemy_p(&plr->slaves, 40.0*I, ENEMY_BOMB, NULL, youmu_split, 280,0,0,0);

			break;
		case YoumuHoming:
			create_enemy_p(&plr->slaves, 40.0*I, ENEMY_BOMB, YoumuSlash, youmu_slash, 280,0,0,0);
			break;
	}
}

void youmu_power(Player *plr, short npow) {
	if(plr->shot == YoumuOpposite && plr->slaves == NULL)
		create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, YoumuOppositeMyon, youmu_opposite_myon, 0, 0, 0, 0);
}

/* Marisa */

// Laser Sign
void MariLaser(Projectile *p, int t) {
	if(REF(p->args[1]) == NULL)
		return;

	if(cimag(p->pos) - cimag(global.plr.pos) < 90) {
		float s = resources.fbo.fg[0].scale;
		glScissor(VIEWPORT_X*s, s * (SCREEN_H - (cimag(((Enemy *)REF(p->args[1]))->pos) + 20)), SCREEN_W*s, SCREEN_H*s);
		glEnable(GL_SCISSOR_TEST);
	}

	ProjDraw(p, t);

	glDisable(GL_SCISSOR_TEST);
}

int mari_laser(Projectile *p, int t) {
	if(t == EVENT_DEATH) {
		free_ref(p->args[1]);
		return 1;
	}

	if(REF(p->args[1]) == NULL)
		return ACTION_DESTROY;

	float angle = creal(p->args[2]);
	float factor = (1-global.plr.focus/30.0) * !!angle;
	complex dir = -cexp(I*((angle+0.125*sin(global.frames/25.0)*(angle > 0? 1 : -1))*factor + M_PI/2));
	p->args[0] = 20*dir;
	linear(p, t);

	p->pos = ((Enemy *)REF(p->args[1]))->pos + p->pos;

	return 1;
}

int marisa_laser_slave(Enemy *e, int t) {
	if(should_shoot(true)) {
		if(!(global.frames % 4))
			create_projectile_p(&global.projs, get_tex("proj/marilaser"), 0, 0, MariLaser, mari_laser, 0, add_ref(e),e->args[2],0)->type = PlrProj+e->args[1]*4;

		if(!(global.frames % 3)) {
			float s = 0.5 + 0.3*sin(global.frames/7.0);
			create_particle3c("marilaser_part0", 0, rgb(1-s,0.5,s), PartDraw, mari_laser, 0, add_ref(e), e->args[2])->type=PlrProj;
		}
		create_particle1c("lasercurve", e->pos, 0, Fade, timeout, 4)->type = PlrProj;
	}

	e->pos = global.plr.pos + (1 - global.plr.focus/30.0)*e->pos0 + (global.plr.focus/30.0)*e->args[0];

	return 1;
}

void MariLaserSlave(Enemy *e, int t) {
	glPushMatrix();
	glTranslatef(creal(e->pos), cimag(e->pos), -1);
	glRotatef(global.frames * 3, 0, 0, 1);

	draw_texture(0,0,"part/lasercurve");

	glPopMatrix();
}

// Laser sign bomb (implemented as Enemy)

static void draw_masterspark_ring(complex base, int t, float fade) {
	glPushMatrix();

	glTranslatef(creal(base), cimag(base)-t*t*0.4, 0);

	float f = sqrt(t/500.0)*1200;

	glColor4f(1,1,1,fade*20.0/t);

	Texture *tex = get_tex("masterspark_ring");
	glScalef(f/tex->w, 1-tex->h/f,0);
	draw_texture_p(0,0,tex);

	glPopMatrix();
}

void MasterSpark(Enemy *e, int t) {
	glPushMatrix();

	float angle = 9 - t/e->args[0]*6.0, fade = 1;

	if(t < creal(e->args[0]/6))
		fade = t/e->args[0]*6;

	if(t > creal(e->args[0])/4*3)
		fade = 1-t/e->args[0]*4 + 3;

	glColor4f(1,0.85,1,fade);
	glTranslatef(creal(global.plr.pos), cimag(global.plr.pos), 0);
	glRotatef(-angle,0,0,1);
	draw_texture(0, -450, "masterspark");
	glColor4f(0.85,1,1,fade);
	glRotatef(angle*2,0,0,1);
	draw_texture(0, -450, "masterspark");
	glColor4f(1,1,1,fade);
	glRotatef(-angle,0,0,1);
	draw_texture(0, -450, "masterspark");
	glPopMatrix();

// 	glColor4f(0.9,1,1,fade*0.8);
	int i;
	for(i = 0; i < 8; i++)
		draw_masterspark_ring(global.plr.pos - 50.0*I, t%20 + 10*i, fade);

	glColor4f(1,1,1,1);
}

int master_spark(Enemy *e, int t) {
	if(t == EVENT_BIRTH) {
		global.shake_view = 8;
		return 1;
	}

	if(t > creal(e->args[0])) {
		global.shake_view = 0;
		return ACTION_DESTROY;
	}

	return 1;
}

// Star Sign

void MariStar(Projectile *p, int t) {
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(t*10, 0, 0, 1);

	if(p->clr) {
		parse_color_call(p->clr, glColor4f);
		draw_texture_p(0, 0, p->tex);
		glColor4f(1, 1, 1, 1);
	} else {
		draw_texture_p(0, 0, p->tex);
	}

	glPopMatrix();
}

void MariStarTrail(Projectile *p, int t) {
	float s = 1 - t / creal(p->args[0]);

	Color clr = derive_color(p->clr, CLRMASK_A, rgba(0, 0, 0, s*0.5));

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glPushMatrix();
	glTranslatef(creal(p->pos), cimag(p->pos), 0);
	glRotatef(t*10, 0, 0, 1);
	glScalef(s, s, 1);
	parse_color_call(clr, glColor4f);
	draw_texture_p(0, 0, p->tex);
	glColor4f(1,1,1,1);
	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MariStarBomb(Projectile *p, int t) {
	MariStar(p, t);
	create_particle1c("maristar_orbit", p->pos, 0, GrowFadeAdd, timeout, 40)->type = PlrProj;
}

int marisa_star_projectile(Projectile *p, int t) {
	float c = 0.3 * psin(t * 0.2);
	p->clr = rgb(1 - c, 0.7 + 0.3 * psin(t * 0.1), 0.9 + c/3);

	int r = accelerated(p, t);
	create_projectile_p(&global.particles, get_tex("proj/maristar"), p->pos, p->clr, MariStarTrail, timeout, 10, 0, 0, 0)->type = PlrProj;

	if(t == EVENT_DEATH) {
		Projectile *impact = create_projectile_p(&global.particles, get_tex("proj/maristar"), p->pos, p->clr, GrowFadeAdd, timeout, 40, 2, 0, 0);
		impact->type = PlrProj;
		impact->angle = frand() * 2 * M_PI;
	}

	return r;
}

int marisa_star_slave(Enemy *e, int t) {
	double focus = global.plr.focus/30.0;

	if(should_shoot(true) && !(global.frames % 15)) {
		complex v = e->args[1] * 2;
		complex a = e->args[2];

		v = creal(v) * (1 - 5 * focus) + I * cimag(v) * (1 - 2.5 * focus);
		a = creal(a) * focus * -0.0525 + I * cimag(a) * 2;

		create_projectile_p(&global.projs, get_tex("proj/maristar"), e->pos, rgb(1, 0.5, 1), MariStar, marisa_star_projectile,
							v, a, 0, 0)->type = PlrProj+e->args[3]*15;
	}

	e->pos = global.plr.pos + (1 - focus)*e->pos0 + focus*e->args[0];

	return 1;
}

int marisa_star_orbit(Projectile *p, int t) { // a[0]: x' a[1]: x''
	if(t == 0)
		p->pos0 = global.plr.pos;
	if(t < 0)
		return 1;

	if(t > 300)
		return ACTION_DESTROY;

	float r = cabs(p->pos0 - p->pos);

	p->args[1] = (0.5e5-t*t)*cexp(I*carg(p->pos0 - p->pos))/(r*r);
	p->args[0] += p->args[1]*0.2;
	p->pos += p->args[0];

	return 1;
}


// Generic Marisa

void marisa_shot(Player *plr) {
	if(should_shoot(false)) {
		if(!(global.frames % 4))
			play_sound("generic_shot");

		if(!(global.frames % 6)) {
			create_projectile1c("marisa", plr->pos + 10 - 15.0*I, 0, linear, -20.0*I)->type = PlrProj+175;
			create_projectile1c("marisa", plr->pos - 10 - 15.0*I, 0, linear, -20.0*I)->type = PlrProj+175;
		}
	}
}

void marisa_bomb(Player *plr) {
	int i;
	float r, phi;
	switch(plr->shot) {
	case MarisaLaser:
		play_sound("masterspark");
		create_enemy_p(&plr->slaves, 40.0*I, ENEMY_BOMB, MasterSpark, master_spark, 280,0,0,0);
		break;
	case MarisaStar:
		for(i = 0; i < 20; i++) {
			r = frand()*40 + 100;
			phi = frand()*2*M_PI;
			create_particle1c("maristar_orbit", plr->pos + r*cexp(I*phi), 0, MariStarBomb, marisa_star_orbit, I*r*cexp(I*(phi+frand()*0.5))/10)->type=PlrProj;
		}
		break;
	default:
		break;
	}
}

void marisa_power(Player *plr, short npow) {
	Enemy *e = plr->slaves, *tmp;
	double dmg;

	if(plr->power / 100 == npow / 100)
		return;

	while(e != 0) {
		tmp = e;
		e = e->next;
		if(tmp->hp == ENEMY_IMMUNE)
			delete_enemy(&plr->slaves, tmp);
	}

	switch(plr->shot) {
	case MarisaLaser:
		dmg = 4;

		if(npow / 100 == 1) {
			create_enemy_p(&plr->slaves, -40.0*I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -40.0*I, dmg, 0, 0);
		}

		if(npow >= 200) {
			create_enemy_p(&plr->slaves, 25-5.0*I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, 8-40.0*I, dmg,   -M_PI/30, 0);
			create_enemy_p(&plr->slaves, -25-5.0*I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -8-40.0*I, dmg,  M_PI/30, 0);
		}

		if(npow / 100 == 3) {
			create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -50.0*I, dmg, 0, 0);
		}

		if(npow >= 400) {
			create_enemy_p(&plr->slaves, 17-30.0*I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, 4-45.0*I, dmg, M_PI/60, 0);
			create_enemy_p(&plr->slaves, -17-30.0*I, ENEMY_IMMUNE, MariLaserSlave, marisa_laser_slave, -4-45.0*I, dmg, -M_PI/60, 0);
		}
		break;
	case MarisaStar:
		dmg = 5;

		if(npow / 100 == 1) {
			create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, +30.0*I, -2.0*I, -0.1*I, dmg);
		}

		if(npow >= 200) {
			create_enemy_p(&plr->slaves, 30.0*I+15, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, +30.0*I+10, -0.3-2.0*I,  1-0.1*I, dmg);
			create_enemy_p(&plr->slaves, 30.0*I-15, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, +30.0*I-10,  0.3-2.0*I, -1-0.1*I, dmg);
		}

		if(npow / 100 == 3) {
			create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, +30.0*I, -2.0*I, -0.1*I, dmg);
		}

		if(npow >= 400) {
			create_enemy_p(&plr->slaves,  30, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave,  25+30.0*I, -0.6-2.0*I,  2-0.1*I, dmg);
			create_enemy_p(&plr->slaves, -30, ENEMY_IMMUNE, MariLaserSlave, marisa_star_slave, -25+30.0*I,  0.6-2.0*I, -2-0.1*I, dmg);
		}
		break;
	}
}

int plrmode_repr(char *out, size_t outsize, Character pchar, ShotMode pshot) {
	char *plr, sht;

	switch(pchar) {
		case Marisa		:	plr = "marisa"	;	break;
		case Youmu		:	plr = "youmu"	;	break;
		default			:	plr = "wtf"		;	break;
	}

	switch(pshot) {
		case MarisaLaser:	sht = 'A'		;	break;
		case MarisaStar :	sht = 'B'		;	break;
		default			:	sht = '?'		;	break;
	}

	return snprintf(out, outsize, "%s%c", plr, sht);
}

// Inverse of plrmode_repr. Sets cha/shot according to name. Returns 0 iff the name is valid.
int plrmode_parse(const char *name, Character *cha, ShotMode *shot) {
	if(!strcasecmp(name,"marisaA")) {
		*cha = Marisa; *shot = MarisaLaser;
	} else if(!strcasecmp(name,"marisaB")) {
		*cha = Marisa; *shot = MarisaStar;
	} else if(!strcasecmp(name,"youmuA")) {
		*cha = Youmu; *shot = YoumuOpposite;
	} else if(!strcasecmp(name,"youmuB")) {
		*cha = Youmu; *shot = YoumuHoming;
	} else {
		return 1;
	}
	return 0;
}
