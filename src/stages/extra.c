/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "extra.h"
#include "coroutine.h"
#include "global.h"
#include "common_tasks.h"
#include "util/glm.h"
#include "stagedraw.h"
#include "resource/model.h"
#include "stagetext.h"
#include "stageutils.h"
#include "enemy_classes.h"

TASK(glider_bullet, {
	cmplx pos; double dir; double spacing; int interval;
}) {
	const int nproj = 5;
	const int nstep = 4;
	BoxedProjectile projs[nproj];

	const cmplx positions[][5] = {
		{1+I, -1, 1, -I, 1-I},
		{2, I, 1, -I, 1-I},
		{2, 1+I, 2-I, -I, 1-I},
		{2, 0, 2-I, 1-I, 1-2*I},
	};

	cmplx trans = cdir(ARGS.dir+1*M_PI/4)*ARGS.spacing;

	for(int i = 0; i < nproj; i++) {
		projs[i] = ENT_BOX(PROJECTILE(
			.pos = ARGS.pos+positions[0][i]*trans,
			.proto = pp_ball,
			.color = RGBA(0,0,1,1),
		));
	}


	for(int step = 0;; step++) {
		int cur_step = step%nstep;
		int next_step = (step+1)%nstep;

		int dead_count = 0;
		for(int i = 0; i < nproj; i++) {
			Projectile *p = ENT_UNBOX(projs[i]);
			if(p == NULL) {
				dead_count++;
			} else {
				p->move.retention = 1;
				p->move.velocity = -(positions[cur_step][i]-(1-I)*(cur_step==3)-positions[next_step][i])*trans/ARGS.interval;
			}
		}
		if(dead_count == nproj) {
			return;
		}
		WAIT(ARGS.interval);
	}
}

TASK(glider_fairy, {
	double hp; cmplx pos; cmplx dir;
}) {
	Enemy *e = TASK_BIND(espawn_big_fairy(VIEWPORT_W/2-10*I, ITEMS(
		.power = 3,
		.points = 5,
	)));

	YIELD;

	for(int i = 0; i < 80; ++i) {
		e->pos += cnormalize(ARGS.pos-e->pos)*2;
		YIELD;
	}

	for(int i = 0; i < 3; i++) {
		double aim = carg(global.plr.pos - e->pos);
		INVOKE_TASK(glider_bullet, e->pos, aim-0.7, 20, 6);
		INVOKE_TASK(glider_bullet, e->pos, aim, 25, 3);
		INVOKE_TASK(glider_bullet, e->pos, aim+0.7, 20, 6);
		WAIT(80-20*i);
	}

	for(;;) {
		e->pos += 2*(creal(e->pos)-VIEWPORT_W/2 > 0)-1;
		YIELD;
	}
}

/*
 * Obliteration “Infinity Network”
 *
 * WARNING unfinished crap code!
 */

static int polkadot_circle_approx_size_upper_bound(int subdiv) {
	// conservative estimate based on circle area formula
	// actual number of elements will be lower
	return ceil(M_PI * pow((subdiv + 1) * 0.5, 2));
}

static int polkadot_offset_cmp(const void *p1, const void *p2) {
	const cmplx *o1 = p1;
	const cmplx *o2 = p2;
	real d1 = cabs(*o1);
	real d2 = cabs(*o2);

	if(d1 < d2) { return -1; }
	if(d2 < d1) { return  1; }

	// stabilize sort
	if(p1 < p2) { return -1; }
	return 1;
}

static int make_polkadot_circle(int subdiv, cmplx out_offsets[]) {
	int c = 0;
	cmplx origin = CMPLX(0.5, 0.5);

	for(int i = 0; i <= subdiv; ++i) {
		for(int j = 0; j <= subdiv; ++j) {
			cmplx ofs = ((j + 0.5 * (i & 1)) / subdiv) + I * (i / (real)subdiv) - origin;

			if(cabs(ofs) < 0.5) {
				out_offsets[c++] = ofs * 2;
			}
		}
	}

	qsort(out_offsets, c, sizeof(*out_offsets), polkadot_offset_cmp);
	return c;
}

