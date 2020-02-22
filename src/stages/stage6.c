/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage6.h"
#include "stage6_events.h"

#include "stage.h"
#include "stageutils.h"
#include "global.h"
#include "resource/model.h"
#include "stagedraw.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage6_spells_s stage6_spells = {
	.scythe = {
		.occams_razor = {
			{ 0,  1,  2,  3}, AT_Spellcard, "Newton Sign “Occam’s Razor”", 50, 60000,
			elly_newton, elly_spellbg_classic, BOSS_DEFAULT_GO_POS, 6
		},
		.orbital_clockwork = {
			{24, 25, 26, 27}, AT_Spellcard, "Kepler Sign “Orbital Clockwork”", 50, 60000,
			elly_kepler, elly_spellbg_classic, BOSS_DEFAULT_GO_POS, 6
		},
		.wave_theory = {
			{ 4,  5,  6,  7}, AT_Spellcard, "Maxwell Sign “Wave Theory”", 30, 25000,
			elly_maxwell, elly_spellbg_classic, BOSS_DEFAULT_GO_POS, 6
		},
	},

	.baryon = {
		.many_world_interpretation = {
			{ 8,  9, 10, 11}, AT_Spellcard, "Eigenstate “Many-World Interpretation”", 50, 60000,
			elly_eigenstate, elly_spellbg_modern, BOSS_DEFAULT_GO_POS, 7
		},
		.wave_particle_duality = {
			{28, 29, 30, 31}, AT_Spellcard, "de Broglie Sign “Wave-Particle Duality”", 60, 65000,
			elly_broglie, elly_spellbg_modern_dark, BOSS_DEFAULT_GO_POS, 7
		},
		.spacetime_curvature = {
			{12, 13, 14, 15}, AT_Spellcard, "Ricci Sign “Spacetime Curvature”", 50, 80000,
			elly_ricci, elly_spellbg_modern, BOSS_DEFAULT_GO_POS, 7
		},
		.higgs_boson_uncovered = {
			{16, 17, 18, 19}, AT_Spellcard, "LHC “Higgs Boson Uncovered”", 75, 60000,
			elly_lhc, elly_spellbg_modern, BOSS_DEFAULT_GO_POS, 7
		}
	},

	.extra = {
		.curvature_domination = {
			{ 0,  1,  2,  3}, AT_ExtraSpell, "Forgotten Universe “Curvature Domination”", 60, 60000,
			elly_curvature, elly_spellbg_modern, BOSS_DEFAULT_GO_POS, 7
		}
	},

	.final = {
		.theory_of_everything = {
			{20, 21, 22, 23}, AT_SurvivalSpell, "Tower of Truth “Theory of Everything”", 70, 40000,
			elly_theory, elly_spellbg_toe, ELLY_TOE_TARGET_POS, 8
		}
	},
};

static int fall_over;

enum {
	NUM_STARS = 200
};

static float starpos[3*NUM_STARS];
Framebuffer *baryon_aux_fb;
FBPair baryon_fbpair;

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
		float x = starpos[3*i+0], y = starpos[3*i+1], z = starpos[3*i+2];
		r_mat_mv_translate(x, y, z);
		r_mat_mv_rotate(acos(starpos[3*i+2]), -y, x, 0);
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

static void stage6_draw(void) {
	r_mat_proj_perspective(STAGE3D_DEFAULT_FOVY, STAGE3D_DEFAULT_ASPECT, 100, 9000);

	Stage3DSegment segs[] = {
		{ stage6_skysphere_draw, stage6_skysphere_pos },
		{ stage6_towertop_draw, stage6_towertop_pos },
		{ stage6_towerwall_draw, stage6_towerwall_pos },
	};

	stage3d_draw(&stage_3d_context, 10000, ARRAY_SIZE(segs), segs);
}

static void stage6_update(void) {
	stage3d_update(&stage_3d_context);

	if(fall_over) {
		int t = global.frames - fall_over;
		TIMER(&t);

		FROM_TO(0, 240, 1) {
			stage_3d_context.cx[0] += 0.02*cos(M_PI/180*stage_3d_context.crot[2]+M_PI/2)*_i;
			stage_3d_context.cx[1] += 0.02*sin(M_PI/180*stage_3d_context.crot[2]+M_PI/2)*_i;
		}

		FROM_TO(150, 1000, 1) {
			stage_3d_context.crot[0] -= 0.02*(global.frames-fall_over-150);
			if(stage_3d_context.crot[0] < 0)
				stage_3d_context.crot[0] = 0;
		}

		if(t >= 190)
			stage_3d_context.cx[2] -= max(6, 0.05*(global.frames-fall_over-150));

		FROM_TO(300, 470,1) {
			stage_3d_context.cx[0] -= 0.01*cos(M_PI/180*stage_3d_context.crot[2]+M_PI/2)*_i;
			stage_3d_context.cx[1] -= 0.01*sin(M_PI/180*stage_3d_context.crot[2]+M_PI/2)*_i;
		}

	}

	float w = 0.002;
	float f = 1, g = 1;
	if(global.timer > 3273) {
		f = max(0, f-0.01*(global.timer-3273));

	}

	if(global.timer > 3628)
		g = max(0, g-0.01*(global.timer - 3628));

	stage_3d_context.cx[0] += -230*w*f*sin(w*global.frames-M_PI/2);
	stage_3d_context.cx[1] += 230*w*f*cos(w*global.frames-M_PI/2);
	stage_3d_context.cx[2] += w*f*140/M_PI;

	stage_3d_context.crot[2] += 180/M_PI*g*w;
}

