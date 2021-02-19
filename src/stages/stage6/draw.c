/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "draw.h"

#include "global.h"

MODERNIZE_THIS_FILE_AND_REMOVE_ME

static Stage6DrawData *stage6_draw_data;

Stage6DrawData *stage6_get_draw_data(void) {
	return NOT_NULL(stage6_draw_data);
}

void stage6_drawsys_init(void) {
	stage6_draw_data = calloc(1, sizeof(*stage6_draw_data));
	// TODO: make this background slightly less horribly inefficient
	stage3d_init(&stage_3d_context, 128);

	stage6_draw_data->fall_over.frames = 0;

	for(int i = 0; i < NUM_STARS; i++) {
		float x,y,z,r;

		x = y = z = 0;
		for(int c = 0; c < 10; c++) {
			x += nfrand();
			y += nfrand();
			z += nfrand();
		} // now x,y,z are approximately gaussian
		z = fabs(z);
		r = sqrt(x*x+y*y+z*z);
		// normalize them and it’s evenly distributed on a sphere
		stage6_draw_data->stars.position[3*i+0] = x/r;
		stage6_draw_data->stars.position[3*i+1] = y/r;
		stage6_draw_data->stars.position[3*i+2] = z/r;
	}

	stage_3d_context.cam.aspect = STAGE3D_DEFAULT_ASPECT; // FIXME
	stage_3d_context.cam.near = 100;
	stage_3d_context.cam.far = 9000;

	stage_3d_context.cx[1] = -230;
	stage_3d_context.crot[0] = 90;
	stage_3d_context.crot[2] = -40;

//	for testing
//	stage_3d_context.cx[0] = 80;
//	stage_3d_context.cx[1] = -215;
//	stage_3d_context.cx[2] = 295;
//	stage_3d_context.crot[0] = 90;
//	stage_3d_context.crot[2] = 381.415100;

	FBAttachmentConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	cfg.tex_params.type = TEX_TYPE_RGBA;
	cfg.tex_params.filter.min = TEX_FILTER_LINEAR;
	cfg.tex_params.filter.mag = TEX_FILTER_LINEAR;
	cfg.tex_params.wrap.s = TEX_WRAP_MIRROR;
	cfg.tex_params.wrap.t = TEX_WRAP_MIRROR;
	stage6_draw_data->baryon.fbpair.front = stage_add_background_framebuffer("Baryon FB 1", 0.25, 0.5, 1, &cfg);
	stage6_draw_data->baryon.fbpair.back = stage_add_background_framebuffer("Baryon FB 2", 0.25, 0.5, 1, &cfg);
	stage6_draw_data->baryon.aux_fb = stage_add_background_framebuffer("Baryon FB AUX", 0.25, 0.5, 1, &cfg);
}

void stage6_drawsys_shutdown(void) {
	stage3d_shutdown(&stage_3d_context);
	free(stage6_draw_data);
	stage6_draw_data = NULL;
}
uint stage6_towerwall_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, -220};
	vec3 r = {0, 0, 300};

	uint num = linear3dpos(s3d, pos, maxrange, p, r);

	for(uint i = 0; i < num; ++i) {
		if(s3d->pos_buffer[i][2] > 0) {
			s3d->pos_buffer[i][1] = -90000;
		}
	}

	return num;
}

void stage6_towerwall_draw(vec3 pos) {
	r_state_push();

	r_shader("tower_wall");
	r_uniform_sampler("tex", "stage6/towerwall");

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	r_mat_mv_scale(30,30,30);
	r_draw_model("towerwall");
	r_mat_mv_pop();

	r_state_pop();
}

static uint stage6_towertop_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	vec3 p = {0, 0, 70};
	return single3dpos(s3d, pos, maxrange, p);
}

static void stage6_towertop_draw(vec3 pos) {
	r_uniform_sampler("tex", "stage6/towertop");

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);
	r_mat_mv_scale(28,28,28);
	r_draw_model("towertop");
	r_mat_mv_pop();
}

static uint stage6_skysphere_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	return single3dpos(s3d, pos, maxrange, s3d->cx);
}


static void stage6_skysphere_draw(vec3 pos) {
	r_state_push();

	r_disable(RCAP_DEPTH_TEST);
	ShaderProgram *s = r_shader_get("stage6_sky");
	r_shader_ptr(s);

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2] - 30);
	r_mat_mv_scale(150, 150, 150);
	r_draw_model("skysphere");

	r_shader("sprite_default");

	for(int i = 0; i < NUM_STARS; i++) {
		r_mat_mv_push();
		float x = stage6_draw_data->stars.position[3*i+0], y = stage6_draw_data->stars.position[3*i+1], z = stage6_draw_data->stars.position[3*i+2];
		r_mat_mv_translate(x, y, z);
		r_mat_mv_rotate(acos(stage6_draw_data->stars.position[3*i+2]), -y, x, 0);
		r_mat_mv_scale(1.0/4000, 1.0/4000, 1.0/4000);
		r_draw_sprite(&(SpriteParams) {
			.sprite = "part/smoothdot",
			.color = RGBA_MUL_ALPHA(0.9, 0.9, 1.0, 0.8 * z),
		});
		r_mat_mv_pop();

	}

	r_mat_mv_pop();
	r_state_pop();
}

void stage6_draw(void) {
	Stage3DSegment segs[] = {
		{ stage6_skysphere_draw, stage6_skysphere_pos },
		{ stage6_towertop_draw, stage6_towertop_pos },
		{ stage6_towerwall_draw, stage6_towerwall_pos },
	};

	stage3d_draw(&stage_3d_context, 10000, ARRAY_SIZE(segs), segs);
}