static int build_path(int num_offsets, cmplx offsets[num_offsets], int path[num_offsets]) {
	for(int i = 0; i < num_offsets; ++i) {
		path[i] = i;
	}

	int first = rng_i32_range(0, num_offsets / 4);
	int tmp = path[first];
	path[first] = path[0];
	path[0] = tmp;

	int num_sorted = 1;
	int x = 0;

	// TODO compute these based on the subdivision factor
	real random_offset = 0.036;
	real max_dist = 0.2;

	while(num_sorted < num_offsets) {
		++x;
		cmplx ref = offsets[path[num_sorted - 1]] + rng_dir() * random_offset;
		real dist = 2;
		int nearest = num_offsets;

		for(int i = num_sorted; i < num_offsets; ++i) {
			real d = cabs(offsets[path[i]] - ref);
			if(d < dist) {
				dist = d;
				nearest = i;
			}
		}

		if(dist > max_dist) {
			return x;
		}

		assume(nearest < num_offsets);

		int tmp = path[num_sorted];
		path[num_sorted] = path[nearest];
		path[nearest] = tmp;

		++num_sorted;
	}

	return x;
}

typedef struct InfnetAnimData {
	BoxedProjectileArray *dots;
	cmplx *offsets;
	cmplx origin;
	real radius;
	int num_dots;
} InfnetAnimData;

TASK(infnet_lasers, { InfnetAnimData *infnet; }) {
	InfnetAnimData *infnet = ARGS.infnet;
	int path[infnet->num_dots];
	int path_size = build_path(infnet->num_dots, infnet->offsets, path);

	BoxedLaser lasers[path_size - 1];
	memset(&lasers, 0, sizeof(lasers));

	int lasers_spawned = 0;
	int lasers_alive = 0;

	while(!lasers_spawned || lasers_alive) {
		lasers_alive = 0;

		for(int i = 0; i < path_size - 1; ++i) {
			int index0 = path[i];
			int index1 = path[i + 1];

			Projectile *a = infnet->dots->size > index0 ? ENT_ARRAY_GET(infnet->dots, index0) : NULL;
			Projectile *b = infnet->dots->size > index1 ? ENT_ARRAY_GET(infnet->dots, index1) : NULL;

			if(a && b) {
				if(!lasers[i].ent) {
					int delay = 5 * i;
					Laser *l = create_laserline_ab(0, 0, 10, 60+delay, 180+delay, RGBA(1, 0.4, 0.1, 0));
					lasers[i] = ENT_BOX(l);
					++lasers_spawned;
				}

				Laser *l = ENT_UNBOX(lasers[i]);
				if(l) {
					l->pos = a->pos;
					l->args[0] = (b->pos - a->pos) * 0.005;

					if((global.frames - a->birthtime) % 8 == 0) {
						PARTICLE(
							.sprite = "stardust",
							.pos = a->pos,
							.color = color_mul_scalar(COLOR_COPY(&a->color), 0.5),
							.timeout = 10,
							.draw_rule = pdraw_timeout_scalefade(0.5, 1, 1, 0),
							.angle = rng_angle(),
						);
					}

					if((global.frames - b->birthtime) % 8 == 0) {
						PARTICLE(
							.sprite = "stardust",
							.pos = b->pos,
							.color = color_mul_scalar(COLOR_COPY(&b->color), 0.5),
							.timeout = 10,
							.draw_rule = pdraw_timeout_scalefade(0.5, 1, 1, 0),
							.angle = rng_angle(),
						);
					}

					// FIXME this optimization is not correct, because sometimes the laser may cross the viewport,
					// even though both endpoints are invisible. Maybe it's worth detecting this case.
					#if 0
					if(a->ent.draw_layer == LAYER_NODRAW && b->ent.draw_layer == LAYER_NODRAW) {
						l->ent.draw_layer = LAYER_NODRAW;
					} else {
						l->ent.draw_layer = LAYER_LASER_HIGH;
					}
					#endif
				}
			} else {
				Laser *l = lasers[i].ent ? ENT_UNBOX(lasers[i]) : NULL;
				if(l) {
					clear_laser(l, CLEAR_HAZARDS_FORCE);
				}
			}

			if(lasers[i].ent && ENT_UNBOX(lasers[i])) {
				++lasers_alive;
			}
		}

		YIELD;
	}

	log_debug("All lasers died!");
}

