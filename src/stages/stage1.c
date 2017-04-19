/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage1.h"

#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stage1_events.h"

static Stage3D bgcontext;

/*
 *	See the definition of AttackInfo in boss.h for information on how to set up the idmaps.
 */

AttackInfo stage1_spells[] = {
	{{ 0,  1,  2,  3},	AT_Spellcard, "Freeze Sign ~ Perfect Freeze", 32, 20000,
							cirno_perfect_freeze, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I},
	{{ 4,  5,  6,  7},	AT_Spellcard, "Freeze Sign ~ Crystal Rain", 28, 33000,
							cirno_crystal_rain, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I},
	{{ 8,  9, 10, 11},	AT_Spellcard, "Doom Sign ~ Icicle Fall", 35, 40000,
							cirno_icicle_fall, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I},
	{{ 0,  1,  2,  3},	AT_ExtraSpell, "Frost Sign ~ Crystal Blizzard", 60, 40000,
							cirno_crystal_blizzard, cirno_pfreeze_bg, VIEWPORT_W/2.0+100.0*I},

	{{0}}
};

void stage1_bg_draw(Vector pos) {
	glPushMatrix();
	glTranslatef(0,bgcontext.cx[1]+500,0);
	glRotatef(180,1,0,0);


	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, get_tex("stage1/water")->gltex);

	glPushMatrix();
	glScalef(1200,3000,1);
	glColor4f(0,0.1,.1,1);
	draw_quad();
	glColor4f(1,1,1,1);
	glPopMatrix();

	glPushMatrix();
	glRotatef(30,1,0,0);
	glScalef(.85,-.85,.85);
	glTranslatef(-VIEWPORT_W/2,0,0);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	for(Projectile *p = global.particles; p; p = p->next) {
		if(p->type != PlrProj)
			p->draw(p,global.frames - p->birthtime);
	}
	draw_enemies(global.enemies);
	if(global.boss)
		draw_boss(global.boss);
	glPopMatrix();
	glEnable(GL_CULL_FACE);

	glPushMatrix();
	glScalef(1200,3000,1);
	glColor4f(0,0.1,.1,0.8);
	draw_quad();
	glColor4f(1,1,1,1);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
}

Vector **stage1_bg_pos(Vector p, float maxrange) {
	Vector q = {0,0,0};
	return single3dpos(p, INFINITY, q);
}

void stage1_smoke_draw(Vector pos) {
	float d = fabsf(pos[1]-bgcontext.cx[1]);

	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glTranslatef(pos[0]+200*sin(pos[1]), pos[1], pos[2]+200*sin(pos[1]/25.0));
	glRotatef(90,-1,0,0);
	glScalef(3.5,2,1);
	glRotatef(global.frames,0,0,1);

	glColor4f(.8,.8,.8,((d-500)*(d-500))/1.5e7);
	draw_texture(0,0,"stage1/fog");
	glColor4f(1,1,1,1);

	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

Vector **stage1_smoke_pos(Vector p, float maxrange) {
	Vector q = {0,0,-300};
	Vector r = {0,300,0};
	return linear3dpos(p, maxrange/2.0, q, r);
}

void stage1_fog(FBO *fbo) {
	Shader *shader = get_shader("zbuf_fog");

	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "tex"), 0);
	glUniform1i(uniloc(shader, "depth"), 1);
	glUniform4f(uniloc(shader, "fog_color"), 0.8, 0.8, 0.8, 1.0);
	glUniform1f(uniloc(shader, "start"), 0.0);
	glUniform1f(uniloc(shader, "end"), 0.8);
	glUniform1f(uniloc(shader, "exponent"), 3.0);
	glUniform1f(uniloc(shader, "sphereness"), 0.2);
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, fbo->depth);
	glActiveTexture(GL_TEXTURE0);

	draw_fbo_viewport(fbo);
	glUseProgram(0);
}

void stage1_draw(void) {
	set_perspective(&bgcontext, 500, 5000);

	draw_stage3d(&bgcontext, 7000);
}

void stage1_reed_draw(Vector pos) {
	float d = -55+50*sin(pos[1]/25.0);
	glPushMatrix();
	glTranslatef(pos[0]+200*sin(pos[1]), pos[1], d);
	glRotatef(90,1,0,0);
//glRotatef(90,0,0,1);
	glScalef(80,80,80);
	glColor4f(0.,0.05,0.05,1);

	draw_model("reeds");
	glTranslatef(0,-d/80,0);
	glScalef(1,-1,1);
	glTranslatef(0,d/80,0);
	glDepthFunc(GL_GREATER);
	glDepthMask(GL_FALSE);
	glColor4f(0.,0.05,0.05,0.5);
	draw_model("reeds");
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glColor4f(1,1,1,1);
	glPopMatrix();
}

void stage1_start(void) {
	init_stage3d(&bgcontext);
	add_model(&bgcontext, stage1_bg_draw, stage1_bg_pos);
	add_model(&bgcontext, stage1_smoke_draw, stage1_smoke_pos);
	add_model(&bgcontext, stage1_reed_draw, stage1_smoke_pos);

	bgcontext.crot[0] = 60;
	bgcontext.cx[2] = 700;
	bgcontext.cv[1] = 4;
}

void stage1_preload(void) {
	preload_resources(RES_BGM, RESF_OPTIONAL, "bgm_stage1", "bgm_stage1boss", NULL);
	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"stage1/cirnobg",
		"stage1/fog",
		"stage1/snowlayer",
		"dialog/cirno",
	NULL);
	preload_resources(RES_MODEL, RESF_DEFAULT,
		"reeds",
	NULL);
	preload_resources(RES_SHADER, RESF_DEFAULT,
		"zbuf_fog",
	NULL);
	preload_resources(RES_ANIM, RESF_DEFAULT,
		"cirno",
	NULL);
}

void stage1_end(void) {
	free_stage3d(&bgcontext);
}

void stage1_spellpractice_events(void) {
	TIMER(&global.timer);

	AT(0) {
		Boss* cirno = create_boss("Cirno", "cirno", "dialog/cirno", BOSS_DEFAULT_SPAWN_POS);
		boss_add_attack_from_info(cirno, global.stage->spell, true);
		start_attack(cirno, cirno->attacks);
		global.boss = cirno;

		start_bgm("bgm_stage1boss");
	}
}

ShaderRule stage1_shaders[] = { stage1_fog, NULL };

StageProcs stage1_procs = {
	.begin = stage1_start,
	.preload = stage1_preload,
	.end = stage1_end,
	.draw = stage1_draw,
	.event = stage1_events,
	.shader_rules = stage1_shaders,
	.spellpractice_procs = &stage1_spell_procs,
};

StageProcs stage1_spell_procs = {
	.preload = stage1_preload,
	.begin = stage1_start,
	.end = stage1_end,
	.draw = stage1_draw,
	.event = stage1_spellpractice_events,
	.shader_rules = stage1_shaders,
};
