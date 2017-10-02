/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stage5.h"
#include "stage5_events.h"

#include "stage.h"
#include "stageutils.h"
#include "global.h"

/*
 *	See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage5_spells_s stage5_spells = {
	.boss = {
		.atmospheric_discharge	= {{ 0,  1,  2,  3}, AT_Spellcard, "High Voltage ~ Atmospheric Discharge", 30, 44000,
									iku_atmospheric, iku_spell_bg, BOSS_DEFAULT_GO_POS},
		.artificial_lightning	= {{ 4,  5,  6,  7}, AT_Spellcard, "Charge Sign ~ Artificial Lightning", 45, 60000,
									iku_lightning, iku_spell_bg, BOSS_DEFAULT_GO_POS},
		.natural_cathode		= {{ 8,  9, 10, 11}, AT_Spellcard, "Spark Sign ~ Natural Cathode", 30, 44000,
									iku_cathode, iku_spell_bg, BOSS_DEFAULT_GO_POS},
		.induction_field		= {{12, 13, -1, -1}, AT_Spellcard, "Current Sign ~ Induction Field", 30, 50000,
									iku_induction, iku_spell_bg, BOSS_DEFAULT_GO_POS},
		.inductive_resonance	= {{-1, -1, 14, 15}, AT_Spellcard, "Current Sign ~ Inductive Resonance", 30, 50000,
									iku_induction, iku_spell_bg, BOSS_DEFAULT_GO_POS},
	},

	.extra.overload				= {{ 0,  1,  2,  3}, AT_ExtraSpell, "Circuit Sign ~ Overload", 60, 44000,
									iku_extra, iku_spell_bg, BOSS_DEFAULT_GO_POS},
};

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
	preload_resources(RES_BGM, RESF_OPTIONAL, "stage5", "stage5boss", NULL);
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
		global.boss = create_boss("Nagae Iku", "iku", "dialog/iku", BOSS_DEFAULT_SPAWN_POS);
		boss_add_attack_from_info(global.boss, global.stage->spell, true);
		boss_start_attack(global.boss, global.boss->attacks);

		start_bgm("stage5boss");
	}
}

ShaderRule stage5_shaders[] = { NULL };

StageProcs stage5_procs = {
	.begin = stage5_start,
	.preload = stage5_preload,
	.end = stage5_end,
	.draw = stage5_draw,
	.event = stage5_events,
	.shader_rules = stage5_shaders,
	.spellpractice_procs = &stage5_spell_procs,
};

StageProcs stage5_spell_procs = {
	.preload = stage5_preload,
	.begin = stage5_start,
	.end = stage5_end,
	.draw = stage5_draw,
	.event = stage5_spellpractice_events,
	.shader_rules = stage5_shaders,
};
