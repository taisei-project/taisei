/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "laser.h"
#include "global.h"
#include "list.h"

Laser *create_laser(complex pos, float time, float deathtime, Color color, LaserPosRule prule, LaserLogicRule lrule, complex a0, complex a1, complex a2, complex a3) {
	Laser *l = create_element((void **)&global.lasers, sizeof(Laser));

	l->birthtime = global.frames;
	l->timespan = time;
	l->deathtime = deathtime;
	l->pos = pos;
	l->color = color;

	l->args[0] = a0;
	l->args[1] = a1;
	l->args[2] = a2;
	l->args[3] = a3;

	l->prule = prule;
	l->lrule = lrule;

	l->shader = NULL;
	l->collision_step = 5;
	l->width = 10;
	l->speed = 1;
	l->timeshift = 0;
	l->in_background = false;
	l->dead = false;
	l->unclearable = false;

	if(l->lrule)
		l->lrule(l, EVENT_BIRTH);

	l->prule(l, EVENT_BIRTH);

	return l;
}

Laser *create_laserline_ab(complex a, complex b, float width, float charge, float dur, Color clr) {
	complex m = (b-a)*0.005;

	return create_laser(a, 200, dur, clr, las_linear, static_laser, m, charge + I*width, 0, 0);
}

void draw_laser_curve_instanced(Laser *l) {
	static float clr[4];
	float t;
	int c;

	Texture *tex = get_tex("part/lasercurve");

	float wq = ((float)tex->w)/tex->truew;
	float hq = ((float)tex->h)/tex->trueh;

	c = l->timespan;

	t = (global.frames - l->birthtime)*l->speed - l->timespan + l->timeshift;

	if(t + l->timespan > l->deathtime + l->timeshift)
		c += l->deathtime + l->timeshift - (t + l->timespan);

	if(t < 0) {
		c += t;
		t = 0;
	}

	if(c < 0) {
		return;
	}

	glBindTexture(GL_TEXTURE_2D, tex->gltex);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glUseProgram(l->shader->prog);
	parse_color_array(l->color, clr);
	glUniform4fv(uniloc(l->shader, "clr"), 1, clr);

	glUniform2f(uniloc(l->shader, "pos"), creal(l->pos), cimag(l->pos));
	glUniform2f(uniloc(l->shader, "a0"), creal(l->args[0]), cimag(l->args[0]));
	glUniform2f(uniloc(l->shader, "a1"), creal(l->args[1]), cimag(l->args[1]));
	glUniform2f(uniloc(l->shader, "a2"), creal(l->args[2]), cimag(l->args[2]));
	glUniform2f(uniloc(l->shader, "a3"), creal(l->args[3]), cimag(l->args[3]));

	glUniform1f(uniloc(l->shader, "timeshift"), t);
	glUniform1f(uniloc(l->shader, "wq"), wq*l->width);
	glUniform1f(uniloc(l->shader, "hq"), hq*l->width);

	glUniform1i(uniloc(l->shader, "span"), c*2);

	glDrawArraysInstanced(GL_QUADS, 0, 4, c*2);

	glUseProgram(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void draw_laser_curve(Laser *laser) {
	glEnable(GL_TEXTURE_2D);
	Texture *tex = get_tex("part/lasercurve");
	complex last;

	glBindTexture(GL_TEXTURE_2D, tex->gltex);
	parse_color_call(laser->color, glColor4f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	float t = (global.frames - laser->birthtime)*laser->speed - laser->timespan + laser->timeshift;
	if(t < 0)
		t = 0;

	last = laser->prule(laser, t);

	for(t += 0.5; t < (global.frames - laser->birthtime)*laser->speed + laser->timeshift && t <= laser->deathtime + laser->timeshift; t += 1.5) {
		complex pos = laser->prule(laser,t);
		glPushMatrix();

		float t1 = t - ((global.frames - laser->birthtime)*laser->speed - laser->timespan/2 + laser->timeshift);

		float tail = laser->timespan/1.9;

		float s = -0.75/pow(tail,2)*(t1-tail)*(t1+tail);

		glTranslatef(creal(pos), cimag(pos), 0);
		glRotatef(180/M_PI*carg(last-pos), 0, 0, 1);

		float wq = ((float)tex->w)/tex->truew;
		float hq = ((float)tex->h)/tex->trueh;

		glScalef(tex->w*wq*0.5*cabs(last-pos),s*laser->width*hq,s);
		draw_quad();

		last = pos;

		glPopMatrix();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(1,1,1,1);
	glDisable(GL_TEXTURE_2D);
}

void draw_lasers(int bgpass) {
	Laser *laser;

	for(laser = global.lasers; laser; laser = laser->next) {
		if(bgpass != laser->in_background)
			continue;

		if(laser->shader && glext.draw_instanced)
			draw_laser_curve_instanced(laser);
		else
			draw_laser_curve(laser);
	}
}

void _delete_laser(void **lasers, void *laser) {
	Laser *l = laser;

	if(l->lrule)
		l->lrule(l, EVENT_DEATH);

	del_ref(laser);
	delete_element(lasers, laser);
}

void delete_lasers(void) {
	delete_all_elements((void **)&global.lasers, _delete_laser);
}

void process_lasers(void) {
	Laser *laser = global.lasers, *del = NULL;

	while(laser != NULL) {
		if(laser->dead) {
			laser->timespan *= 0.9;
			bool kill_now = laser->timespan < 5;

			if(!((global.frames - laser->birthtime) % 2) || kill_now) {
				double t = max(0, (global.frames - laser->birthtime)*laser->speed - laser->timespan + laser->timeshift);
				complex p = laser->prule(laser, t);
				double x = creal(p);
				double y = cimag(p);

				if(x > 0 && x < VIEWPORT_W && y > 0 && y < VIEWPORT_H) {
					create_particle1c("flare", p, 0, Fade, timeout, 30);
					create_item(p, 0, BPoint)->auto_collect = 10;
				}

				if(kill_now) {
					create_particle1c("flare", p, 0, GrowFade, timeout, 20);
					laser->deathtime = 0;
				}
			}
		} else {
			if(collision_laser_curve(laser)) {
				player_death(&global.plr);
			}

			if(laser->lrule) {
				laser->lrule(laser, global.frames - laser->birthtime);
			}
		}

		if(global.frames - laser->birthtime > laser->deathtime + laser->timespan*laser->speed) {
			del = laser;
			laser = laser->next;
			_delete_laser((void **)&global.lasers, del);
		} else {
			laser = laser->next;
		}
	}
}

int collision_line(complex a, complex b, complex c, float r) {
	complex m;
	float d, la, lm, s;

	m = b-a;
	a -= c;

	la = a*conj(a);
	lm = m*conj(m);

	d = -(creal(a)*creal(m)+cimag(a)*cimag(m))/lm;

	s = d*d - (la - r*r)/lm;

	if(s >= 0) {
		if((d+s >= 0 && d+s <= 1) || (d-s >= 0 && d-s <= 1))
			return 1;
	}

	return 0;
}

int collision_laser_curve(Laser *l) {
	float s = (global.frames - l->birthtime)*l->speed + l->timeshift;
	float t = s - l->timespan;
	complex last, pos;

	if(l->width <= 3.0)
		return 0;

	if(t < 0)
		t = 0;

	last = l->prule(l,t);

	for(t += l->collision_step; t + l->collision_step <= s && t + l->collision_step <= l->deathtime + l->timeshift; t += l->collision_step) {
		pos = l->prule(l,t);
		if(collision_line(last, pos, global.plr.pos, l->width*0.5))
			return 1;
		else if(!(global.frames % 5) && global.frames - abs(global.plr.recovery) > 0 && collision_line(last, pos, global.plr.pos, l->width*1.8))
			player_graze(&global.plr, pos, 7);

		last = pos;
	}

	pos = l->prule(l, min(s, l->deathtime + l->timeshift));

	if(collision_line(last, pos, global.plr.pos, l->width*0.5))
		return 1;

	return 0;
}

complex las_linear(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = get_shader("laser_linear");
		l->collision_step = l->timespan;
		return 0;
	}

	return l->pos + l->args[0]*t;
}

complex las_accel(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = get_shader("laser_accelerated");
		return 0;
	}

	return l->pos + l->args[0]*t + 0.5*l->args[1]*t*t;
}

complex las_sine(Laser *l, float t) {				// [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	if(t == EVENT_BIRTH) {
		l->shader = get_shader("laser_sine");
		return 0;
	}

	double s = (l->args[2] * t + l->args[3]);
	return l->pos + cexp(I * (carg(l->args[0]) + l->args[1] * sin(s) / s)) * t * cabs(l->args[0]);
}

complex las_sine_expanding(Laser *l, float t) {	// [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	if(t == EVENT_BIRTH) {
		l->shader = get_shader("laser_sine_expanding");
		l->collision_step = 3;
		return 0;
	}

	double s = (l->args[2] * t + l->args[3]);
	return l->pos + cexp(I * (carg(l->args[0]) + l->args[1] * sin(s))) * t * cabs(l->args[0]);
}

float laser_charge(Laser *l, int t, float charge, float width) {
	if(t < charge - 10)
		return 1.7;

	if(t >= charge - 10 && t < l->deathtime - 20) {
		float w = 1.7 + width/20*(t-charge+10);
		return w < width ? w : width;
	}

	if(t >= l->deathtime - 20) {
		float w = width - width/20*(t-l->deathtime+20);
		return w > 0 ? w : 0;
	}

	return width;
}

void static_laser(Laser *l, int t) {
	if(t == EVENT_BIRTH) {
		l->width = 0;
		l->speed = 0;
		l->timeshift = l->timespan;
		return;
	}

	l->width = laser_charge(l, t, creal(l->args[1]), cimag(l->args[1]));
}