TASK(animate_infnet, { InfnetAnimData *infnet; }) {
	InfnetAnimData *infnet = ARGS.infnet;

	int i = 0;
	real t = 0;

	real mtime = 4000;
	real radius0 = hypot(VIEWPORT_W, VIEWPORT_H) * 2;
	real radius1 = 380;

	infnet->radius = radius0;

	do {
		YIELD;

		infnet->radius = lerp(radius0, radius1, fmin(1, t / mtime));

		ENT_ARRAY_FOREACH_COUNTER(infnet->dots, i, Projectile *dot, {
			dot->pos = infnet->radius * infnet->offsets[i] + infnet->origin;
			real rot_phase = (global.frames - dot->birthtime) / 300.0;
			// real color_phase = psin(t / 60.0 + M_PI * i / (infnet->num_dots - 1.0));
			dot->color = *color_lerp(RGBA(1, 0, 0, 0), RGBA(0, 1, 0, 0), pcos(rot_phase));

			cmplx pivot = 0.2 * cdir(t/30.0);
			infnet->offsets[i] -= pivot;
			infnet->offsets[i] *= cdir(M_PI/400 * sin(rot_phase));
			infnet->offsets[i] += pivot;

			if(projectile_in_viewport(dot)) {
				dot->flags &= ~PFLAG_NOCOLLISION;
				dot->ent.draw_layer = LAYER_BULLET;
			} else {
				dot->flags |= PFLAG_NOCOLLISION;
				dot->ent.draw_layer = LAYER_NODRAW;
			}
		});

		capproach_asymptotic_p(&infnet->origin, global.plr.pos, 0.001, 1e-9);
		t += 1;
	} while(i > 0);
}

TASK(infinity_net, {
	BoxedBoss boss;
	cmplx origin;
}) {
	Boss *boss = TASK_BIND(ARGS.boss);

	int spawntime = 120;
	int subdiv = 22;

	cmplx offsets[polkadot_circle_approx_size_upper_bound(subdiv)];
	int num_offsets = make_polkadot_circle(subdiv, offsets);
	log_debug("%i / %zi", num_offsets, ARRAY_SIZE(offsets));
	assume(num_offsets <= ARRAY_SIZE(offsets));

	DECLARE_ENT_ARRAY(Projectile, dots, num_offsets);

	InfnetAnimData infnet;
	infnet.offsets = offsets;
	infnet.num_dots = num_offsets;
	infnet.origin = ARGS.origin;
	infnet.dots = &dots;

	INVOKE_SUBTASK(animate_infnet, &infnet);

	for(int i = 0; i < num_offsets; ++i) {
		ENT_ARRAY_ADD(&dots, PROJECTILE(
			.proto = pp_ball,
			.color = color_lerp(RGBA(1, 0, 0, 0), RGBA(0, 0, 1, 0), i / (num_offsets - 1.0)),
			.pos = ARGS.origin + infnet.radius * offsets[i],
			.flags = PFLAG_INDESTRUCTIBLE | PFLAG_NOCLEAR | PFLAG_NOAUTOREMOVE,
			// .flags = PFLAG_NOCLEAR,
		));
		if(i % (num_offsets / spawntime) == 0) YIELD;
	}

	for(;;) {
		INVOKE_SUBTASK(infnet_lasers, &infnet);
		WAIT(170);
	}
}

TASK_WITH_INTERFACE(stagex_spell_infinity_network, BossAttack) {
	Boss *boss = INIT_BOSS_ATTACK();
	BEGIN_BOSS_ATTACK();

	INVOKE_SUBTASK(infinity_net, ENT_BOX(boss), (VIEWPORT_W+VIEWPORT_H*I)*0.5);

	STALL;
}

Boss *stagex_spawn_yumemi(cmplx pos) {
	Boss *yumemi = create_boss("Okazaki Yumemi", "yumemi", pos);
	boss_set_portrait(yumemi, "yumemi", NULL, "normal");
	yumemi->shadowcolor = *RGBA(0.5, 0.0, 0.22, 1);
	yumemi->glowcolor = *RGBA(0.30, 0.0, 0.12, 0);
	return yumemi;
}

