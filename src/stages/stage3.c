/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "stage3.h"
#include "stage3_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stagedraw.h"
#include "common_tasks.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage3_spells_s stage3_spells = {
	.mid = {
		.deadly_dance = {
			{ 0,  1,  2,  3}, AT_SurvivalSpell, "Venom Sign “Deadly Dance”", 14, 40000,
			NULL, scuttle_spellbg, VIEWPORT_W/2.0+100*I, 1, TASK_INDIRECT_INIT(BossAttack, stage3_spell_deadly_dance)
		},
	},

	.boss = {
		.moonlight_rocket = {
			{ 6,  7,  8,  9}, AT_Spellcard, "Firefly Sign “Moonlight Rocket”", 40, 40000,
			NULL, wriggle_spellbg, VIEWPORT_W/2.0+100*I, 1, TASK_INDIRECT_INIT(BossAttack, stage3_spell_moonlight_rocket)
		},
		.wriggle_night_ignite = {
			{10, 11, 12, 13}, AT_Spellcard, "Light Source “Wriggle Night Ignite”", 50, 46000,
			NULL, wriggle_spellbg, VIEWPORT_W/2.0+100*I, 1, TASK_INDIRECT_INIT(BossAttack, stage3_spell_night_ignite)
		},
		.firefly_storm = {
			{14, 15, 16, 17}, AT_Spellcard, "Bug Sign “Firefly Storm”", 45, 45000,
			NULL, wriggle_spellbg, VIEWPORT_W/2.0+100*I, 1, TASK_INDIRECT_INIT(BossAttack, stage3_spell_firefly_storm)
		},
	},

	.extra.light_singularity = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Lamp Sign “Light Singularity”", 75, 45000,
			NULL, wriggle_spellbg, VIEWPORT_W/2.0+100*I, 1, TASK_INDIRECT_INIT(BossAttack, stage3_spell_light_singularity)
	},
};

static struct {
	float clr_r;
	float clr_g;
	float clr_b;
	float clr_mixfactor;

	float fog_exp;
	float fog_brightness;

	float tunnel_angle;
	float tunnel_avel;
	float tunnel_updn;
	float tunnel_side;
} stgstate;

static uint stage3_bg_pos(Stage3D *s3d, vec3 pos, float maxrange) {
	//vec3 p = {100 * cos(global.frames / 52.0), 100, 50 * sin(global.frames / 50.0)};
	vec3 p = {
		stgstate.tunnel_side * cos(global.frames / 52.0),
		0,
		stgstate.tunnel_updn * sin(global.frames / 50.0)
	};
	vec3 r = {0, 3000, 0};

	return linear3dpos(s3d, pos, maxrange, p, r);
}

static void stage3_bg_tunnel_draw(vec3 pos) {
	int n = 6;
	float r = 300;
	int i;

	r_mat_mv_push();
	r_mat_mv_translate(pos[0], pos[1], pos[2]);

	r_uniform_sampler("tex", "stage3/border");

	for(i = 0; i < n; i++) {
		r_mat_mv_push();
		r_mat_mv_rotate(M_PI*2.0/n*i + stgstate.tunnel_angle * DEG2RAD, 0, 1, 0);
		r_mat_mv_translate(0,0,-r);
		r_mat_mv_scale(2*r/tan((n-2)*M_PI/n), 3000, 1);
		r_draw_quad();
		r_mat_mv_pop();
	}

	r_mat_mv_pop();
}

