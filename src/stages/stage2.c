/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage2.h"
#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stage2_events.h"

static Stage3D bgcontext;

void stage2_bg_leaves_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);
	if(!config_get_int(CONFIG_NO_SHADER))
		glUseProgram(get_shader("alpha_depth")->prog);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(-1,1,1);
	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	Texture *leaves = get_tex("stage2/leaves");
	glBindTexture(GL_TEXTURE_2D, leaves->gltex);

	glTranslatef(pos[0]-360,pos[1],pos[2]+500);
	glRotatef(-160,0,1,0);
	glTranslatef(-150,0,0);
	glScalef(1000,3000,1);

	draw_quad();

	glPopMatrix();

	if(!config_get_int(CONFIG_NO_SHADER))
		glUseProgram(0);

	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}
void stage2_bg_ground_draw(Vector pos) {
	glEnable(GL_TEXTURE_2D);

	glPushMatrix();
	glTranslatef(pos[0]+60,pos[1],pos[2]);
	glScalef(1500,3000,1000);

	Texture *road = get_tex("stage2/road");

	glBindTexture(GL_TEXTURE_2D, road->gltex);

	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glScalef(1,2,1);
	glMatrixMode(GL_MODELVIEW);

	glRotatef(-180,1,0,0);

	draw_quad();

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glTranslatef(global.frames/100.0,1*sin(global.frames/100.0),0);
	glMatrixMode(GL_MODELVIEW);

	glPopMatrix();

	glPushMatrix();

	Texture *border = get_tex("stage2/border");
	glBindTexture(GL_TEXTURE_2D, border->gltex);

	glTranslatef(pos[0]+410,pos[1],pos[2]+600);
	glRotatef(90,0,1,0);
	glScalef(1200,3000,1);

	draw_quad();

	glPopMatrix();

	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

Vector **stage2_bg_pos(Vector pos, float maxrange) {
	Vector p = {0, 0, 0};
	Vector r = {0, 3000, 0};

	return linear3dpos(pos, maxrange, p, r);
}

void stage2_fog(int fbonum) {
	Shader *shader = get_shader("zbuf_fog");

	glUseProgram(shader->prog);
	glUniform1i(uniloc(shader, "depth"),2);
	glUniform4f(uniloc(shader, "fog_color"),0.05,0.0,0.03,1.0);
	glUniform1f(uniloc(shader, "start"),0.2);
	glUniform1f(uniloc(shader, "end"),0.8);
	glUniform1f(uniloc(shader, "exponent"),3.0);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, resources.fbg[fbonum].depth);
	glActiveTexture(GL_TEXTURE0);

	draw_fbo_viewport(&resources.fbg[fbonum]);
	glUseProgram(0);
}

void stage2_bloom(int fbonum) {
	Shader *shader = get_shader("bloom");

	glUseProgram(shader->prog);
	glUniform1f(uniloc(shader, "intensity"),0.05);
	draw_fbo_viewport(&resources.fbg[fbonum]);
	glUseProgram(0);
}

void stage2_start(void) {
	init_stage3d(&bgcontext);
	bgcontext.cx[2] = 1000;
	bgcontext.cx[0] = -850;
	bgcontext.crot[0] = 60;
	bgcontext.crot[2] = -90;

	bgcontext.cv[0] = 9;

	add_model(&bgcontext, stage2_bg_ground_draw, stage2_bg_pos);
	add_model(&bgcontext, stage2_bg_leaves_draw, stage2_bg_pos);
}

void stage2_end(void) {
	free_stage3d(&bgcontext);
}

void stage2_draw(void) {
	TIMER(&global.frames);

	set_perspective(&bgcontext, 500, 5000);

	FROM_TO(0,180,1) {
		bgcontext.cv[0] -= 0.05;
		bgcontext.cv[1] += 0.1;
		bgcontext.crot[2] += 0.5;
	}

	draw_stage3d(&bgcontext, 7000);

}

void stage2_spellpractice_events(void) {
	TIMER(&global.timer);

	AT(0) {
		skip_background_anim(&bgcontext, stage2_draw, 180, &global.frames, NULL);

		Boss* hina = create_boss("Kagiyama Hina", "hina", BOSS_DEFAULT_SPAWN_POS);
		boss_add_attack_from_info(hina, global.stage->spell, true);
		start_attack(hina, hina->attacks);
		global.boss = hina;

		start_bgm("bgm_stage2boss");
	}
}

ShaderRule stage2_shaders[] = { stage2_fog, stage2_bloom, NULL };

StageProcs stage2_procs = {
	.begin = stage2_start,
	.end = stage2_end,
	.draw = stage2_draw,
	.event = stage2_events,
	.shader_rules = stage2_shaders,
	.spellpractice_procs = &stage2_spell_procs,
};

StageProcs stage2_spell_procs = {
	.begin = stage2_start,
	.end = stage2_end,
	.draw = stage2_draw,
	.event = stage2_spellpractice_events,
	.shader_rules = stage2_shaders,
};