TASK(stage_main) {
	YIELD;

	// WAIT(3900);
	STAGE_BOOKMARK(boss);

	global.boss = stagex_spawn_yumemi(VIEWPORT_W/2 + 180*I);

	boss_add_attack_task(global.boss, AT_SurvivalSpell, "Obliteration “Infinity Network”", 90, 80000, TASK_INDIRECT(BossAttack, stagex_spell_infinity_network), NULL);

	PlayerMode *pm = global.plr.mode;
	StageExPreBossDialogEvents *e;
	INVOKE_TASK_INDIRECT(StageExPreBossDialog, pm->dialog->StageExPreBoss, &e);
	INVOKE_TASK_WHEN(&e->music_changes, common_start_bgm, "stagexboss");
	WAIT_EVENT(&global.dialog->events.fadeout_began);

	boss_start_attack(global.boss, global.boss->attacks);

	WAIT_EVENT(&global.boss->events.defeated);

	for(int i = 0;;i++) {
		INVOKE_TASK_DELAYED(60, glider_fairy, 2000, CMPLX(VIEWPORT_W*(i&1), VIEWPORT_H*0.5), 3*I);
		WAIT(50+100*(i&1));
	}
}

static struct {
	float plr_yaw;
	float plr_pitch;
	float plr_influence;

	struct {
		float red_flash_intensity;
		float opacity;
		float exponent;
	} fog;

	struct {
		Framebuffer *tower_mask;
	} fb;

	float codetex_num_segments;
	float codetex_aspect[2];
	float tower_global_dissolution;
	float tower_partial_dissolution;
	float tower_spin;
} draw_data;

#define SEGMENT_PERIOD 6000
#define MAX_RANGE (32 * SEGMENT_PERIOD)

static uint extra_stairs_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 0};
	vec3 r = {0, 0, SEGMENT_PERIOD};

	maxrange /= 2;

	uint pnum = linear3dpos(s3d, pos, maxrange, p, r);

	for(uint i = 0; i < pnum; ++i) {
		stage_3d_context.pos_buffer[i][2] -= maxrange - r[2];
	}

	return pnum;
}

static void extra_stairs_draw(vec3 pos, bool is_mask) {
	r_state_push();
	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	r_mat_mv_scale(300,300,300);

	if(is_mask) {
		uint32_t tmp = float_to_bits(pos[2]);
		float tex_ofs_x = (float)(splitmix32(&tmp) / (double)UINT32_MAX);
		float tex_ofs_y = (float)(splitmix32(&tmp) / (double)UINT32_MAX);
		float tex_rot = (float)(splitmix32(&tmp) / (double)UINT32_MAX);
		float tex_scale_x = 0.7 + (float)(splitmix32(&tmp) / (double)UINT32_MAX) * 0.4;
		float tex_scale_y = 0.7 + (float)(splitmix32(&tmp) / (double)UINT32_MAX) * 0.4;
		float phase = (float)(splitmix32(&tmp) / (double)UINT32_MAX);
		r_mat_tex_push();
		r_mat_tex_translate(tex_ofs_x, tex_ofs_y, 0);
		r_mat_tex_rotate(tex_rot * M_PI * 2, 0, 0, 1);
		r_mat_tex_scale(tex_scale_x, tex_scale_y, 0);
		r_shader("extra_tower_mask");
		r_uniform_sampler("tex", "cell_noise");
		r_uniform_sampler("tex2", "stageex/dissolve_mask");
		r_uniform_float("time", global.frames/60.0f + phase * M_PI * 2.0f);
		r_uniform_float("dissolve", draw_data.tower_partial_dissolution * draw_data.tower_partial_dissolution);
		r_draw_model("tower_alt_uv");
		r_mat_tex_pop();
	} else {
		r_shader("tower_light");
		r_uniform_sampler("tex", "stage5/tower");
		r_uniform_vec3("lightvec", 0, 0, 0);
		r_uniform_vec4("color", 0.1, 0.1, 0.5, 1);
		r_uniform_float("strength", 0);
		r_draw_model("tower");
	}

	r_mat_mv_pop();
	r_state_pop();
}

static void extra_stairs_draw_solid(vec3 pos) {
	extra_stairs_draw(pos, false);
}

static void extra_stairs_draw_mask(vec3 pos) {
	extra_stairs_draw(pos, true);
}

TASK(animate_value, { float *val; float target; float rate; }) {
	while(*ARGS.val != ARGS.target) {
		fapproach_p(ARGS.val, ARGS.target, ARGS.rate);
		YIELD;
	}
}