static void draw_baryon_connector(cmplx a, cmplx b) {
	Sprite *spr = get_sprite("stage6/baryon_connector");
	r_draw_sprite(&(SpriteParams) {
		.sprite_ptr = spr,
		.pos = { creal(a + b) * 0.5, cimag(a + b) * 0.5 },
		.rotation.vector = { 0, 0, 1 },
		.rotation.angle = carg(a - b),
		.scale = { (cabs(a - b) - 70) / spr->w, 20 / spr->h },
	});
}

void baryon(Enemy *e, int t, bool render) {
	if(render) {
		// the center piece draws the nodes; applying the postprocessing effect is easier this way.
		return;
	}
}

static void draw_baryons(Enemy *bcenter, int t) {
	// r_color4(1, 1, 1, 0.8);

	r_mat_mv_push();
	r_mat_mv_translate(creal(bcenter->pos), cimag(bcenter->pos), 0);

	r_draw_sprite(&(SpriteParams) {
		.sprite = "stage6/baryon_center",
		.rotation.angle = DEG2RAD * 2 * t,
		.rotation.vector = { 0, 0, 1 },
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite = "stage6/baryon",
	});

	r_mat_mv_pop();

	r_color4(1, 1, 1, 1);

	Enemy *link0 = REF(creal(bcenter->args[1]));
	Enemy *link1 = REF(cimag(bcenter->args[1]));

	if(link0 && link1) {
		draw_baryon_connector(bcenter->pos, link0->pos);
		draw_baryon_connector(bcenter->pos, link1->pos);
	}

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == baryon) {
			r_draw_sprite(&(SpriteParams) {
				.pos = { creal(e->pos), cimag(e->pos) },
				.sprite = "stage6/baryon",
			});

			Enemy *n = REF(e->args[1]);

			if(n != NULL) {
				draw_baryon_connector(e->pos, n->pos);
			}
		}
	}
}

void baryon_center_draw(Enemy *bcenter, int t, bool render) {
	if(!render) {
		return;
	}

	if(config_get_int(CONFIG_POSTPROCESS) < 1) {
		draw_baryons(bcenter, t);
		return;
	}

	stage_draw_begin_noshake();
	r_state_push();

	r_shader("baryon_feedback");
	r_uniform_vec2("blur_resolution", 0.5*VIEWPORT_W, 0.5*VIEWPORT_H);
	r_uniform_float("hue_shift", 0);
	r_uniform_float("time", t/60.0);

	r_framebuffer(stage6_draw_data->baryon.aux_fb);
	r_blend(BLEND_NONE);
	r_uniform_vec2("blur_direction", 1, 0);
	r_uniform_float("hue_shift", 0.01);
	r_color4(0.95, 0.88, 0.9, 0.5);

	draw_framebuffer_tex(stage6_draw_data->baryon.fbpair.front, VIEWPORT_W, VIEWPORT_H);

	fbpair_swap(&stage6_draw_data->baryon.fbpair);

	r_framebuffer(stage6_draw_data->baryon.fbpair.back);
	r_uniform_vec2("blur_direction", 0, 1);
	r_color4(1, 1, 1, 1);
	draw_framebuffer_tex(stage6_draw_data->baryon.aux_fb, VIEWPORT_W, VIEWPORT_H);

	r_state_pop();
	stage_draw_end_noshake();

	r_shader("sprite_default");
	draw_baryons(bcenter, t);

	stage_draw_begin_noshake();

	r_shader_standard();
	fbpair_swap(&stage6_draw_data->baryon.fbpair);
	r_color4(0.7, 0.7, 0.7, 0.7);
	draw_framebuffer_tex(stage6_draw_data->baryon.fbpair.front, VIEWPORT_W, VIEWPORT_H);

	stage_draw_end_noshake();

	r_color4(1, 1, 1, 1);
	r_framebuffer(stage6_draw_data->baryon.fbpair.front);
	r_shader("sprite_default");
	draw_baryons(bcenter, t);

	for(Enemy *e = global.enemies.first; e; e = e->next) {
		if(e->visual_rule == baryon) {
			cmplx p = e->pos; //+10*frand()*cexp(2.0*I*M_PI*frand());

			r_draw_sprite(&(SpriteParams) {
				.sprite = "part/myon",
				.color = RGBA(1, 0.2, 1.0, 0.7),
				.pos = { creal(p), cimag(p) },
				.rotation.angle = (creal(e->args[0]) - t) / 16.0, // frand()*M_PI*2,
				.scale.both = 2,
			});
		}
	}
}

void elly_spellbg_classic(Boss *b, int t) {
	fill_viewport(0,0,0.7,"stage6/spellbg_classic");
	r_blend(BLEND_MOD);
	r_color4(1, 1, 1, 0);
	fill_viewport(0,-t*0.005,0.7,"stage6/spellbg_chalk");
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(1, 1, 1, 1);
}

void elly_spellbg_modern(Boss *b, int t) {
	fill_viewport(0,0,0.6,"stage6/spellbg_modern");
	r_blend(BLEND_MOD);
	r_color4(1, 1, 1, 0);
	fill_viewport(0,-t*0.005,0.7,"stage6/spellbg_chalk");
	r_blend(BLEND_PREMUL_ALPHA);
	r_color4(1, 1, 1, 1);
}

void elly_spellbg_modern_dark(Boss *b, int t) {
	elly_spellbg_modern(b, t);
	fade_out(0.75 * fmin(1, t / 300.0));
}

void elly_global_rule(Boss *b, int time) {
	global.boss->glowcolor = *HSL(time/120.0, 1.0, 0.25);
	global.boss->shadowcolor = *HSLA_MUL_ALPHA((time+20)/120.0, 1.0, 0.25, 0.5);
}
