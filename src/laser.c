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

static struct {
	VertexArray varr;
	VertexBuffer vbuf;
	ShaderProgram *shader_generic;
} lasers;

typedef struct LaserInstancedAttribs {
	float pos[2];
	float delta[2];
} LaserInstancedAttribs;

void lasers_preload(void) {
	preload_resource(RES_SHADER_PROGRAM, "laser_generic", RESF_DEFAULT);

	size_t sz_vert = sizeof(GenericModelVertex);
	size_t sz_attr = sizeof(LaserInstancedAttribs);

	#define VERTEX_OFS(attr)   offsetof(GenericModelVertex,  attr)
	#define INSTANCE_OFS(attr) offsetof(LaserInstancedAttribs, attr)

	VertexAttribFormat fmt[] = {
		// Per-vertex attributes (for the static models buffer, bound at 0)
		{ { 3, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(position),       0 },
		{ { 3, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(normal),         0 },
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT, 0 }, sz_vert, VERTEX_OFS(uv),             0 },

		// Per-instance attributes (for our own buffer, bound at 1)
		// pos and delta packed into a single attribute
		{ { 4, VA_FLOAT, VA_CONVERT_FLOAT, 1 }, sz_attr, INSTANCE_OFS(pos),          1 },
	};

	#undef VERTEX_OFS
	#undef INSTANCE_OFS

	r_vertex_buffer_create(&lasers.vbuf, sizeof(LaserInstancedAttribs) * 4096, NULL);

	r_vertex_array_create(&lasers.varr);
	r_vertex_array_layout(&lasers.varr, sizeof(fmt)/sizeof(VertexAttribFormat), fmt);
	r_vertex_array_attach_buffer(&lasers.varr, r_vertex_buffer_static_models(), 0);
	r_vertex_array_attach_buffer(&lasers.varr, &lasers.vbuf, 1);
	r_vertex_array_layout(&lasers.varr, sizeof(fmt)/sizeof(VertexAttribFormat), fmt);

	lasers.shader_generic = r_shader_get("laser_generic");
}

void lasers_free(void) {
	r_vertex_array_destroy(&lasers.varr);
	r_vertex_buffer_destroy(&lasers.vbuf);
}

static void ent_draw_laser(EntityInterface *ent);

Laser *create_laser(complex pos, float time, float deathtime, const Color *color, LaserPosRule prule, LaserLogicRule lrule, complex a0, complex a1, complex a2, complex a3) {
	Laser *l = (Laser*)alist_push(&global.lasers, objpool_acquire(stage_object_pools.lasers));

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

	l->shader = NULL;
	l->collision_step = 1;
	l->width = 10;
	l->width_exponent = 1.0;
	l->speed = 1;
	l->timeshift = 0;
	l->dead = false;
	l->unclearable = false;

	l->ent.draw_layer = LAYER_LASER_HIGH;
	l->ent.draw_func = ent_draw_laser;
	ent_register(&l->ent, ENT_LASER);

	if(l->lrule)
		l->lrule(l, EVENT_BIRTH);

	l->prule(l, EVENT_BIRTH);

	return l;
}

Laser *create_laserline(complex pos, complex dir, float charge, float dur, const Color *clr) {
	return create_laserline_ab(pos, (pos)+(dir)*VIEWPORT_H*1.4/cabs(dir), cabs(dir), charge, dur, clr);
}

Laser *create_laserline_ab(complex a, complex b, float width, float charge, float dur, const Color *clr) {
	complex m = (b-a)*0.005;

	return create_laser(a, 200, dur, clr, las_linear, static_laser, m, charge + I*width, 0, 0);
}

static bool draw_laser_instanced_prepare(Laser *l, uint *out_instances, float *out_timeshift) {
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

	if(c <= 0) {
		return false;
	}

	*out_instances = c * 2;
	*out_timeshift = t;

	return true;
}

