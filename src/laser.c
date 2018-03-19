/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "laser.h"
#include "global.h"
#include "list.h"
#include "stageobjects.h"
#include "renderer/api.h"

Laser *create_laser(complex pos, float time, float deathtime, Color color, LaserPosRule prule, LaserLogicRule lrule, complex a0, complex a1, complex a2, complex a3) {
	Laser *l = (Laser*)list_push(&global.lasers, objpool_acquire(stage_object_pools.lasers));

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
	l->collision_step = 1;
	l->width = 10;
	l->width_exponent = 1.0;
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

Laser *create_laserline(complex pos, complex dir, float charge, float dur, Color clr) {
	return create_laserline_ab(pos, (pos)+(dir)*VIEWPORT_H*1.4/cabs(dir), cabs(dir), charge, dur, clr);
}

Laser *create_laserline_ab(complex a, complex b, float width, float charge, float dur, Color clr) {
	complex m = (b-a)*0.005;

	return create_laser(a, 200, dur, clr, las_linear, static_laser, m, charge + I*width, 0, 0);
}

static void draw_laser_curve_instanced(Laser *l) {
	float t;
	int c;

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

	r_uniform_rgba("clr", l->color);
	r_uniform_complex("pos", l->pos);
	r_uniform_complex("a0", l->args[0]);
	r_uniform_complex("a1", l->args[1]);
	r_uniform_complex("a2", l->args[2]);
	r_uniform_complex("a3", l->args[3]);
	r_uniform_float("timeshift", t);
	r_uniform_float("width", l->width);
	r_uniform_float("width_exponent", l->width_exponent);
	r_uniform_int("span", c * 2);

	r_flush();
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, c*2);
}

static void draw_laser_curve(Laser *laser) {
	Texture *tex = get_tex("part/lasercurve");
	complex last;

	r_color(laser->color);

	float t = (global.frames - laser->birthtime)*laser->speed - laser->timespan + laser->timeshift;
	if(t < 0)
		t = 0;

	last = laser->prule(laser, t);

	for(t += 0.5; t < (global.frames - laser->birthtime)*laser->speed + laser->timeshift && t <= laser->deathtime + laser->timeshift; t += 1.5) {
		complex pos = laser->prule(laser,t);
		r_mat_push();

		float t1 = t - ((global.frames - laser->birthtime)*laser->speed - laser->timespan/2 + laser->timeshift);

		float tail = laser->timespan/1.9;

		float s = -0.75/pow(tail,2)*(t1-tail)*(t1+tail);
		s = pow(s, laser->width_exponent);

		r_mat_translate(creal(pos), cimag(pos), 0);
		r_mat_rotate_deg(180/M_PI*carg(last-pos), 0, 0, 1);

		r_mat_scale(tex->w*0.5*cabs(last-pos),s*laser->width,s);
		r_draw_quad();

		last = pos;

		r_mat_pop();
	}

	r_color4(1,1,1,1);
}

void draw_lasers(int bgpass) {
	Laser *laser;
	r_texture(0, "part/lasercurve");
	r_blend(BLEND_ADD);

	for(laser = global.lasers; laser; laser = laser->next) {
		if(bgpass != laser->in_background) {
			continue;
		}

		if(laser->shader && glext.draw_instanced) {
			r_shader_ptr(laser->shader);
			draw_laser_curve_instanced(laser);
		} else {
			r_shader_standard();
			draw_laser_curve(laser);
		}
	}

	r_blend(BLEND_ALPHA);
	r_shader_standard();
}

void* _delete_laser(List **lasers, List *laser, void *arg) {
	Laser *l = (Laser*)laser;

	if(l->lrule)
		l->lrule(l, EVENT_DEATH);

	del_ref(laser);
	objpool_release(stage_object_pools.lasers, (ObjectInterface*)list_unlink(lasers, laser));
	return NULL;
}

void delete_laser(Laser **lasers, Laser *laser) {
	_delete_laser((List**)lasers, (List*)laser, NULL);
}

void delete_lasers(void) {
	list_foreach(&global.lasers, _delete_laser, NULL);
}

bool clear_laser(Laser **laserlist, Laser *l, bool force, bool now) {
	if(!force && l->unclearable) {
		return false;
	}

	// TODO: implement "now"
	l->dead = true;
	return true;
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
					create_bpoint(p);
				}

				if(kill_now) {
					PARTICLE("flare", p, 0, timeout, { 20 }, .draw_rule = GrowFade);
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
			delete_laser(&global.lasers, del);
		} else {
			laser = laser->next;
		}
	}
}

// what this terrible function probably does:
// is the point of shortest distance between the line through a and b
// and a point c between a and b and closer than r? if yes return f so that a+f*(b-a) is that point.
// otherwise return -1.
double collision_line(complex a, complex b, complex c, float r) {
	complex m,v;
	double projection, lv, lm, distance;

	m = b-a; // vector pointing along the line
	v = a-c; // vector from collider to point A

	lv = cabs(v);
	lm = cabs(m);
	if(lm == 0) {
		return -1;
	}

	projection = -creal(v*conj(m))/lm; // project v onto the line

	// now the distance can be calculated by Pythagoras
	distance = sqrt(lv*lv-projection*projection);
	if(distance <= r) {
		double f = projection/lm;
		if(f >= 0 && f <= 1) // it’s on the line!
			return f;
	}

	// now here is an additional check that becomes important if we use
	// this to check collisions along a curve made out of multiple straight
	// line segments.
	// Imagine approaching a convex corner of the curve from outside. With
	// just the above check there is a triangular death zone. This check
	// here puts a circle in that zone.
	if(lv < r)
		return 0;

	return -1;
}

