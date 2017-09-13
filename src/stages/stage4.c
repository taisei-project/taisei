/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stage4.h"
#include "stage4_events.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"

/*
 *	See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 *  To add, remove, or reorder spells, see this stage's header file.
 */

struct stage4_spells_s stage4_spells = {
	.mid = {
		.gate_of_walachia	= {{ 0,  1,  2,  3}, AT_Spellcard, "Bloodless ~ Gate of Walachia", 25, 40000,
								kurumi_slaveburst, kurumi_spell_bg, BOSS_DEFAULT_GO_POS},
		.dry_fountain		= {{ 4,  5, -1, -1}, AT_Spellcard, "Bloodless ~ Dry Fountain", 30, 40000,
								kurumi_redspike, kurumi_spell_bg, BOSS_DEFAULT_GO_POS},
		.red_spike			= {{-1, -1,  6,  7}, AT_Spellcard, "Bloodless ~ Red Spike", 30, 44000,
								kurumi_redspike, kurumi_spell_bg, BOSS_DEFAULT_GO_POS},
	},

	.boss = {
		.animate_wall		= {{ 8,  9, -1, -1}, AT_Spellcard, "Limit ~ Animate Wall", 30, 45000,
								kurumi_aniwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS},
		.demon_wall			= {{-1, -1, 10, 11}, AT_Spellcard, "Summoning ~ Demon Wall", 30, 50000,
								kurumi_aniwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS},
		.blow_the_walls		= {{12, 13, 14, 15}, AT_Spellcard, "Power Sign ~ Blow the Walls", 30, 52000,
								kurumi_blowwall, kurumi_spell_bg, BOSS_DEFAULT_GO_POS},
		.bloody_danmaku		= {{-1, -1, 16, 17}, AT_Spellcard, "Fear Sign ~ Bloody Danmaku", 30, 55000,
								kurumi_danmaku, kurumi_spell_bg, BOSS_DEFAULT_GO_POS},
	},

	.extra.vlads_army		= {{ 0,  1,  2,  3}, AT_ExtraSpell, "Blood Magic ~ Vlad’s Army", 60, 50000,
								kurumi_extra, kurumi_spell_bg, BOSS_DEFAULT_GO_POS},
};

static Stage3D bgcontext;

void stage4_fog(FBO *fbo) {
	Shader *shader = get_shader("zbuf_fog");

	float f = 0;
	if(global.timer > 5100) {
		float v = (global.timer-5100)*0.0005;
		f =  v < 0.1 ? v : 0.1;
	}

	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "depth"),2);
	glUniform4f(uniloc(shader, "fog_color"),10*f,0,0.1-f,1.0);
	glUniform1f(uniloc(shader, "start"),0.4);
	glUniform1f(uniloc(shader, "end"),0.8);
	glUniform1f(uniloc(shader, "exponent"),4.0);
	glUniform1f(uniloc(shader, "sphereness"),0);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, fbo->depth);
	glActiveTexture(GL_TEXTURE0);

	draw_fbo_viewport(fbo);
	glUseProgram(0);
}

Vector **stage4_fountain_pos(Vector pos, float maxrange) {
	Vector p = {0, 400, 1500};
	Vector r = {0, 0, 3000};

	Vector **list = linear3dpos(pos, maxrange, p, r);

	int i;

	for(i = 0; list[i] != NULL; i++) {
		if((*list[i])[2] > 0)
			(*list[i])[2] = -9000;
	}

	return list;
}

void stage4_fountain_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage2/border")->gltex);

	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
	glRotatef(-90, 1,0,0);
	glScalef(1000,3010,1);

	draw_quad();

	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
}

Vector **stage4_lake_pos(Vector pos, float maxrange) {
	Vector p = {0, 600, 0};
	Vector d;

	int i;

	for(i = 0; i < 3; i++)
		d[i] = p[i] - pos[i];

	if(length(d) > maxrange) {
		return NULL;
	} else {
		Vector **list = calloc(2, sizeof(Vector*));

		list[0] = malloc(sizeof(Vector));
		for(i = 0; i < 3; i++)
			(*list[0])[i] = p[i];
		list[1] = NULL;

		return list;
	}
}

void stage4_lake_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage4/lake")->gltex);

	glPushMatrix();
	glTranslatef(pos[0], pos[1]+140, pos[2]);
	glScalef(15,15,15);

	draw_model("lake");
	glPopMatrix();

	glPushMatrix();
	glTranslatef(pos[0], pos[1]+944, pos[2]+50);
	glScalef(30,30,30);

	glBindTexture(GL_TEXTURE_2D, get_tex("stage4/mansion")->gltex);

	draw_model("mansion");
	glPopMatrix();


	glDisable(GL_TEXTURE_2D);
}

