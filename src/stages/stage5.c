/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage5.h"
#include "stage5_events.h"

#include "stage.h"
#include "stageutils.h"

#include "global.h"

static Stage3D bgcontext;

struct {
	float light_strength;

	float rotshift;
	float omega;
	float rad;
} stagedata;

Vector **stage5_stairs_pos(Vector pos, float maxrange) {
	Vector p = {0, 0, 0};
	Vector r = {0, 0, 6000};

	return linear3dpos(pos, maxrange, p, r);
}

void stage5_stairs_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage5/tower")->gltex);

	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
	glScalef(300,300,300);

	Shader *sha = get_shader("tower_light");
	glUseProgram(sha->prog);
	glUniform3f(uniloc(sha, "lightvec"), 0, 0, 0);
	glUniform4f(uniloc(sha, "color"), 0.1, 0.1, 0.5, 1);
	glUniform1f(uniloc(sha, "strength"), stagedata.light_strength);

	draw_model("tower");

	glPopMatrix();

	glDisable(GL_TEXTURE_2D);

	glUseProgram(0);
}

void stage5_draw(void) {
	set_perspective(&bgcontext, 100, 20000);
	draw_stage3d(&bgcontext, 30000);

	TIMER(&global.timer);
	float w = 0.005;

	stagedata.rotshift += stagedata.omega;
	bgcontext.crot[0] += stagedata.omega*0.5;
	stagedata.rad += stagedata.omega*20;
	FROM_TO(5000, 5050, 1)
		stagedata.omega -= 0.005;

	FROM_TO(5200, 5250, 1)
		stagedata.omega += 0.005;

	bgcontext.cx[0] = stagedata.rad*cos(-w*global.frames);
	bgcontext.cx[1] = stagedata.rad*sin(-w*global.frames);
	bgcontext.cx[2] = -1700+w*3000/M_PI*global.frames;

	bgcontext.crot[2] = stagedata.rotshift-180/M_PI*w*global.frames;

	stagedata.light_strength *= 0.98;

	if(frand() < 0.01)
		stagedata.light_strength = 5+5*frand();
}

void iku_spell_bg(Boss *b, int t) {
	fill_screen(0, 300, 1, "stage5/spell_bg");

	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	fill_screen(0, t*0.001, 0.7, "stage5/noise");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPushMatrix();
	glTranslatef(0, -100, 0);

	fill_screen(t/100.0,0,0.5,"stage5/spell_clouds");
	glPushMatrix();
	glTranslatef(0, 100, 0);
	fill_screen(t/100.0*0.75,0,0.6,"stage5/spell_clouds");
	glPushMatrix();
	glTranslatef(0, 100, 0);
	fill_screen(t/100.0*0.5,0,0.7,"stage5/spell_clouds");
	glPushMatrix();
	glTranslatef(0, 100, 0);
	fill_screen(t/100.0*0.25,0,0.8,"stage5/spell_clouds");
	glPopMatrix();
	glPopMatrix();
	glPopMatrix();
	glPopMatrix();

	glColor4f(1,1,1,0.05*stagedata.light_strength);
	fill_screen(0, 300, 1, "stage5/spell_lightning");
	glColor4f(1,1,1,1);
}

void stage5_start(void) {
	memset(&stagedata, 0, sizeof(stagedata));

	init_stage3d(&bgcontext);
	add_model(&bgcontext, stage5_stairs_draw, stage5_stairs_pos);

	bgcontext.crot[0] = 60;
	stagedata.rotshift = 140;
	stagedata.rad = 2800;
}

void stage5_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "bgm_stage5", "bgm_stage5boss", NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage5/noise",
		"stage5/spell_bg",
		"stage5/spell_clouds",
		"stage5/spell_lightning",
		"stage5/tower",
		"dialog/iku",
	NULL);
	preload_resources(RES_SHADER, RESF_DEFAULT,
		"tower_light",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"iku",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"tower",
	NULL);
}

void stage5_end(void) {
	free_stage3d(&bgcontext);
}

void stage5_spellpractice_events(void) {
	TIMER(&global.timer);

	AT(0) {
		skip_background_anim(&bgcontext, stage5_draw, 5300, &global.timer, NULL);
		global.boss = create_boss("Nagae Iku", "iku", BOSS_DEFAULT_SPAWN_POS);
		boss_add_attack_from_info(global.boss, global.stage->spell, true);
		start_attack(global.boss, global.boss->attacks);

		start_bgm("bgm_stage5boss");
	}
}

ShaderRule stage5_shaders[] = { NULL };

StageProcs stage5_procs = {
	stage5_start,
	stage5_preload,
	stage5_end,
	stage5_draw,
	stage5_events,
	stage5_shaders,
	&stage5_spell_procs,
};

StageProcs stage5_spell_procs = {
	stage5_start,
	stage5_preload,
	stage5_end,
	stage5_draw,
	stage5_spellpractice_events,
	stage5_shaders,
};
