/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "laser.h"
#include "laserdraw.h"
#include "global.h"
#include "list.h"
#include "stageobjects.h"
#include "stagedraw.h"
#include "renderer/api.h"
#include "resource/model.h"
#include "util/fbmgr.h"
#include "util/glm.h"
#include "video.h"

Laser *create_laser(cmplx pos, float time, float deathtime, const Color *color, LaserPosRule prule, LaserLogicRule lrule, cmplx a0, cmplx a1, cmplx a2, cmplx a3) {
	Laser *l = alist_push(&global.lasers, (Laser*)objpool_acquire(stage_object_pools.lasers));

	l->birthtime = global.frames;
	l->timespan = time;
	l->deathtime = deathtime;
	l->pos = pos;
	l->color = *color;

	l->args[0] = a0;
	l->args[1] = a1;
	l->args[2] = a2;
	l->args[3] = a3;

	l->prule = prule;
	l->lrule = lrule;

	l->collision_step = 1;
	l->width = 10;
	l->width_exponent = 1.0;
	l->speed = 1;
	l->timeshift = 0;
	l->next_graze = 0;
	l->clear_flags = 0;
	l->unclearable = false;
	l->collision_active = true;

	l->ent.draw_layer = LAYER_LASER_HIGH;
	l->ent.draw_func = laserdraw_ent_drawfunc;
	ent_register(&l->ent, ENT_TYPE_ID(Laser));

	if(l->lrule)
		l->lrule(l, EVENT_BIRTH);

	l->prule(l, EVENT_BIRTH);

	return l;
}

Laser *create_laserline(cmplx pos, cmplx dir, float charge, float dur, const Color *clr) {
	return create_laserline_ab(pos, (pos)+(dir)*VIEWPORT_H*1.4/cabs(dir), cabs(dir), charge, dur, clr);
}

Laser *create_laserline_ab(cmplx a, cmplx b, float width, float charge, float dur, const Color *clr) {
	cmplx m = (b-a)*0.005;

	return create_laser(a, 200, dur, clr, las_linear, static_laser, m, charge + I*width, 0, 0);
}

static float laser_graze_width(Laser *l, float *exponent) {
	*exponent = 0; // l->width_exponent * 0.5f;
	return 5.0f * sqrtf(l->width) + 15.0f;
}

static void *_delete_laser(ListAnchor *lasers, List *laser, void *arg) {
	Laser *l = (Laser*)laser;

	if(l->lrule) {
		l->lrule(l, EVENT_DEATH);
	}

	ent_unregister(&l->ent);
	objpool_release(stage_object_pools.lasers, alist_unlink(lasers, laser));
	return NULL;
}

static void delete_laser(LaserList *lasers, Laser *laser) {
	_delete_laser((ListAnchor*)lasers, (List*)laser, NULL);
}

void delete_lasers(void) {
	alist_foreach(&global.lasers, _delete_laser, NULL);
}

bool laser_is_active(Laser *l) {
	// return l->width > 3.0;
	return l->collision_active;
}

bool laser_is_clearable(Laser *l) {
	return !l->unclearable && laser_is_active(l);
}

bool clear_laser(Laser *l, uint flags) {
	if(!(flags & CLEAR_HAZARDS_FORCE) && !laser_is_clearable(l)) {
		return false;
	}

	l->clear_flags |= flags;
	return true;
}

static bool laser_collision(Laser *l);

