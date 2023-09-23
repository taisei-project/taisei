/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "laser.h"
#include "internal.h"
#include "draw.h"

#include "global.h"
#include "list.h"
#include "stageobjects.h"
#include "stagedraw.h"
#include "renderer/api.h"
#include "resource/model.h"
#include "util/fbmgr.h"
#include "util/glm.h"
#include "video.h"

typedef struct LaserSamplingParams {
	uint num_samples;
	float time_shift;
	float time_step;
} LaserSamplingParams;

void lasers_init(void) {
	laserintern_init();
	laserdraw_init();
}

void lasers_shutdown(void) {
	laserdraw_shutdown();
	laserintern_shutdown();
}

Laser *create_laser(
	cmplx pos, float time, float deathtime, const Color *color,
	LaserPosRule prule,
	cmplx a0, cmplx a1, cmplx a2, cmplx a3
) {
	Laser *l = objpool_acquire(&stage_object_pools.lasers);
	alist_push(&global.lasers, l);

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
	l->width = 10;
	l->width_exponent = 1.0;
	l->speed = 1;
	l->collision_active = true;

	l->ent.draw_layer = LAYER_LASER_HIGH;
	l->ent.draw_func = laserdraw_ent_drawfunc;
	ent_register(&l->ent, ENT_TYPE_ID(Laser));

	l->prule(l, EVENT_BIRTH);

	return l;
}

Laser *create_laserline(cmplx pos, cmplx dir, float charge, float dur, const Color *clr) {
	return create_laserline_ab(pos, (pos)+(dir)*VIEWPORT_H*1.4/cabs(dir), cabs(dir), charge, dur, clr);
}

Laser *create_laserline_ab(cmplx a, cmplx b, float width, float charge, float dur, const Color *clr) {
	float lt = 200;
	cmplx m = (b - a) / lt;
	Laser *l = create_laser(a, lt, dur, clr, las_linear, m, 0, 0, 0);
	INVOKE_TASK(laser_charge,
		.laser = ENT_BOX(l),
		.charge_delay = charge,
		.target_width = width,
	);
	return l;
}

void laserline_set_ab(Laser *l, cmplx a, cmplx b) {
	l->pos = a;
	l->args[0] = (b - a) / l->timespan;
}

void laserline_set_posdir(Laser *l, cmplx pos, cmplx dir) {
	laserline_set_ab(l, pos, pos + VIEWPORT_H * cnormalize(dir));
}