TASK(animate_value_asymptotic, { float *val; float target; float rate; float epsilon; }) {
	if(ARGS.epsilon == 0) {
		ARGS.epsilon = 1e-5;
	}

	while(*ARGS.val != ARGS.target) {
		fapproach_asymptotic_p(ARGS.val, ARGS.target, ARGS.rate, ARGS.epsilon);
		YIELD;
	}
}

static void animate_bg_intro(void) {
	const float w = -0.005f;
	float transition = 0.0;
	float t = 0.0;

	stage_3d_context.cam.pos[2] = 2*SEGMENT_PERIOD/3;
	stage_3d_context.cam.rot.pitch = 40;
	stage_3d_context.cam.rot.roll = 10;

	float prev_x0 = stage_3d_context.cam.pos[0];
	float prev_x1 = stage_3d_context.cam.pos[1];

	for(int frame = 0;; ++frame) {
		float dt = 5.0f * (1.0f - transition);
		float rad = 2300.0f * glm_ease_back_in(glm_ease_sine_out(1.0f - transition));
		stage_3d_context.cam.rot.pitch = -10 + 50 * glm_ease_sine_in(1.0f - transition);
		stage_3d_context.cam.pos[0] += rad*cosf(-w*t) - prev_x0;
		stage_3d_context.cam.pos[1] += rad*sinf(-w*t) - prev_x1;
		stage_3d_context.cam.pos[2] += w*3000.0f/M_PI*dt;
		prev_x0 = stage_3d_context.cam.pos[0];
		prev_x1 = stage_3d_context.cam.pos[1];
		stage_3d_context.cam.rot.roll -= 180.0f/M_PI*w*dt;

		t += dt;

		if(frame > 60) {
			if(transition == 1.0f) {
				return;
			}

			if(frame == 61) {
				INVOKE_TASK_DELAYED(60, animate_value, &stage_3d_context.cam.vel[2], -200, 1);
			}

			fapproach_p(&transition, 1.0f, 1.0f/300.0f);
		}

		YIELD;
	}
}

static void animate_bg_descent(int anim_time) {
	const float max_dist = 800.0f;
	const float max_spin = RAD2DEG*0.01f;
	float dist = 0.0f;
	float target_dist = 0.0f;
	float spin = 0.0f;

	for(float a = 0; anim_time > 0; a += 0.01, --anim_time) {
		float r = smoothmin(dist, max_dist, max_dist * 0.5);

		stage_3d_context.cam.pos[0] =  r * cos(a);
		stage_3d_context.cam.pos[1] = -r * sin(a);
		// stage_3d_context.cam.rot.roll += spin * sin(0.4331*a);
		fapproach_asymptotic_p(&draw_data.tower_spin,  spin * sin(0.4331*a), 0.02, 1e-4);
		fapproach_p(&spin, max_spin, max_spin / 240.0f);

		fapproach_asymptotic_p(&dist, target_dist, 0.01, 1e-4);
		fapproach_asymptotic_p(&target_dist, 0, 0.03, 1e-4);

		if(rng_chance(0.02)) {
			target_dist += max_dist;
		}

		YIELD;
	}
}

static void animate_bg_midboss(int anim_time) {
	float camera_shift_rate = 0;

	// stagetext_add("Midboss time!", CMPLX(VIEWPORT_W, VIEWPORT_H)/2, ALIGN_CENTER, res_font("big"), RGB(1,1,1), 0, 120, 30, 30);

	while(anim_time-- > 0) {
		fapproach_p(&camera_shift_rate, 1, 1.0f/120.0f);
		fapproach_asymptotic_p(&draw_data.plr_influence, 0, 0.01, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.vel[2], -64, 0.02, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.rot.pitch, 30, 0.01 * camera_shift_rate, 1e-4);

		float a = glm_rad(stage_3d_context.cam.rot.roll + 67.5);
		cmplxf p = cdir(a) * -2200;
		fapproach_asymptotic_p(&stage_3d_context.cam.pos[0], crealf(p), 0.01 * camera_shift_rate, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.pos[1], cimagf(p), 0.01 * camera_shift_rate, 1e-4);

		YIELD;
	}
}