int collision_laser_curve(Laser *l) {
	float t_end = (global.frames - l->birthtime)*l->speed + l->timeshift; // end of the laser based on length
	float t_death = l->deathtime*l->speed+l->timeshift; // end of the laser based on lifetime
	float t = t_end - l->timespan;
	complex last, pos;

	bool grazed = false;

	if(l->width <= 3.0)
		return 0;

	if(t < 0)
		t = 0;

	//float t_start = t;

	last = l->prule(l,t);

	for(t += l->collision_step; t <= min(t_end,t_death); t += l->collision_step) {
		pos = l->prule(l,t);
		float t1 = t-l->timespan/2; // i have no idea
		float tail = l->timespan/1.9;

		float widthfac = -0.75/pow(tail,2)*(t1-tail)*(t1+tail);

		//float widthfac = -(t-t_start)*(t-min(t_end,t_death))/pow((min(t_end,t_death)-t_start)/2.,2);
		widthfac = max(0.25,pow(widthfac, l->width_exponent));

		float collision_width = widthfac*l->width*0.5+1;

		if(collision_line(last, pos, global.plr.pos, collision_width) >= 0) {
			return 1;
		}
		if(!grazed && !(global.frames % 7) && global.frames - abs(global.plr.recovery) > 0) {
			float f = collision_line(last, pos, global.plr.pos, l->width*2+8);

			if(f >= 0) {
				player_graze(&global.plr, last+f*(pos-last), 7, 5);
				grazed = true;
			}
		}

		last = pos;
	}

	pos = l->prule(l, min(t_end, t_death));

	if(collision_line(last, pos, global.plr.pos, l->width*0.5) >= 0)
		return 1;

	return 0;
}

complex las_linear(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("laser_linear");
		l->collision_step = max(3,l->timespan/10);
		return 0;
	}

	return l->pos + l->args[0]*t;
}

complex las_accel(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("laser_accelerated");
		return 0;
	}

	return l->pos + l->args[0]*t + 0.5*l->args[1]*t*t;
}

complex las_weird_sine(Laser *l, float t) {             // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// XXX: this used to be called "las_sine", but it's actually not a proper sine wave
	// do we even still need this?

	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("laser_weird_sine");
		return 0;
	}

	double s = (l->args[2] * t + l->args[3]);
	return l->pos + cexp(I * (carg(l->args[0]) + l->args[1] * sin(s) / s)) * t * cabs(l->args[0]);
}

complex las_sine(Laser *l, float t) {               // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// this is actually shaped like a sine wave

	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("laser_sine");
		return 0;
	}

	complex line_vel = l->args[0];
	complex line_dir = line_vel / cabs(line_vel);
	complex line_normal = cimag(line_dir) - I*creal(line_dir);
	complex sine_amp = l->args[1];
	complex sine_freq = l->args[2];
	complex sine_phase = l->args[3];

	complex sine_ofs = line_normal * sine_amp * sin(sine_freq * t + sine_phase);
	return l->pos + t * line_vel + sine_ofs;
}

complex las_sine_expanding(Laser *l, float t) { // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// XXX: this is also a "weird" one

	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("laser_sine_expanding");
		return 0;
	}

	complex velocity = l->args[0];
	double amplitude = creal(l->args[1]);
	double frequency = creal(l->args[2]);
	double phase = creal(l->args[3]);

	double angle = carg(velocity);
	double speed = cabs(velocity);

	double s = (frequency * t + phase);
	return l->pos + cexp(I * (angle + amplitude * sin(s))) * t * speed;
}

complex las_turning(Laser *l, float t) { // [0] = vel0; [1] = vel1; [2] r: turn begin time, i: turn end time
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("laser_turning");
		return 0;
	}

	complex v0 = l->args[0];
	complex v1 = l->args[1];
	float begin = creal(l->args[2]);
	float end = cimag(l->args[2]);

	float a = clamp((t - begin) / (end - begin), 0, 1);
	a = 1.0 - (0.5 + 0.5 * cos(a * M_PI));
	a = 1.0 - pow(1.0 - a, 2);

	complex v = v1 * a + v0 * (1 - a);

	return l->pos + v * t;
}

complex las_circle(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("laser_circle");
		return 0;
	}

	// XXX: should turn speed be in rad/sec or rad/frame? currently rad/sec.
	double turn_speed = creal(l->args[0]) / 60;
	double time_ofs = cimag(l->args[0]);
	double radius = creal(l->args[1]);

	return l->pos + radius * cexp(I * (t + time_ofs) * turn_speed);
}

float laser_charge(Laser *l, int t, float charge, float width) {
	if(t < charge - 10) {
		return min(2, 2 * t / min(30, charge - 10));
	}

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