static void *_delete_laser(ListAnchor *lasers, List *laser, void *arg) {
	Laser *l = (Laser*)laser;
	ent_unregister(&l->ent);
	objpool_release(&stage_object_pools.lasers, alist_unlink(lasers, laser));
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

static bool laser_prepare_sampling_params(Laser *l, float step, LaserSamplingParams *out_params) {
	float t;
	int c;

	c = l->timespan;
	t = (global.frames - l->birthtime) * l->speed - l->timespan + l->timeshift;

	if(t + l->timespan > l->deathtime + l->timeshift) {
		c += l->deathtime + l->timeshift - (t + l->timespan);
	}

	if(t < 0) {
		c += t;
		t = 0;
	}

	if(c <= 0) {
		return false;
	}

	out_params->num_samples = c / step;
	out_params->time_shift = t;
	out_params->time_step = step;

	return true;
}

static float calc_sample_width(
	Laser *l, float sample, float half_samples, float width_factor, float tail
) {
	float mid_ofs = sample - half_samples;
	float w = width_factor * (mid_ofs - tail) * (mid_ofs + tail);

	if(l->width_exponent != 1.0) {
		w = powf(w, l->width_exponent);
	}

	return 0.75f * l->width * w;
}

static void laserseg_flip(LaserSegment *s) {
	SWAP(s->pos.a, s->pos.b);
	SWAP(s->time.a, s->time.b);
	SWAP(s->width.a, s->width.b);
}

static int quantize_laser(Laser *l) {
	// Break the laser curve into small line segments, simplify and cull them,
	// compute the bounding box.

	l->_internal.segments_ofs = lintern.segments.num_elements;
	l->_internal.num_segments = 0;

	LaserSamplingParams sp;

	if(!laser_prepare_sampling_params(l, 0.5f, &sp)) {
		l->_internal.bbox.top_left.as_cmplx = 0;
		l->_internal.bbox.bottom_right.as_cmplx = 0;
		return 0;
	}

	// Precomputed magic parameters for width calculation
	float half_samples = sp.num_samples * 0.5;
	float tail = sp.num_samples / 1.6;
	float width_factor = -1 / (tail * tail);

	// Maximum value of `1 - cos(angle)` between two curve segments to reduce to straight lines
	const float thres_angular = 1e-4;
	// Maximum laser-time sample difference between two segment points (for width interpolation)
	const float thres_temporal = sp.num_samples / 16.0;
	// These values should be kept as high as possible without introducing artifacts.

	// Time value of current sample
	float t = sp.time_shift;

	// Time value of last included sample
	float t0 = t;

	// Points of the current line segment
	// Begin constructing at t0
	// WARNING: these must be double precision to prevent cross-platform replay desync
	cmplx a, b;
	a = l->prule(l, t0);

	// Width value of the last included sample
	// Initialized to the width at t0
	float w0 = calc_sample_width(l, 0, half_samples, width_factor, tail);

	// Already sampled the first point, so shift
	t += sp.time_step;

	// Vector from A to B of the last included segment, and its squared length.
	cmplxf v0 = a - l->prule(l, t0 - sp.time_step);
	float v0_abs2 = cabs2f(v0);

	float viewmargin = l->width * 0.5f;
	FloatRect viewbounds = { .extent = VIEWPORT_SIZE };
	viewbounds.w += viewmargin * 2.0f;
	viewbounds.h += viewmargin * 2.0f;
	viewbounds.x -= viewmargin;
	viewbounds.y -= viewmargin;

	FloatOffset top_left, bottom_right;
	top_left.as_cmplx = a;
	bottom_right.as_cmplx = a;

	for(uint i = 1; i < sp.num_samples; ++i, t += sp.time_step) {
		b = l->prule(l, t);

		if(i < sp.num_samples - 1 && (t - t0) < thres_temporal) {
			cmplxf v1 = b - a;

			// dot(a, b) == |a|*|b|*cos(theta)
			float dot = cdotf(v0, v1);
			float norm2 = v0_abs2 * cabs2f(v1);

			if(norm2 == 0.0f) {
				// degenerate case
				continue;
			}

			float norm = sqrtf(norm2);
			float cosTheta = dot / norm;
			float d = 1.0f - fabsf(cosTheta);

			if(d < thres_angular) {
				continue;
			}
		}

		float w = calc_sample_width(l, i, half_samples, width_factor, tail);

		float xa = re(a);
		float ya = im(a);
		float xb = re(b);
		float yb = im(b);

		bool visible =
			(xa > viewbounds.x && xa < viewbounds.w && ya > viewbounds.y && ya < viewbounds.h) ||
			(xb > viewbounds.x && xb < viewbounds.w && yb > viewbounds.y && yb < viewbounds.h);

		if(visible) {
			LaserSegment *seg = dynarray_append(&lintern.segments, {
				.pos   = {   a,  b },
				.width = {  w0,  w },
				.time  = { -t0, -t },
			});

			if(w < w0) {
				// NOTE: the uneven capsule distance function may not work correctly in cases where
				//       radius(A) > radius(B) and circle A contains circle B.
				laserseg_flip(seg);
			}

			assert(seg->width.a <= seg->width.b);

			top_left.x     = min(    top_left.x, min(xa, xb));
			top_left.y     = min(    top_left.y, min(ya, yb));
			bottom_right.x = max(bottom_right.x, max(xa, xb));
			bottom_right.y = max(bottom_right.y, max(ya, yb));
		}

		t0 = t;
		w0 = w;
		v0 = b - a;
		v0_abs2 = cabs2f(v0);
		a = b;
	}

	float aabb_margin = LASER_SDF_RANGE + l->width * 0.5f;
	top_left.as_cmplx -= aabb_margin * (1.0f + I);
	bottom_right.as_cmplx += aabb_margin * (1.0f + I);

	l->_internal.bbox.top_left = top_left;
	l->_internal.bbox.bottom_right = bottom_right;

	l->_internal.num_segments = lintern.segments.num_elements - l->_internal.segments_ofs;
	return l->_internal.num_segments;
}

static bool laser_collision(Laser *l, Player *plr);

typedef struct LaserTraceState {
	Laser *l;
	LaserTraceFunc func;
	void *userdata;
	LaserSegment *seg;
	LaserTraceSample sample;
	cmplx p;
	real step;
	real accum;
	real inverse_seglen;
} LaserTraceState;

static void *laser_trace_dispatch(LaserTraceState *st) {
	return st->func(st->l, &st->sample, st->userdata);
}

static void *laser_trace_advance(LaserTraceState *st, cmplx v, real l) {
	real full = l;
	l = fmin(l, st->step - st->accum);

	st->accum += l;
	st->sample.segment_param += l * st->inverse_seglen;
	st->p += v * l;

	if(st->accum >= st->step) {
		st->accum -= st->step;
		st->sample.pos = st->p;

		void *result = laser_trace_dispatch(st);

		if(result) {
			return result;
		}
	}

	if(full - l > 0) {
		return laser_trace_advance(st, v, full - l);
	}

	return NULL;
}

void *laser_trace(Laser *l, real step, LaserTraceFunc trace, void *userdata) {
	if(l->_internal.num_segments < 1) {
		return NULL;
	}

	int first_seg = l->_internal.segments_ofs;
	int last_seg = first_seg + l->_internal.num_segments - 1;

	LaserTraceState st = {
		.l = l,
		.step = step,
		.func = trace,
		.userdata = userdata,
		.p = dynarray_get(&lintern.segments, first_seg).pos.a,
	};

	void *result;

	cmplx prev_endpos = INFINITY;

	for(int seg = first_seg; seg <= last_seg; ++seg) {
		// NOTE: deliberate copy
		LaserSegment s = dynarray_get(&lintern.segments, seg);

		if(prev_endpos != s.pos.a && prev_endpos == s.pos.b) {
			// Segment was flipped (see quantize_laser); undo it
			laserseg_flip(&s);
		}

		cmplx v = s.pos.b - s.pos.a;
		real len = cabs(v);
		st.inverse_seglen = 1 / len;
		v *= st.inverse_seglen;

		st.sample.segment = &s;
		st.sample.segment_param = 0;

		if(prev_endpos != s.pos.a) {
			// discontinuity, or first segment.
			st.p = s.pos.a;
			st.accum = 0;
			st.sample.discontinuous = true;
			st.sample.pos = st.p;
			result = laser_trace_dispatch(&st);

			if(result) {
				return result;
			}

			st.sample.discontinuous = false;
		}

		real pstep = step * 1;
		while(len >= pstep) {
			result = laser_trace_advance(&st, v, pstep);

			if(result) {
				return result;
			}

			len -= pstep;
		}

		laser_trace_advance(&st, v, len);
		prev_endpos = s.pos.b;
	}

	return NULL;
}

static void laser_clear_effect(Sprite *spr, cmplx p, cmplxf scale, const Color *clr) {
	int timeout = rng_irange(18, 24);
	cmplx v = rng_dir();
	v *= rng_range(0.4, 1.2);
	PARTICLE(
		.sprite_ptr = spr,
		.pos = p,
		.color = clr,
		.timeout = timeout,
		.move = move_linear(v),
		.draw_rule = pdraw_timeout_scalefade(1+I, 0.25+0.5i, 1, 0),
		.flags = PFLAG_NOREFLECT,
		.scale = scale,
	);
}

#define CLEAR_STEP 16

typedef struct LaserClearTraceCtx {
	struct {
		Sprite *spr;
		Color clr;
	} particle;

	struct {
		cmplx pos;
		float width;
	} prev;
} LaserClearTraceCtx;

static void *laser_clear_now_tracefunc(Laser *l, const LaserTraceSample *sample, void *userdata) {
	LaserClearTraceCtx *ctx = userdata;
	cmplx pos = sample->pos;
	float width = lerpf(sample->segment->width.a, sample->segment->width.b, sample->segment_param);
	create_clear_item(pos, l->clear_flags);

	if(!sample->discontinuous) {
		for(float f = 0.33; f < 0.9; f += 0.33) {
			cmplx ipos = clerp(ctx->prev.pos, pos, f);
			float iwidth = lerpf(ctx->prev.width, width, f);
			laser_clear_effect(
				ctx->particle.spr, ipos, iwidth / ctx->particle.spr->w, &ctx->particle.clr);
		}
	}

	laser_clear_effect(ctx->particle.spr, pos, width / ctx->particle.spr->w, &ctx->particle.clr);
	ctx->prev.pos = pos;
	ctx->prev.width = width;
	return NULL;
}

static void laser_clear_now(Laser *l) {
	LaserClearTraceCtx ctx;
	ctx.particle.spr = res_sprite("part/flare");
	ctx.particle.clr = l->color;
	color_mul(&ctx.particle.clr, RGBA(2, 2, 2, 0));
	color_add(&ctx.particle.clr, RGBA(0.1, 0.1, 0.1, 0));
	laser_trace(l, CLEAR_STEP, laser_clear_now_tracefunc, &ctx);
}

void process_lasers(void) {
	bool stage_cleared = stage_is_cleared();
	Player *plr = &global.plr;

	lintern.segments.num_elements = 0;

	/*
	 * NOTE: it's important to have two loops here, because something triggered from ent_damage()
	 * may try poking laser segment data before it's initialized by quantize_laser().
	 * For example, dying to a laser while having a surge field active will immediately trigger a
	 * discharge and try to cancel all lasers in a circle.
	 */

	for(Laser *laser = global.lasers.first, *next; laser; laser = next) {
		next = laser->next;

		if(global.frames - laser->birthtime > laser->deathtime + laser->timespan * laser->speed) {
			delete_laser(&global.lasers, laser);
			continue;
		}

		quantize_laser(laser);

		if(stage_cleared) {
			clear_laser(laser, CLEAR_HAZARDS_LASERS | CLEAR_HAZARDS_FORCE);
		}
	}

	for(Laser *laser = global.lasers.first, *next; laser; laser = next) {
		next = laser->next;

		if(laser->clear_flags & CLEAR_HAZARDS_LASERS) {
			laser_clear_now(laser);
			laser->deathtime = 0;
		} else if(laser_collision(laser, plr)) {
			ent_damage(&plr->ent, &(DamageInfo) { .type = DMG_ENEMY_SHOT });
		}
	}
}

static inline Rect laser_bbox_rect(Laser *l) {
	return (Rect) {
		l->_internal.bbox.top_left.as_cmplx,
		l->_internal.bbox.bottom_right.as_cmplx
	};
}

static bool laser_collision(Laser *l, Player *plr) {
	if(!laser_is_active(l)) {
		return false;
	}

	int num_segs = l->_internal.num_segments;

	if(num_segs < 1) {
		return false;
	}

	bool graze = global.frames >= l->next_graze;

	double graze_maxdist = 42;
	double graze_dist = graze_maxdist;
	cmplx graze_pos = 0;

	Rect bbox = laser_bbox_rect(l);

	if(graze) {
		cmplx graze_bbox_ofs = graze_dist * (1 + I);
		bbox.top_left -= graze_bbox_ofs;
		bbox.bottom_right += graze_bbox_ofs;
	}

	if(!point_in_rect(plr->pos, bbox)) {
		return false;
	}

	LaserSegment *segs = dynarray_get_ptr(&lintern.segments, l->_internal.segments_ofs);

	LineSegment plrmotion;
	cmplx plrpos = plr->pos;
	bool player_moved = false;

	if(plr->velocity != 0) {
		player_moved = true;
		plrmotion.a = plrpos - plr->velocity;
		plrmotion.b = plrpos;
	}

	for(int i = 0; i < num_segs; ++i) {
		LaserSegment *lseg = segs + i;
		LineSegment s = { lseg->pos.a, lseg->pos.b };

		if(player_moved && lineseg_lineseg_intersection(plrmotion, s, NULL)) {
			// Prevent phasing through laser beams
			return true;
		}

		UnevenCapsule c = {
			.pos = s,
			.radius.a = fmax(lseg->width.a * 0.5 - 4, 2),
			.radius.b = fmax(lseg->width.b * 0.5 - 4, 2),
		};

		double d = ucapsule_dist_from_point(plrpos, c);

		if(d < 0) {
			return true;
		}

		if(graze && d < graze_dist) {
			double f = lineseg_closest_factor(c.pos, plrpos);
			graze_pos = clerp(c.pos.a, c.pos.b, f);
			cmplx v = cnormalize(plrpos - graze_pos);
			graze_pos += 0.5 * clerp(lseg->width.a, lseg->width.b, f) * v;
			graze_dist = d;
		}

	}

	if(graze_dist < graze_maxdist) {
		player_graze(plr, graze_pos, 7, 5, &l->color);
		l->next_graze = global.frames + 4;
	}

	return false;
}

bool laser_intersects_ellipse(Laser *l, Ellipse ellipse) {
	// NOTE: This function does not take laser width into account.
	// It also can't test culled parts of the laser, because culling
	// is done at the quantization stage.
	// But surely this won't ever be a problem, right…?

	int num_segs = l->_internal.num_segments;

	if(num_segs < 1) {
		return false;
	}

	Rect e_bbox = ellipse_bbox(ellipse);
	Rect l_bbox = laser_bbox_rect(l);

	if(!rect_rect_intersect(e_bbox, l_bbox, true, true)) {
		return false;
	}

	LaserSegment *segs = dynarray_get_ptr(&lintern.segments, l->_internal.segments_ofs);

	for(int i = 0; i < num_segs; ++i) {
		LaserSegment *lseg = segs + i;
		LineSegment s = { lseg->pos.a, lseg->pos.b };

		if(lineseg_ellipse_intersect(s, ellipse)) {
			return true;
		}
	}

	return false;
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
		return 0;
	}

	return l->pos + l->args[0]*t;
}