static void animate_bg_post_midboss(int anim_time) {
	float center_distance = -2200;
	float camera_shift_rate = 0;

	// stagetext_add("Midboss defeated!", CMPLX(VIEWPORT_W, VIEWPORT_H)/2, ALIGN_CENTER, res_font("big"), RGB(1,1,1), 0, 120, 30, 30);

	while(anim_time-- > 0) {
		fapproach_p(&camera_shift_rate, 1, 1.0f/120.0f);
		fapproach_asymptotic_p(&draw_data.plr_influence, 1, 0.01, 1e-4);
		fapproach_p(&stage_3d_context.cam.vel[2], -300, 1);
		fapproach_asymptotic_p(&center_distance, 0, 0.03, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.rot.pitch, -10, 0.01 * camera_shift_rate, 1e-4);

		float a = glm_rad(stage_3d_context.cam.rot.roll + 67.5);
		cmplxf p = cdir(a) * center_distance;
		fapproach_asymptotic_p(&stage_3d_context.cam.pos[0], crealf(p), 0.01 * camera_shift_rate, 1e-4);
		fapproach_asymptotic_p(&stage_3d_context.cam.pos[1], cimagf(p), 0.01 * camera_shift_rate, 1e-4);

		YIELD;
	}
}

TASK(animate_bg, NO_ARGS) {
	draw_data.fog.exponent = 8;
	INVOKE_TASK_DELAYED(140, animate_value, &draw_data.fog.opacity, 1.0f, 1.0f/80.0f);
	INVOKE_TASK_DELAYED(140, animate_value_asymptotic, &draw_data.fog.exponent, 24.0f, 0.004f);
	INVOKE_TASK(animate_value, &draw_data.plr_influence, 1.0f, 1.0f/120.0f);
	animate_bg_intro();
	INVOKE_TASK_DELAYED(520, animate_value, &draw_data.fog.red_flash_intensity, 1.0f, 1.0f/320.0f);
	INVOKE_TASK_DELAYED(200, animate_value, &stage_3d_context.cam.vel[2], -300, 1);
	animate_bg_descent(60 * 20);
	animate_bg_midboss(60 * 10);
	animate_bg_post_midboss(60 * 7);
	INVOKE_TASK_DELAYED(120, animate_value, &draw_data.tower_partial_dissolution, 1.0f, 1.0f/600.0f);
	animate_bg_descent(60 * 20);
	INVOKE_TASK(animate_value_asymptotic, &draw_data.fog.exponent, 8.0f, 0.01f);
	INVOKE_TASK(animate_value, &draw_data.tower_global_dissolution, 1.0f, 1.0f/180.0f);
	INVOKE_TASK(animate_value_asymptotic, &stage_3d_context.cam.pos[0], 0, 0.02, 1e-4);
	INVOKE_TASK(animate_value_asymptotic, &stage_3d_context.cam.pos[1], 0, 0.02, 1e-4);
}

static void extra_begin_3d(void) {
	r_mat_proj_perspective(STAGE3D_DEFAULT_FOVY, STAGE3D_DEFAULT_ASPECT, 1700, MAX_RANGE);
	stage_3d_context.cam.rot.pitch += draw_data.plr_pitch;
	stage_3d_context.cam.rot.yaw += draw_data.plr_yaw;
}

static void extra_end_3d(void) {
	stage_3d_context.cam.rot.pitch -= draw_data.plr_pitch;
	stage_3d_context.cam.rot.yaw -= draw_data.plr_yaw;
}

static void render_mask(Framebuffer *fb) {
	r_state_push();
	r_mat_proj_push();
	extra_begin_3d();
	r_enable(RCAP_DEPTH_TEST);

	r_framebuffer(fb);
	r_clear(CLEAR_ALL, RGBA(1, 1, 1, draw_data.tower_global_dissolution), 1);

	r_mat_mv_push();
	stage3d_apply_transforms(&stage_3d_context, *r_mat_mv_current_ptr());
	stage3d_draw_segment(&stage_3d_context, extra_stairs_pos, extra_stairs_draw_mask, MAX_RANGE);
	r_mat_mv_pop();

	extra_end_3d();
	r_mat_proj_pop();
	r_state_pop();
}

static void set_bg_uniforms(void) {
	r_uniform_sampler("background_tex", "stageex/bg");
	r_uniform_sampler("background_binary_tex", "stageex/bg_binary");
	r_uniform_sampler("code_tex", "stageex/code");
	r_uniform_vec4("code_tex_params",
		draw_data.codetex_aspect[0],
		draw_data.codetex_aspect[1],
		draw_data.codetex_num_segments,
		1.0f / draw_data.codetex_num_segments
	);
	r_uniform_vec2("dissolve",
		1 - draw_data.tower_global_dissolution,
		(1 - draw_data.tower_global_dissolution) * (1 - draw_data.tower_global_dissolution)
	);
	r_uniform_float("time", global.frames / 60.0f);
}

