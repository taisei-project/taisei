/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stage2.h"
#include "stage2_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"

/*
 *	See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage2_spells_s stage2_spells = {
	.boss = {
		.amulet_of_harm		= {{ 0,  1,  2,  3}, AT_Spellcard, "Shard ~ Amulet of Harm", 26, 50000,
									hina_amulet, hina_spell_bg, BOSS_DEFAULT_GO_POS},
		.bad_pick		= {{ 4,  5,  6,  7}, AT_Spellcard, "Lottery Sign ~ Bad Pick", 30, 43200,
									hina_bad_pick, hina_spell_bg, BOSS_DEFAULT_GO_POS},
		.wheel_of_fortune_easy	= {{ 8,  9, -1, -1}, AT_Spellcard, "Lottery Sign ~ Wheel of Fortune", 20, 36000,
									hina_wheel, hina_spell_bg, BOSS_DEFAULT_GO_POS},
		.wheel_of_fortune_hard	= {{-1, -1, 10, 11}, AT_Spellcard, "Lottery Sign ~ Wheel of Fortune", 25, 36000,
									hina_wheel, hina_spell_bg, BOSS_DEFAULT_GO_POS},
	},

	.extra.monty_hall_danmaku	= {{ 0,  1,  2,  3}, AT_ExtraSpell, "Lottery Sign ~ Monty Hall Danmaku", 60, 60000,
								hina_monty, hina_spell_bg, BOSS_DEFAULT_GO_POS},
};

void stage2_bg_leaves_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glUseProgram(get_shader("alpha_depth")->prog);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(-1,1,1);
	glMatrixMode(GL_MODELVIEW);

	Texture *leaves = get_tex("stage2/leaves");
	glBindTexture(GL_TEXTURE_2D, leaves->gltex);

	glPushMatrix();
	glTranslatef(pos[0]-360,pos[1],pos[2]+500);
	glRotatef(-160,0,1,0);
	glTranslatef(-50,0,0);
	glScalef(1000,3000,1);

	draw_quad();

	glPopMatrix();

	glUseProgram(0);

	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

static void stage2_bg_grass_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage2/roadgrass")->gltex);

	glPushMatrix();
	glTranslatef(pos[0]+250,pos[1],pos[2]+40);
	glRotatef(pos[2]/2-14,0,1,0);

	glScalef(-500,2000,1);
	draw_quad();
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

}

static void stage2_bg_ground_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();
	glTranslatef(pos[0]-50,pos[1],pos[2]);
	glScalef(-1000,1000,1);

	Texture *road = get_tex("stage2/roadstones");

	glBindTexture(GL_TEXTURE_2D, road->gltex);

	glColor4f(0.08,0.,0.1,1);
	glDisable(GL_TEXTURE_2D);
	draw_quad();
	glEnable(GL_TEXTURE_2D);
	glColor4f(0.5,0.5,0.5,1);
	draw_quad();
	glColor4f(1,1,1,1);
	glTranslatef(0,0,+10);
	draw_quad();


	glPopMatrix();

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(global.frames/100.0,1*sin(global.frames/100.0),0);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	Texture *border = get_tex("stage2/border");
	glBindTexture(GL_TEXTURE_2D, border->gltex);

	glTranslatef(pos[0]+410,pos[1],pos[2]+600);
	glRotatef(90,0,1,0);
	glScalef(1200,1000,1);

	draw_quad();

	glPopMatrix();

	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

Vector **stage2_bg_pos(Vector pos, float maxrange) {
	Vector p = {0, 0, 0};
	Vector r = {0, 1000, 0};

	return linear3dpos(pos, maxrange, p, r);
}

Vector **stage2_bg_grass_pos(Vector pos, float maxrange) {
	Vector p = {0, 0, 0};
	Vector r = {0, 2000, 0};

	return linear3dpos(pos, maxrange, p, r);
}

Vector **stage2_bg_grass_pos2(Vector pos, float maxrange) {
	Vector p = {0, 1234, 40};
	Vector r = {0, 2000, 0};

	return linear3dpos(pos, maxrange, p, r);
}

static void stage2_fog(FBO *fbo) {
	Shader *shader = get_shader("zbuf_fog");

	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "depth"),2);
	glUniform4f(uniloc(shader, "fog_color"),0.05,0.0,0.03,1.0);
	glUniform1f(uniloc(shader, "start"),0.2);
	glUniform1f(uniloc(shader, "end"),0.8);
	glUniform1f(uniloc(shader, "exponent"),3.0);
	glUniform1f(uniloc(shader, "sphereness"),0);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, fbo->depth);
	glActiveTexture(GL_TEXTURE0);

	draw_fbo_viewport(fbo);
	glUseProgram(0);
}

static void stage2_bloom(FBO *fbo) {
	Shader *shader = get_shader("bloom");

	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "samples"), 10);
	glUniform1f(uniloc(shader, "intensity"), 0.05);
	glUniform1f(uniloc(shader, "radius"), 0.03);
	draw_fbo_viewport(fbo);
	glUseProgram(0);
}

void stage2_start(void) {
	init_stage3d(&stage_3d_context);
	stage_3d_context.cx[2] = 1000;
	stage_3d_context.cx[0] = -850;
	stage_3d_context.crot[0] = 60;
	stage_3d_context.crot[2] = -90;

	stage_3d_context.cv[0] = 9;

	add_model(&stage_3d_context, stage2_bg_ground_draw, stage2_bg_pos);
	add_model(&stage_3d_context, stage2_bg_grass_draw, stage2_bg_grass_pos);
	add_model(&stage_3d_context, stage2_bg_grass_draw, stage2_bg_grass_pos2);
	add_model(&stage_3d_context, stage2_bg_leaves_draw, stage2_bg_pos);
}

void stage2_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage2", "stage2boss", NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage2/border",
		"stage2/leaves",
		"stage2/roadgrass",
		"stage2/roadstones",
		"stage2/spellbg1",
		"stage2/spellbg2",
		"dialog/hina",
	NULL);
	preload_resources(RES_SHADER, RESF_DEFAULT,
		"bloom",
		"zbuf_fog",
		"alpha_depth",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"wriggle",
		"hina",
		"fire",
	NULL);
}

void stage2_end(void) {
	free_stage3d(&stage_3d_context);
}

void stage2_draw(void) {
	TIMER(&global.frames);

	set_perspective(&stage_3d_context, 500, 5000);

	FROM_TO(0,180,1) {
		stage_3d_context.cv[0] -= 0.05;
		stage_3d_context.cv[1] += 0.05;
		stage_3d_context.crot[2] += 0.5;
	}

	draw_stage3d(&stage_3d_context, 7000);

}

void stage2_spellpractice_events(void) {
	TIMER(&global.timer);

	AT(0) {
		skip_background_anim(&stage_3d_context, stage2_draw, 180, &global.frames, NULL);

		Boss* hina = stage2_spawn_hina(BOSS_DEFAULT_SPAWN_POS);
		boss_add_attack_from_info(hina, global.stage->spell, true);
		boss_start_attack(hina, hina->attacks);
		global.boss = hina;

		start_bgm("stage2boss");
	}
}

ShaderRule stage2_shaders[] = { stage2_fog, stage2_bloom, NULL };

StageProcs stage2_procs = {
	.begin = stage2_start,
	.preload = stage2_preload,
	.end = stage2_end,
	.draw = stage2_draw,
	.event = stage2_events,
	.shader_rules = stage2_shaders,
	.spellpractice_procs = &stage2_spell_procs,
};

StageProcs stage2_spell_procs = {
	.preload = stage2_preload,
	.begin = stage2_start,
	.end = stage2_end,
	.draw = stage2_draw,
	.event = stage2_spellpractice_events,
	.shader_rules = stage2_shaders,
};