Vector **stage4_corridor_pos(Vector pos, float maxrange) {
	Vector p = {0, 2400, 50};
	Vector r = {0, 2000, 0};

	Vector **list = linear3dpos(pos, maxrange, p, r);

	int i;

	for(i = 0; list[i] != NULL; i++) {
		if((*list[i])[1] < p[1])
			(*list[i])[1] = -9000;
	}

	return list;
}

void stage4_corridor_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, get_tex("stage4/planks")->gltex);

	glMatrixMode(GL_TEXTURE);
	glScalef(1,2,1);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);

	glPushMatrix();
	glRotatef(180, 1,0,0);
	glScalef(300,2000,1);

	draw_quad();
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, get_tex("stage4/wall")->gltex);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glRotatef(90,0,0,1);
	glScalef(1,10,1);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glTranslatef(100,5,75);
	glRotatef(90, 0,1,0);
	glScalef(150,2000,1);
	draw_quad();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-100,5,75);
	glRotatef(180,1,0,0);
	glRotatef(-90, 0,1,0);
	glScalef(150,2000,1);
	draw_quad();
	glPopMatrix();

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_TEXTURE_2D);

	glColor3f(0.01,0.01,0.01);
	glPushMatrix();
	glTranslatef(0,0,150);
	glScalef(500,2000,1);
	draw_quad();
	glPopMatrix();

	glPopMatrix();
	glColor3f(1,1,1);
}

void stage4_start(void) {
	init_stage3d(&bgcontext);

	bgcontext.cx[2] = -10000;
	bgcontext.cv[2] = 19.7;
	bgcontext.crot[0] = 80;

	// for testing
// 	bgcontext.cx[1] = 2000;
// 	bgcontext.cx[2] = 130;
// 	bgcontext.cv[1] = 10;
// 	bgcontext.crot[0] = 80;

	add_model(&bgcontext, stage4_lake_draw, stage4_lake_pos);
	add_model(&bgcontext, stage4_fountain_draw, stage4_fountain_pos);
	add_model(&bgcontext, stage4_corridor_draw, stage4_corridor_pos);
}

void stage4_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "bgm_stage4", "bgm_stage4boss", NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage2/border", // Stage 2 is intentional!
		"stage4/kurumibg1",
		"stage4/kurumibg2",
		"stage4/lake",
		"stage4/mansion",
		"stage4/planks",
		"stage4/wall",
		"dialog/kurumi",
	NULL);
	preload_resources(RES_SHADER, RESF_DEFAULT,
		"zbuf_fog",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"kurumi",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"mansion",
		"lake",
	NULL);
}

void stage4_end(void) {
	free_stage3d(&bgcontext);
}

void stage4_draw(void) {
	set_perspective(&bgcontext, 130, 3000);

	draw_stage3d(&bgcontext, 4000);

	if(bgcontext.cx[2] >= -1000 && bgcontext.cv[2] > 0)
		bgcontext.cv[2] -= 0.17;

	if(bgcontext.cx[1] < 100 && bgcontext.cv[2] < 0)
		bgcontext.cv[2] = 0;

	if(bgcontext.cx[2] >= 0 && bgcontext.cx[2] <= 10)
		bgcontext.cv[1] += 0.2;

	if(bgcontext.cx[1] >= 1200 && bgcontext.cx[1] <= 2000)
		bgcontext.cv[1] += 0.02;
}

void stage4_spellpractice_events(void) {
	TIMER(&global.timer);

	AT(0) {
		skip_background_anim(&bgcontext, stage4_draw, 3200, &global.frames, NULL);
		global.boss = create_boss("Kurumi", "kurumi", "dialog/kurumi", BOSS_DEFAULT_SPAWN_POS);
		boss_add_attack_from_info(global.boss, global.stage->spell, true);
		start_attack(global.boss, global.boss->attacks);

		start_bgm("bgm_stage4boss");
	}
}

ShaderRule stage4_shaders[] = { stage4_fog, NULL };

StageProcs stage4_procs = {
	.begin = stage4_start,
	.preload = stage4_preload,
	.end = stage4_end,
	.draw = stage4_draw,
	.event = stage4_events,
	.shader_rules = stage4_shaders,
	.spellpractice_procs = &stage4_spell_procs,
};

StageProcs stage4_spell_procs = {
	.preload = stage4_preload,
	.begin = stage4_start,
	.end = stage4_end,
	.draw = stage4_draw,
	.event = stage4_spellpractice_events,
	.shader_rules = stage4_shaders,
};