static void extra_begin(void) {
	memset(&draw_data, 0, sizeof(draw_data));

	FBAttachmentConfig a[2] = { 0 };
	a[0].attachment = FRAMEBUFFER_ATTACH_COLOR0;
	a[0].tex_params.type = TEX_TYPE_RGBA_16;
	a[0].tex_params.filter.min = TEX_FILTER_LINEAR;
	a[0].tex_params.filter.mag = TEX_FILTER_LINEAR;
	a[0].tex_params.width = VIEWPORT_W;
	a[0].tex_params.height = VIEWPORT_H;
	a[0].tex_params.wrap.s = TEX_WRAP_MIRROR;
	a[0].tex_params.wrap.t = TEX_WRAP_MIRROR;

	a[1].attachment = FRAMEBUFFER_ATTACH_DEPTH;
	a[1].tex_params.type = TEX_TYPE_DEPTH;
	a[1].tex_params.filter.min = TEX_FILTER_NEAREST;
	a[1].tex_params.filter.mag = TEX_FILTER_NEAREST;
	a[1].tex_params.width = VIEWPORT_W;
	a[1].tex_params.height = VIEWPORT_H;
	a[1].tex_params.wrap.s = TEX_WRAP_MIRROR;
	a[1].tex_params.wrap.t = TEX_WRAP_MIRROR;

	draw_data.fb.tower_mask = stage_add_background_framebuffer("Tower mask", 0.25, 0.5, ARRAY_SIZE(a), a);
	stage3d_init(&stage_3d_context, 16);

	SDL_RWops *stream = vfs_open("res/gfx/stageex/code.num_slices", VFS_MODE_READ);

	if(!stream) {
		log_fatal("VFS error: %s", vfs_get_error());
	}

	char buf[32];
	SDL_RWgets(stream, buf, sizeof(buf));
	draw_data.codetex_num_segments = strtol(buf, NULL, 0);
	SDL_RWclose(stream);

	Texture *tex_code = res_texture("stageex/code");
	uint w, h;
	r_texture_get_size(tex_code, 0, &w, &h);

	// FIXME: this doesn't seem right, i don't know what the fuck i'm doing!
	float viewport_aspect = (float)VIEWPORT_W / (float)VIEWPORT_H;
	float seg_w = w / draw_data.codetex_num_segments;
	float seg_h = h;
	float seg_aspect = seg_w / seg_h;
	draw_data.codetex_aspect[0] = 1;
	draw_data.codetex_aspect[1] = 0.5/(seg_aspect/viewport_aspect);

	INVOKE_TASK(animate_bg);

	INVOKE_TASK(stage_main);
}

static void extra_end(void) {
	stage3d_shutdown(&stage_3d_context);
}

static void extra_update(void) {
	stage3d_update(&stage_3d_context);
	stage_3d_context.cam.rot.roll += draw_data.tower_spin;
	float yaw   = 10.0f * (crealf(global.plr.pos) / VIEWPORT_W - 0.5f) * draw_data.plr_influence;
	float pitch = 10.0f * (cimagf(global.plr.pos) / VIEWPORT_H - 0.5f) * draw_data.plr_influence;
	fapproach_asymptotic_p(&draw_data.plr_yaw,   yaw,   0.03, 1e-4);
	fapproach_asymptotic_p(&draw_data.plr_pitch, pitch, 0.03, 1e-4);
}

static bool should_draw_tower(void) {
	return draw_data.tower_global_dissolution < 0.999;
}

static bool should_draw_tower_postprocess(void) {
	return should_draw_tower() && draw_data.tower_partial_dissolution > 0;
}

static void extra_draw(void) {
	if(should_draw_tower()) {
		extra_begin_3d();

		Stage3DSegment segs[] = {
			{ extra_stairs_draw_solid, extra_stairs_pos },
		};

		stage3d_draw(&stage_3d_context, MAX_RANGE, ARRAY_SIZE(segs), segs);
		extra_end_3d();
	} else {
		r_state_push();
		r_mat_proj_push_ortho(VIEWPORT_W, VIEWPORT_H);
		r_mat_mv_push_identity();
		r_disable(RCAP_DEPTH_TEST);
		r_shader("extra_bg");
		set_bg_uniforms();
		r_mat_mv_scale(VIEWPORT_W, VIEWPORT_H, 1);
		r_mat_mv_translate(0.5, 0.5, 0);
		r_draw_quad();
		r_mat_mv_pop();
		r_mat_proj_pop();
		r_state_pop();
	}
}