void start_fall_over(void) { //troll
	fall_over = global.frames;
}

static void stage6_start(void) {
	// TODO: make this background slightly less horribly inefficient

	stage3d_init(&stage_3d_context, 128);
	fall_over = 0;

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
		starpos[3*i+0] = x/r;
		starpos[3*i+1] = y/r;
		starpos[3*i+2] = z/r;
	}

	stage_3d_context.cx[1] = -230;
	stage_3d_context.crot[0] = 90;
	stage_3d_context.crot[2] = -40;

//  for testing
//  stage_3d_context.cx[0] = 80;
//  stage_3d_context.cx[1] = -215;
//  stage_3d_context.cx[2] = 295;
//  stage_3d_context.crot[0] = 90;
//  stage_3d_context.crot[2] = 381.415100;

	FBAttachmentConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.attachment = FRAMEBUFFER_ATTACH_COLOR0;
	cfg.tex_params.type = TEX_TYPE_RGBA;
	cfg.tex_params.filter.min = TEX_FILTER_LINEAR;
	cfg.tex_params.filter.mag = TEX_FILTER_LINEAR;
	cfg.tex_params.wrap.s = TEX_WRAP_MIRROR;
	cfg.tex_params.wrap.t = TEX_WRAP_MIRROR;
	baryon_fbpair.front = stage_add_background_framebuffer("Baryon FB 1", 0.25, 0.5, 1, &cfg);
	baryon_fbpair.back = stage_add_background_framebuffer("Baryon FB 2", 0.25, 0.5, 1, &cfg);
	baryon_aux_fb = stage_add_background_framebuffer("Baryon FB AUX", 0.25, 0.5, 1, &cfg);
}

static void stage6_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL,
		"stage6",
		"stage6boss_phase1",
		"stage6boss_phase2",
		"stage6boss_phase3",
	NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage6/towertop",
		"stage6/towerwall",
	NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"dialog/elly",
		"part/blast_huge_halo",
		"part/blast_huge_rays",
		"part/myon",
		"part/stardust",
		"proj/apple",
		"stage6/baryon",
		"stage6/baryon_center",
		"stage6/baryon_connector",
		"stage6/scythe",
		"stage6/spellbg_chalk",
		"stage6/spellbg_classic",
		"stage6/spellbg_modern",
		"stage6/spellbg_toe",
		"stage6/toelagrangian/0",
		"stage6/toelagrangian/1",
		"stage6/toelagrangian/2",
		"stage6/toelagrangian/3",
		"stage6/toelagrangian/4",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"baryon_feedback",
		"stage6_sky",
		"tower_wall",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_OPTIONAL,
		"lasers/accelerated",
		"lasers/circle",
		"lasers/elly_toe_fermion",
		"lasers/elly_toe_gluon",
		"lasers/elly_toe_higgs",
		"lasers/elly_toe_photon",
		"lasers/linear",
		"lasers/maxwell",
		"lasers/sine",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/elly",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"towerwall",
		"towertop",
		"skysphere",
	NULL);
	preload_resources(RES_SFX, RESF_DEFAULT | RESF_OPTIONAL,
		"warp",
		"noise1",
		"boom",
		"laser1",
	NULL);
}

static void stage6_end(void) {
	stage3d_shutdown(&stage_3d_context);
}

static void stage6_spellpractice_start(void) {
	stage6_start();
	skip_background_anim(stage6_update, 3800, &global.timer, &global.frames);

	global.boss = stage6_spawn_elly(BOSS_DEFAULT_SPAWN_POS);
	AttackInfo *s = global.stage->spell;

	if(STG6_SPELL_NEEDS_SCYTHE(s)) {
		create_enemy3c(-32 + 32*I, ENEMY_IMMUNE, Scythe, scythe_reset, 0, 1+0.2*I, 1);
		stage_start_bgm("stage6boss_phase1");
	} else if(STG6_SPELL_NEEDS_BARYON(s)) {
		elly_spawn_baryons(global.boss->pos);
		stage_start_bgm("stage6boss_phase2");
	} else if(s == &stage6_spells.final.theory_of_everything) {
		start_fall_over();
		skip_background_anim(stage6_update, 300, &global.timer, &global.frames);
		stage_start_bgm("stage6boss_phase3");
	} else {
		stage_start_bgm("stage6boss_phase2");
	}

	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_start_attack(global.boss, global.boss->attacks);
}

static void stage6_spellpractice_events(void) {
	if(!global.boss) {
		enemy_kill_all(&global.enemies);
	}
}

ShaderRule stage6_shaders[] = { NULL };

StageProcs stage6_procs = {
	.begin = stage6_start,
	.preload = stage6_preload,
	.end = stage6_end,
	.draw = stage6_draw,
	.update = stage6_update,
	.event = stage6_events,
	.shader_rules = stage6_shaders,
	.spellpractice_procs = &stage6_spell_procs,
};

StageProcs stage6_spell_procs = {
	.begin = stage6_spellpractice_start,
	.preload = stage6_preload,
	.end = stage6_end,
	.draw = stage6_draw,
	.update = stage6_update,
	.event = stage6_spellpractice_events,
	.shader_rules = stage6_shaders,
};