cmplx las_accel(Laser *l, float t) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	return l->pos + l->args[0]*t + 0.5*l->args[1]*t*t;
}

cmplx las_sine(Laser *l, float t) {               // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase
	// this is actually shaped like a sine wave

	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx line_vel = l->args[0];
	cmplx line_dir = line_vel / cabs(line_vel);
	cmplx line_normal = im(line_dir) - I*re(line_dir);
	cmplx sine_amp = l->args[1];
	real sine_freq = re(l->args[2]);
	real sine_phase = re(l->args[3]);

	cmplx sine_ofs = line_normal * sine_amp * sin(sine_freq * t + sine_phase);
	return l->pos + t * line_vel + sine_ofs;
}

cmplx las_sine_expanding(Laser *l, float t) { // [0] = velocity; [1] = sine amplitude; [2] = sine frequency; [3] = sine phase

	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx velocity = l->args[0];
	real amplitude = re(l->args[1]);
	real frequency = re(l->args[2]);
	real phase = re(l->args[3]);

	real angle = carg(velocity);
	real speed = cabs(velocity);

	real s = (frequency * t + phase);
	return l->pos + cdir(angle + amplitude * sin(s)) * t * speed;
}

cmplx las_turning(Laser *l, float t) { // [0] = vel0; [1] = vel1; [2] r: turn begin time, i: turn end time
	if(t == EVENT_BIRTH) {
		return 0;
	}

	cmplx v0 = l->args[0];
	cmplx v1 = l->args[1];
	float begin = re(l->args[2]);
	float end = im(l->args[2]);

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

	real turn_speed = re(l->args[0]);
	real time_ofs = im(l->args[0]);
	real radius = re(l->args[1]);

	return l->pos + radius * cdir(turn_speed * (t + time_ofs));
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

DEFINE_EXTERN_TASK(laser_charge) {
	Laser *l = TASK_BIND(ARGS.laser);

	l->width = 0;
	l->collision_active = false;
	laser_make_static(l);

	float target_width = ARGS.target_width;
	float charge_delay = ARGS.charge_delay;

	// TODO: stop when done charging
	for(int t = 0;; ++t) {
		laser_charge(l, t, charge_delay, target_width);
		YIELD;
	}
}