void process_lasers(void) {
	Laser *laser = global.lasers.first, *del = NULL;
	bool stage_cleared = stage_is_cleared();

	while(laser != NULL) {
		if(stage_cleared) {
			clear_laser(laser, CLEAR_HAZARDS_LASERS | CLEAR_HAZARDS_FORCE);
		}

		if(laser->clear_flags & CLEAR_HAZARDS_LASERS) {
			// TODO: implement CLEAR_HAZARDS_NOW

			laser->timespan *= 0.9;
			bool kill_now = laser->timespan < 5;

			if(!((global.frames - laser->birthtime) % 2) || kill_now) {
				double t = fmax(0, (global.frames - laser->birthtime)*laser->speed - laser->timespan + laser->timeshift);
				cmplx p = laser->prule(laser, t);
				double x = creal(p);
				double y = cimag(p);

				if(x > 0 && x < VIEWPORT_W && y > 0 && y < VIEWPORT_H) {
					create_clear_item(p, laser->clear_flags);
				}

				if(kill_now) {
					PARTICLE(
						.sprite = "flare",
						.pos = p,
						.timeout = 20,
						.draw_rule = pdraw_timeout_scalefade(0, 1, 1, 0),
					);
					laser->deathtime = 0;
				}
			}
		} else {
			if(laser_collision(laser)) {
				ent_damage(&global.plr.ent, &(DamageInfo) { .type = DMG_ENEMY_SHOT });
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

static inline bool laser_collision_segment(Laser *l, LineSegment *segment, Circle *collision_area, float t, float segment_width_factor, float tail) {
	float mid_ofs = t - l->timespan * 0.5f;
	float widthfac_orig = segment_width_factor * (mid_ofs - tail) * (mid_ofs + tail);
	float widthfac = 0.75f * 0.5f * powf(widthfac_orig, l->width_exponent);

	collision_area->radius = fmaxf(widthfac * l->width - 4.0f, 2.0f);

	if(lineseg_circle_intersect(*segment, *collision_area) >= 0.0f) {
		return true;
	}

	if(global.frames >= l->next_graze) {
		float exponent;
		collision_area->radius = laser_graze_width(l, &exponent) * fmaxf(0.25f, powf(widthfac_orig, exponent));
		assert(collision_area->radius > 0);
		float f = lineseg_circle_intersect(*segment, *collision_area);

		if(f >= 0) {
			player_graze(&global.plr, segment->a + f * (segment->b - segment->a), 7, 5, &l->color);
			l->next_graze = global.frames + 4;
		}
	}

	return false;
}

static bool laser_collision(Laser *l) {
	if(!laser_is_active(l)) {
		return false;
	}

	float t_end_len = (global.frames - l->birthtime) * l->speed + l->timeshift; // end of the laser based on length
	float t_end_lifetime = l->deathtime * l->speed + l->timeshift; // end of the laser based on lifetime
	float t = t_end_len - l->timespan;
	float t_end = fmin(t_end_len, t_end_lifetime);

	if(t < 0) {
		t = 0;
	}

	LineSegment segment = { .a = l->prule(l, t) };
	Circle collision_area = { .origin = global.plr.pos };

	float tail = l->timespan / 1.6f;
	float width_factor = -1.0f / (tail * tail);

	for(t += l->collision_step; t < t_end; t += l->collision_step) {
		segment.b = l->prule(l, t);

		if(laser_collision_segment(l, &segment, &collision_area, t, width_factor, tail)) {
			return true;
		}

		segment.a = segment.b;
	}

	segment.b = l->prule(l, t_end);
	return laser_collision_segment(l, &segment, &collision_area, t_end, width_factor, tail);
}

bool laser_intersects_ellipse(Laser *l, Ellipse ellipse) {
	// NOTE: this function does not take laser width into account

	float t_end_len = (global.frames - l->birthtime) * l->speed + l->timeshift; // end of the laser based on length
	float t_end_lifetime = l->deathtime * l->speed + l->timeshift; // end of the laser based on lifetime
	float t = t_end_len - l->timespan;
	float t_end = fmin(t_end_len, t_end_lifetime);

	if(t < 0) {
		t = 0;
	}

	LineSegment segment = { .a = l->prule(l, t) };

	for(t += l->collision_step; t < t_end; t += l->collision_step) {
		segment.b = l->prule(l, t);

		if(lineseg_ellipse_intersect(segment, ellipse)) {
			return true;
		}

		segment.a = segment.b;
	}

	segment.b = l->prule(l, t_end);
	return lineseg_ellipse_intersect(segment, ellipse);
}

bool laser_intersects_circle(Laser *l, Circle circle) {
	Ellipse ellipse = {
		.origin = circle.origin,
		.axes = circle.radius * 2 * (1 + I),
	};

	return laser_intersects_ellipse(l, ellipse);
}

cmplx las_linear(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->collision_step = fmax(3, l->timespan/10);
		return 0;
	}

	return l->pos + l->args[0]*t;
}

cmplx las_accel(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->collision_step = fmax(3, l->timespan/10);
		return 0;
	}

	return l->pos + l->args[0]*t + 0.5*l->args[1]*t*t;
}

cmplx las_weird_sine(Laser *l, float t) {             // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// XXX: this used to be called "las_sine", but it's actually not a proper sine wave
	// do we even still need this?

	if(t == EVENT_BIRTH) {
		return 0;
	}

	double s = (l->args[2] * t + l->args[3]);
	return l->pos + cexp(I * (carg(l->args[0]) + l->args[1] * sin(s) / s)) * t * cabs(l->args[0]);
}

cmplx las_sine(Laser *l, float t) {               // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// this is actually shaped like a sine wave

	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx line_vel = l->args[0];
	cmplx line_dir = line_vel / cabs(line_vel);
	cmplx line_normal = cimag(line_dir) - I*creal(line_dir);
	cmplx sine_amp = l->args[1];
	cmplx sine_freq = l->args[2];
	cmplx sine_phase = l->args[3];

	cmplx sine_ofs = line_normal * sine_amp * sin(sine_freq * t + sine_phase);
	return l->pos + t * line_vel + sine_ofs;
}

cmplx las_sine_expanding(Laser *l, float t) { // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// XXX: this is also a "weird" one

	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx velocity = l->args[0];
	double amplitude = creal(l->args[1]);
	double frequency = creal(l->args[2]);
	double phase = creal(l->args[3]);

	double angle = carg(velocity);
	double speed = cabs(velocity);

	double s = (frequency * t + phase);
	return l->pos + cexp(I * (angle + amplitude * sin(s))) * t * speed;
}

cmplx las_turning(Laser *l, float t) { // [0] = vel0; [1] = vel1; [2] r: turn begin time, i: turn end time
	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx v0 = l->args[0];
	cmplx v1 = l->args[1];
	float begin = creal(l->args[2]);
	float end = cimag(l->args[2]);

	float a = clamp((t - begin) / (end - begin), 0, 1);
	a = 1.0 - (0.5 + 0.5 * cos(a * M_PI));
	a = 1.0 - pow(1.0 - a, 2);

	cmplx v = v1 * a + v0 * (1 - a);

	return l->pos + v * t;
}

cmplx las_circle(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	// XXX: should turn speed be in rad/sec or rad/frame? currently rad/sec.
	double turn_speed = creal(l->args[0]) / 60;
	double time_ofs = cimag(l->args[0]);
	double radius = creal(l->args[1]);

	return l->pos + radius * cexp(I * (t + time_ofs) * turn_speed);
}

void laser_charge(Laser *l, int t, float charge, float width) {
	float new_width;

	if(t < charge - 10) {
		new_width = fminf(2.0f, 2.0f * t / fminf(30.0f, charge - 10.0f));
	} else if(t >= charge - 10.0f && t < l->deathtime - 20.0f) {
		new_width = fminf(width, 1.7f + width / 20.0f * (t - charge + 10.0f));
	} else if(t >= l->deathtime - 20.0f) {
		new_width = fmaxf(0.0f, width - width / 20.0f * (t - l->deathtime + 20.0f));
	} else {
		new_width = width;
	}

	l->width = new_width;
	l->collision_active = (new_width > width * 0.6f);
}

void laser_make_static(Laser *l) {
	l->speed = 0;
	l->timeshift = l->timespan;
}

void static_laser(Laser *l, int t) {
	if(t == EVENT_BIRTH) {
		l->width = 0;
		l->collision_active = false;
		laser_make_static(l);
		return;
	}

	laser_charge(l, t, creal(l->args[1]), cimag(l->args[1]));
}

DEFINE_EXTERN_TASK(laser_charge) {
	Laser *l = TASK_BIND(ARGS.laser);

	l->width = 0;
	l->collision_active = false;
	laser_make_static(l);

	float target_width = ARGS.target_width;
	float charge_delay = ARGS.charge_delay;

	for(int t = 0;; ++t) {
		laser_charge(l, t, charge_delay, target_width);
		YIELD;
	}
}