static void draw_laser_curve_specialized(Laser *l) {
	float timeshift;
	uint instances;

	if(!draw_laser_instanced_prepare(l, &instances, &timeshift)) {
		return;
	}

	r_color(&l->color);
	r_uniform_complex("origin", l->pos);
	r_uniform_complex_array("args[0]", 4, l->args);
	r_uniform_float("timeshift", timeshift);
	r_uniform_float("width", l->width);
	r_uniform_float("width_exponent", l->width_exponent);
	r_uniform_int("span", instances);
	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, instances, 0);
}

static void draw_laser_curve_generic(Laser *l) {
	float timeshift;
	uint instances;

	if(!draw_laser_instanced_prepare(l, &instances, &timeshift)) {
		return;
	}

	r_color(&l->color);
	r_uniform_float("timeshift", timeshift);
	r_uniform_float("width", l->width);
	r_uniform_float("width_exponent", l->width_exponent);
	r_uniform_int("span", instances);

	LaserInstancedAttribs attrs[instances], *aptr = attrs;
	r_vertex_buffer_invalidate(&lasers.vbuf);

	for(uint i = 0; i < instances; ++i, ++aptr) {
		complex pos = l->prule(l, i * 0.5 + timeshift);
		complex delta = pos - l->prule(l, i * 0.5 + timeshift - 0.1);

		aptr->pos[0] = creal(pos);
		aptr->pos[1] = cimag(pos);
		aptr->delta[0] = creal(delta);
		aptr->delta[1] = cimag(delta);
	}

	r_vertex_buffer_append(&lasers.vbuf, sizeof(attrs), &attrs);
	r_draw(PRIM_TRIANGLE_FAN, 0, 4, NULL, instances, 0);
}

static void ent_draw_laser(EntityInterface *ent) {
	Laser *laser = ENT_CAST(ent, Laser);

	r_texture(0, "part/lasercurve");

	if(laser->shader) {
		// Specialized lasers work with either vertex array,
		// provided that the static models buffer is attached to it.
		// We'll only ever draw the first quad, and only care about
		// attributes 0 and 2 (vec3 position, vec2 uv)

		VertexArray *va = r_vertex_array_current();

		if(va != &lasers.varr && va != r_vertex_array_static_models()) {
			r_vertex_array(&lasers.varr);
		}

		r_shader_ptr(laser->shader);
		draw_laser_curve_specialized(laser);
	} else {
		r_vertex_array(&lasers.varr);
		r_shader_ptr(lasers.shader_generic);
		draw_laser_curve_generic(laser);
	}
}

void* _delete_laser(ListAnchor *lasers, List *laser, void *arg) {
	Laser *l = (Laser*)laser;

	if(l->lrule)
		l->lrule(l, EVENT_DEATH);

	del_ref(laser);
	ent_unregister(&l->ent);
	objpool_release(stage_object_pools.lasers, (ObjectInterface*)alist_unlink(lasers, laser));
	return NULL;
}

void delete_laser(LaserList *lasers, Laser *laser) {
	_delete_laser((ListAnchor*)lasers, (List*)laser, NULL);
}

void delete_lasers(void) {
	alist_foreach(&global.lasers, _delete_laser, NULL);
}

bool clear_laser(LaserList *laserlist, Laser *l, bool force, bool now) {
	if(!force && l->unclearable) {
		return false;
	}

	// TODO: implement "now"
	l->dead = true;
	return true;
}

static bool collision_laser_curve(Laser *l);