static bool stage3_tunnel(Framebuffer *fb) {
	r_shader("tunnel");
	r_uniform_vec3("color", stgstate.clr_r, stgstate.clr_g, stgstate.clr_b);
	r_uniform_float("mixfactor", stgstate.clr_mixfactor);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static bool stage3_fog(Framebuffer *fb) {
	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", stgstate.fog_brightness, stgstate.fog_brightness, stgstate.fog_brightness, 1.0);
	r_uniform_float("start", 0.2);
	r_uniform_float("end", 0.8);
	r_uniform_float("exponent", stgstate.fog_exp/2);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static bool stage3_glitch(Framebuffer *fb) {
	float strength;

	if(global.boss && global.boss->current && ATTACK_IS_SPELL(global.boss->current->type) && !strcmp(global.boss->name, "Scuttle")) {
		strength = 0.05 * max(0, (global.frames - global.boss->current->starttime) / (double)global.boss->current->timeout);
	} else {
		strength = 0.0;
	}

	if(strength > 0) {
		r_shader("glitch");
		r_uniform_float("strength", strength);
		r_uniform_float("frames", global.frames + 15 * nfrand());
	} else {
		return false;
	}

	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
	return true;
}

static void stage3_init_background(void) {
	stage3d_init(&stage_3d_context, 16);

	stage_3d_context.cx[2] = -10;
	stage_3d_context.crot[0] = -95;
	stage_3d_context.cv[1] = 10;

	memset(&stgstate, 0, sizeof(stgstate));
	stgstate.clr_r = 1.0;
	stgstate.clr_g = 0.0;
	stgstate.clr_b = 0.5;
	stgstate.clr_mixfactor = 1.0;
	stgstate.fog_brightness = 0.5;
}

static void stage3_start(void) {
	stage3_init_background();
	stage_start_bgm("stage3");
	stage_set_voltage_thresholds(50, 125, 300, 600);
	INVOKE_TASK(stage3_main);
}

static void stage3_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage3", "stage3boss", NULL);
	preload_resources(RES_SPRITE, RESF_DEFAULT,
		"stage3/border",
		"stage3/spellbg1",
		"stage3/spellbg2",
		"stage3/wspellbg",
		"stage3/wspellclouds",
		"stage3/wspellswarm",
		"dialog/wriggle",
		"dialog/scuttle",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"tunnel",
		"zbuf_fog",
		"glitch",
		"maristar_bombbg",
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_OPTIONAL,
		"lasers/accelerated",
		"lasers/sine_expanding",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"boss/scuttle",
		"boss/wriggleex",
	NULL);
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"laser1",
	NULL);
}

static void stage3_end(void) {
	stage3d_shutdown(&stage_3d_context);
}

static void stage3_draw(void) {
	r_mat_proj_perspective(STAGE3D_DEFAULT_FOVY, STAGE3D_DEFAULT_ASPECT, 300, 5000);
	stage3d_draw(&stage_3d_context, 7000, 1, (Stage3DSegment[]) { stage3_bg_tunnel_draw, stage3_bg_pos });
}

static void stage3_update(void) {
	TIMER(&global.timer)

	stgstate.tunnel_angle += stgstate.tunnel_avel;
	stage_3d_context.crot[2] = -(creal(global.plr.pos)-VIEWPORT_W/2)/80.0;

	if(dialog_is_active(global.dialog)) {
		stage3d_update(&stage_3d_context);
		return;
	}

	stage_3d_context.cv[1] -= 0.5/2;
	stgstate.clr_r -= 0.2 / 160.0;
	stgstate.clr_b -= 0.1 / 160.0;

	stage3d_update(&stage_3d_context);
}

void scuttle_spellbg(Boss*, int t);

static void stage3_spellpractice_start(void) {
	if(global.stage->spell->draw_rule == scuttle_spellbg) {
		skip_background_anim(stage3_update, 2800, &global.timer, NULL);
		global.boss = stage3_spawn_scuttle(BOSS_DEFAULT_SPAWN_POS);
		stage_unlock_bgm("scuttle");
		stage_start_bgm("scuttle");
	} else {
		skip_background_anim(stage3_update, 5300 + STAGE3_MIDBOSS_TIME, &global.timer, NULL);
		global.boss = stage3_spawn_wriggle_ex(BOSS_DEFAULT_SPAWN_POS);
		stage_start_bgm("stage3boss");
	}

	boss_add_attack_from_info(global.boss, global.stage->spell, true);
	boss_start_attack(global.boss, global.boss->attacks);
}

static void stage3_spellpractice_events(void) {
}

void stage3_skip(int t) {
	skip_background_anim(stage3_update, t, &global.timer, &global.frames);
	audio_music_set_position(global.timer / (double)FPS);
}

ShaderRule stage3_shaders[] = { stage3_fog, stage3_tunnel, NULL };
ShaderRule stage3_postprocess[] = { stage3_glitch, NULL };

StageProcs stage3_procs = {
	.begin = stage3_start,
	.preload = stage3_preload,
	.end = stage3_end,
	.draw = stage3_draw,
	.update = stage3_update,
	.shader_rules = stage3_shaders,
	.postprocess_rules = stage3_postprocess,
	.spellpractice_procs = &stage3_spell_procs,
};

StageProcs stage3_spell_procs = {
	.begin = stage3_spellpractice_start,
	.preload = stage3_preload,
	.end = stage3_end,
	.draw = stage3_draw,
	.update = stage3_update,
	.shader_rules = stage3_shaders,
	.postprocess_rules = stage3_postprocess,
};