static bool extra_postprocess_tower_mask(Framebuffer *fb) {
	if(!should_draw_tower_postprocess()) {
		return false;
	}

	Framebuffer *mask_fb = draw_data.fb.tower_mask;
	render_mask(mask_fb);

	r_state_push();
	r_enable(RCAP_DEPTH_TEST);
	r_enable(RCAP_DEPTH_WRITE);
	r_depth_func(DEPTH_ALWAYS);
	r_shader("extra_tower_apply_mask");
	r_uniform_sampler("mask_tex", r_framebuffer_get_attachment(mask_fb, FRAMEBUFFER_ATTACH_COLOR0));
	r_uniform_sampler("depth_tex", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	set_bg_uniforms();
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_state_pop();

	return true;
}

static bool extra_postprocess_copy_depth(Framebuffer *fb) {
	if(should_draw_tower_postprocess()) {
		r_state_push();
		r_enable(RCAP_DEPTH_TEST);
		r_depth_func(DEPTH_ALWAYS);
		r_blend(BLEND_NONE);
		r_shader("copy_depth");
		draw_framebuffer_attachment(fb, VIEWPORT_W, VIEWPORT_H, FRAMEBUFFER_ATTACH_DEPTH);
		r_state_pop();
	}

	return false;
}

static bool extra_postprocess_fog(Framebuffer *fb) {
	if(!should_draw_tower()) {
		return false;
	}

	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));

	float t = global.frames/15.0f;

	Color c;
	c.r = psin(t);
	c.g = 0.25f * c.r * powf(pcosf(t), 1.25f);
	c.b = c.r * c.g;

	float w = 1.0f - sqrtf(erff(8.0f * c.g));
	w = lerpf(1.0f, w, draw_data.fog.red_flash_intensity);
	c.b += w*w*w*w*w;

	c.r = lerpf(c.r, 1.0f, w);
	c.g = lerpf(c.g, 1.0f, w);
	c.b = lerpf(c.b, 1.0f, w);
	c.a = 1.0f;

	color_lerp(&c, RGBA(1, 1, 1, 1), 0.2);
	color_mul_scalar(&c, draw_data.fog.opacity);

	r_uniform_vec4_rgba("fog_color", &c);
	r_uniform_float("start", 0.0);
	r_uniform_float("end", 1.0);
	r_uniform_float("exponent", draw_data.fog.exponent);
	r_uniform_float("sphereness", 0.0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

void extra_draw_yumemi_portrait_overlay(SpriteParams *sp) {
	sp->sprite = NULL;
	sp->sprite_ptr = res_sprite("dialog/yumemi_misc_code_mask");
	sp->shader = NULL;
	sp->shader_ptr = res_shader("sprite_yumemi_overlay");
	sp->aux_textures[0] = res_texture("stageex/code");
	sp->shader_params = &(ShaderCustomParams) {
		global.frames / 60.0,
		draw_data.codetex_aspect[0],
		draw_data.codetex_aspect[1],
		draw_data.codetex_num_segments,
	};

	r_draw_sprite(sp);
}

static void extra_preload(ResourceGroup *rg) {
	res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT,
		"cell_noise",
		"stageex/bg",
		"stageex/bg_binary",
		"stageex/code",
		"stageex/dissolve_mask",
	NULL);
	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"copy_depth",
		"extra_bg",
		"extra_tower_apply_mask",
		"extra_tower_mask",
		"zbuf_fog",
	NULL);
	res_group_preload(rg, RES_MODEL, RESF_DEFAULT,
		"tower",
		"tower_alt_uv",
	NULL);
}

StageProcs extra_procs = {
	.begin = extra_begin,
	.end = extra_end,
	.update = extra_update,
	.draw = extra_draw,
	.preload = extra_preload,
	.shader_rules = (ShaderRule[]) {
		extra_postprocess_tower_mask,
		extra_postprocess_copy_depth,
		extra_postprocess_fog,
		NULL
	},
};