void process_lasers(void) {
	Laser *laser = global.lasers.first, *del = NULL;

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
					PARTICLE(
						.sprite = "flare",
						.pos = p,
						.timeout = 20,
						.draw_rule = GrowFade
					);
					laser->deathtime = 0;
				}
			}
		} else {
			if(collision_laser_curve(laser)) {
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

static bool collision_laser_curve(Laser *l) {
	if(l->width <= 3.0) {
		return false;
	}

	float t_end = (global.frames - l->birthtime) * l->speed + l->timeshift; // end of the laser based on length
	float t_death = l->deathtime * l->speed + l->timeshift; // end of the laser based on lifetime
	float t = t_end - l->timespan;
	bool grazed = false;

	if(t < 0) {
		t = 0;
	}

	LineSegment segment = { .a = l->prule(l,t) };
	Circle collision_area = { .origin = global.plr.pos };

	for(t += l->collision_step; t <= min(t_end,t_death); t += l->collision_step) {
		float t1 = t - l->timespan / 2; // i have no idea
		float tail = l->timespan / 1.9;
		float widthfac = -0.75 / pow(tail, 2) * (t1 - tail) * (t1 + tail);
		widthfac = max(0.25, pow(widthfac, l->width_exponent));

		segment.b = l->prule(l, t);
		collision_area.radius = widthfac * l->width * 0.5 + 1;

		if(lineseg_circle_intersect(segment, collision_area) >= 0) {
			return true;
		}

		if(!grazed && !(global.frames % 7) && global.frames - abs(global.plr.recovery) > 0) {
			collision_area.radius = l->width * 2+8;
			float f = lineseg_circle_intersect(segment, collision_area);

			if(f >= 0) {
				player_graze(&global.plr, segment.a + f * (segment.b - segment.a), 7, 5);
				grazed = true;
			}
		}

		segment.a = segment.b;
	}

	segment.b = l->prule(l, min(t_end, t_death));
	collision_area.radius = l->width * 0.5; // WTF: what is this sorcery?

	return lineseg_circle_intersect(segment, collision_area) >= 0;
}

bool laser_intersects_circle(Laser *l, Circle circle) {
	// TODO: lots of copypasta from the function above here, maybe refactor both somehow.

	float t_end = (global.frames - l->birthtime) * l->speed + l->timeshift; // end of the laser based on length
	float t_death = l->deathtime * l->speed + l->timeshift; // end of the laser based on lifetime
	float t = t_end - l->timespan;

	if(t < 0) {
		t = 0;
	}

	LineSegment segment = { .a = l->prule(l, t) };
	double orig_radius = circle.radius;

	for(t += l->collision_step; t <= min(t_end, t_death); t += l->collision_step) {
		float t1 = t - l->timespan / 2; // i have no idea
		float tail = l->timespan / 1.9;
		float widthfac = -0.75 / pow(tail, 2) * (t1 - tail) * (t1 + tail);
		widthfac = max(0.25, pow(widthfac, l->width_exponent));

		segment.b = l->prule(l, t);
		circle.radius = orig_radius + widthfac * l->width * 0.5 + 1;

		if(lineseg_circle_intersect(segment, circle) >= 0) {
			return true;
		}

		segment.a = segment.b;
	}

	segment.b = l->prule(l, min(t_end, t_death));
	circle.radius = orig_radius + l->width * 0.5; // WTF: what is this sorcery?

	return lineseg_circle_intersect(segment, circle) >= 0;
}

complex las_linear(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("lasers/linear");
		l->collision_step = max(3,l->timespan/10);
		return 0;
	}

	return l->pos + l->args[0]*t;
}

complex las_accel(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("lasers/accelerated");
		return 0;
	}

	return l->pos + l->args[0]*t + 0.5*l->args[1]*t*t;
}

complex las_weird_sine(Laser *l, float t) {             // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// XXX: this used to be called "las_sine", but it's actually not a proper sine wave
	// do we even still need this?

	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("lasers/weird_sine");
		return 0;
	}

	double s = (l->args[2] * t + l->args[3]);
	return l->pos + cexp(I * (carg(l->args[0]) + l->args[1] * sin(s) / s)) * t * cabs(l->args[0]);
}

complex las_sine(Laser *l, float t) {               // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// this is actually shaped like a sine wave

	if(t == EVENT_BIRTH) {
		l->shader = r_shader_get_optional("lasers/sine");
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
		l->shader = r_shader_get_optional("lasers/sine_expanding");
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
		l->shader = r_shader_get_optional("lasers/turning");
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
		l->shader = r_shader_get_optional("lasers/circle");
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
