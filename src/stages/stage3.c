/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "stage3.h"
#include "stage3_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"

/*
 *  See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage3_spells_s stage3_spells = {
	.mid = {
		.deadly_dance = {
			{ 0,  1,  2,  3}, AT_SurvivalSpell, "Venom Sign “Deadly Dance”", 14, 40000,
			scuttle_deadly_dance, scuttle_spellbg, BOSS_DEFAULT_GO_POS
		},
	},

	.boss = {
		.moonlight_rocket = {
			{ 6,  7,  8,  9}, AT_Spellcard, "Firefly Sign “Moonlight Rocket”", 35, 40000,
			wriggle_moonlight_rocket, wriggle_spellbg, BOSS_DEFAULT_GO_POS
		},
		.wriggle_night_ignite = {
			{10, 11, 12, 13}, AT_Spellcard, "Light Source “Wriggle Night Ignite”", 50, 46000,
			wriggle_night_ignite, wriggle_spellbg, BOSS_DEFAULT_GO_POS
		},
		.firefly_storm = {
			{14, 15, 16, 17}, AT_Spellcard, "Bug Sign “Firefly Storm”", 45, 45000,
			wriggle_firefly_storm, wriggle_spellbg, BOSS_DEFAULT_GO_POS
		},
	},

	.extra.light_singularity = {
		{ 0,  1,  2,  3}, AT_ExtraSpell, "Lamp Sign “Light Singularity”", 75, 45000,
		wriggle_light_singularity, wriggle_spellbg, BOSS_DEFAULT_GO_POS
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

static vec3 **stage3_bg_pos(vec3 pos, float maxrange) {
	//vec3 p = {100 * cos(global.frames / 52.0), 100, 50 * sin(global.frames / 50.0)};
	vec3 p = {
		stgstate.tunnel_side * cos(global.frames / 52.0),
		0,
		stgstate.tunnel_updn * sin(global.frames / 50.0)
	};
	vec3 r = {0, 3000, 0};

	return linear3dpos(pos, maxrange, p, r);
}

static void stage3_bg_tunnel_draw(vec3 pos) {
	int n = 6;
	float r = 300;
	int i;

	r_mat_push();
	r_mat_translate(pos[0], pos[1], pos[2]);

	r_uniform_sampler("tex", "stage3/border");

	for(i = 0; i < n; i++) {
		r_mat_push();
		r_mat_rotate_deg(360.0/n*i + stgstate.tunnel_angle, 0, 1, 0);
		r_mat_translate(0,0,-r);
		r_mat_scale(2*r/tan((n-2)*M_PI/n), 3000, 1);
		r_draw_quad();
		r_mat_pop();
	}

	r_mat_pop();
}

static void stage3_tunnel(Framebuffer *fb) {
	r_shader("tunnel");
	r_uniform_vec3("color", stgstate.clr_r, stgstate.clr_g, stgstate.clr_b);
	r_uniform_float("mixfactor", stgstate.clr_mixfactor);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
}

static void stage3_fog(Framebuffer *fb) {
	r_shader("zbuf_fog");
	r_uniform_sampler("depth", r_framebuffer_get_attachment(fb, FRAMEBUFFER_ATTACH_DEPTH));
	r_uniform_vec4("fog_color", stgstate.fog_brightness, stgstate.fog_brightness, stgstate.fog_brightness, 1.0);
	r_uniform_float("start", 0.2);
	r_uniform_float("end", 0.8);
	r_uniform_float("exponent", stgstate.fog_exp/2);
	r_uniform_float("sphereness", 0);
	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
}

static void stage3_glitch(Framebuffer *fb) {
	float strength;

	if(global.boss && global.boss->current && ATTACK_IS_SPELL(global.boss->current->type) && !strcmp(global.boss->name, "Scuttle")) {
		strength = 0.05 * pow(max(0, (global.frames - global.boss->current->starttime) / (double)global.boss->current->timeout), 2.0);
	} else {
		strength = 0.0;
	}

	if(strength > 0) {
		r_shader("glitch");
		r_uniform_float("strength", strength);
		r_uniform_int("frames", global.frames + tsrand() % 30);
	} else {
		r_shader_standard();
		r_color4(1, 1, 1, 1);
	}

	draw_framebuffer_tex(fb, VIEWPORT_W, VIEWPORT_H);
	r_shader_standard();
}

static void stage3_start(void) {
	init_stage3d(&stage_3d_context);

	stage_3d_context.cx[2] = -10;
	stage_3d_context.crot[0] = -95;
	stage_3d_context.cv[1] = 10;

	add_model(&stage_3d_context, stage3_bg_tunnel_draw, stage3_bg_pos);

	memset(&stgstate, 0, sizeof(stgstate));
	stgstate.clr_r = 1.0;
	stgstate.clr_g = 0.0;
	stgstate.clr_b = 0.5;
	stgstate.clr_mixfactor = 1.0;
	stgstate.fog_brightness = 0.5;
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
	NULL);
	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT,
		"tunnel",
		"zbuf_fog",
		"glitch",
		"maristar_bombbg",
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
	free_stage3d(&stage_3d_context);
}

static void stage3_draw(void) {
	set_perspective(&stage_3d_context, 300, 5000);
	draw_stage3d(&stage_3d_context, 7000);
}

static void stage3_update(void) {
	TIMER(&global.timer)

	stgstate.tunnel_angle += stgstate.tunnel_avel;
	stage_3d_context.crot[2] = -(creal(global.plr.pos)-VIEWPORT_W/2)/80.0;

	if(global.dialog) {
		update_stage3d(&stage_3d_context);
		return;
	}

	FROM_TO(0, 160, 1) {
		stage_3d_context.cv[1] -= 0.5/2;
		stgstate.clr_r -= 0.2 / 160.0;
		stgstate.clr_b -= 0.1 / 160.0;
	}

	FROM_TO(0, 500, 1)
		stgstate.fog_exp += 3.0 / 500.0;

	FROM_TO(400, 500, 1) {
		stgstate.tunnel_avel += 0.005;
		stage_3d_context.cv[1] -= 0.3/2;
	}

	FROM_TO(1050, 1150, 1) {
		stgstate.tunnel_avel -= 0.010;
		stage_3d_context.cv[1] -= 0.2/2;
	}

	FROM_TO(1060, 1400, 1) {
		/*stgstate.clr_r -= 1.0 / 340.0;
		stgstate.clr_g += 1.0 / 340.0;
		stgstate.clr_b -= 0.5 / 340.0;*/
	}

	FROM_TO(1170, 1400, 1)
		stgstate.tunnel_side += 100.0 / 230.0;

	FROM_TO(1400, 1550, 1) {
		stage_3d_context.crot[0] -= 3 / 150.0;
		stgstate.tunnel_updn += 70.0 / 150.0;
		stgstate.tunnel_avel += 1 / 150.0;
		stage_3d_context.cv[1] -= 0.2/2;
	}

	FROM_TO(1570, 1700, 1) {
		stgstate.tunnel_updn -= 20 / 130.0;
	}

	FROM_TO(1800, 1850, 1)
		stgstate.tunnel_avel -= 0.02;

	FROM_TO(1900, 2000, 1) {
		stgstate.tunnel_avel += 0.013;
	}

	FROM_TO(2000, 2680, 1) {
		stgstate.tunnel_side -= 100.0 / 680.0;
		//stgstate.fog_exp -= 1.0 / 740.0;
		stage_3d_context.crot[0] += 11 / 680.0;
	}

	FROM_TO(2680, 2739, 1) {
		stgstate.fog_exp += 1.0 / 60.0;
		stage_3d_context.cv[1] += 1.0/2;
		stgstate.clr_r -= 0.3 / 60.0;
		stgstate.clr_g += 0.5 / 60.0;
		stgstate.tunnel_avel -= 0.7 / 60.0;
		stage_3d_context.crot[0] -= 11 / 60.0;
	}

	// 2740 - MIDBOSS

	int midboss_time = STAGE3_MIDBOSS_TIME;

	FROM_TO(2900 + midboss_time, 3100 + midboss_time, 1) {
		stgstate.clr_r += 0.3 / 200.0;
		stgstate.clr_g -= 0.5 / 200.0;
		stage_3d_context.cv[1] -= 90 / 200.0/2;
		stgstate.tunnel_avel -= 1 / 200.0;
		stgstate.fog_exp -= 1.0 / 200.0;
		stgstate.clr_b += 0.2 / 200.0;
	}

	FROM_TO(3300 + midboss_time, 3360 + midboss_time, 1) {
		stgstate.tunnel_avel += 2 / 60.0;
		stgstate.tunnel_side += 50 / 60.0;
		stage_3d_context.cx[2] += (-20 - stage_3d_context.cx[2]) * 0.04;
	}

	FROM_TO(3600 + midboss_time, 3700 + midboss_time, 1) {
		stgstate.tunnel_side += 20 / 60.0;
		stgstate.tunnel_updn += 40 / 60.0;
	}

	FROM_TO(3830 + midboss_time, 3950 + midboss_time, 1) {
		stgstate.tunnel_avel -= 2 / 120.0;
	}

	FROM_TO(3960 + midboss_time, 4000 + midboss_time, 1) {
		stgstate.tunnel_avel += 2 / 40.0;
	}

	FROM_TO(4360 + midboss_time, 4390 + midboss_time, 1) {
		stgstate.clr_r -= .5 / 30.0;
	}

	FROM_TO(4390 + midboss_time, 4510 + midboss_time, 1) {
		stgstate.clr_r += .5 / 120.0;
		stage_3d_context.cx[2] += (0 - stage_3d_context.cx[2]) * 0.05;
		stgstate.fog_exp = approach(stgstate.fog_exp, 4, 1/20.0);
	}

	FROM_TO(4299 + midboss_time, 5299 + midboss_time, 1) {
		stgstate.tunnel_side -= 70 / 1000.0;
		stgstate.tunnel_updn -= 40 / 1000.0;
		stgstate.clr_r -= 0.5 / 1000.0;
		stgstate.clr_b += 0.2 / 1000.0;
		stage_3d_context.crot[0] += 7 / 1000.0;
		stgstate.fog_exp -= 1.5 / 1000.0;
	}

	FROM_TO(5099 + midboss_time, 5299 + midboss_time, 1) {
		stage_3d_context.cv[1] += 90 / 200.0/2;
		stgstate.tunnel_avel -= 1.1 / 200.0;
		stage_3d_context.crot[0] -= 15 / 200.0;
		stgstate.fog_exp = approach(stgstate.fog_exp, 2.5, 1/50.0);
	}

	FROM_TO(5200 + midboss_time, 5300 + midboss_time, 1) {
		stage_3d_context.cx[2] += (-50 - stage_3d_context.cx[2]) * 0.02;
	}

	// 5300 - BOSS

	FROM_TO(5301 + midboss_time, 5700 + midboss_time, 1) {
		stgstate.tunnel_avel = (0 - stgstate.tunnel_avel) * 0.02;
		stage_3d_context.cx[2] += (-500 - stage_3d_context.cx[2]) * 0.02;
		stage_3d_context.crot[0] += (-180 - stage_3d_context.crot[0]) * 0.004;

		stgstate.clr_mixfactor = approach(stgstate.clr_mixfactor, 0, 1/300.0);
		stgstate.fog_brightness = approach(stgstate.fog_brightness, 1.0, 1/200.0);
		stgstate.fog_exp = approach(stgstate.fog_exp, 3, 1/50.0);

		stage_3d_context.cv[1] = approach(stage_3d_context.cv[1], -20, 1/10.0);

		stgstate.clr_r = approach(stgstate.clr_r, 0.6, 1.0/200.0);
		stgstate.clr_g = approach(stgstate.clr_g, 0.3, 1.0/200.0);
		stgstate.clr_b = approach(stgstate.clr_b, 0.4, 1.0/200.0);
	}

	update_stage3d(&stage_3d_context);
}

void scuttle_spellbg(Boss*, int t);

static void stage3_spellpractice_start(void) {
	stage3_start();

	if(global.stage->spell->draw_rule == scuttle_spellbg) {
		skip_background_anim(stage3_update, 2800, &global.timer, NULL);
		global.boss = stage3_spawn_scuttle(BOSS_DEFAULT_SPAWN_POS);
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
	audio_backend_music_set_position(global.timer / (double)FPS);
}

ShaderRule stage3_shaders[] = { stage3_fog, stage3_tunnel, NULL };
ShaderRule stage3_postprocess[] = { stage3_glitch, NULL };

StageProcs stage3_procs = {
	.begin = stage3_start,
	.preload = stage3_preload,
	.end = stage3_end,
	.draw = stage3_draw,
	.update = stage3_update,
	.event = stage3_events,
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
	.event = stage3_spellpractice_events,
	.shader_rules = stage3_shaders,
	.postprocess_rules = stage3_postprocess,
};
