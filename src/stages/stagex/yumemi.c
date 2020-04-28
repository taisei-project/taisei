/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "yumemi.h"
#include "draw.h"
#include "stagex.h"

#include "renderer/api.h"
#include "util/glm.h"
#include "global.h"

static void draw_yumemi_slave(EntityInterface *ent) {
	YumemiSlave *slave = ENT_CAST(ent, YumemiSlave);

	float time = global.frames - slave->spawn_time;
	float s = slave->rotation_factor;
	float opacity;
	float scale;

	if(stagex_drawing_into_glitchmask()) {
		opacity = lerpf(80.0f, 2.0f, fminf(time / 80.0f, 1.0));
		scale = 1.1f;
	} else {
		opacity = slave->alpha;
		scale = 1.0f + 6.0f * glm_ease_elast_in(1.0f - slave->alpha);
	}

	scale *= 0.5f;

	SpriteParams sp = {
		.pos.as_cmplx = slave->pos,
		.scale = scale,
		.color = color_mul_scalar(RGBA(1, 1, 1, 0.5), opacity),
	};

	sp.sprite_ptr = slave->sprites.frame;
	sp.rotation.angle = -M_PI/73 * time * s;
	r_draw_sprite(&sp);

	sp.sprite_ptr = slave->sprites.outer;
	sp.rotation.angle = +M_PI/73 * time * s;
	r_draw_sprite(&sp);

	sp.sprite_ptr = slave->sprites.core;
	sp.rotation.angle = 0;
	sp.color = color_mul_scalar(RGBA(0.4, 0.4, 0.4, 0.1), opacity);
	sp.pos.as_cmplx += 3 * cdir(M_PI/72 * time * s);
	r_draw_sprite(&sp);
}

Boss *stagex_spawn_yumemi(cmplx pos) {
	Boss *yumemi = create_boss("Okazaki Yumemi", "yumemi", pos - 400 * I);
	boss_set_portrait(yumemi, "yumemi", NULL, "normal");
	yumemi->shadowcolor = *RGBA(0.5, 0.0, 0.22, 1);
	yumemi->glowcolor = *RGBA(0.30, 0.0, 0.12, 0);
	yumemi->move = move_towards(pos, 0.01);

	return yumemi;
}

TASK(yumemi_slave_fadein, { BoxedYumemiSlave slave; }) {
	YumemiSlave *slave = TASK_BIND(ARGS.slave);

	while(fapproach_p(&slave->alpha, 1.0f, 1.0f / 60.0f) < 1) {
		YIELD;
	}
}

void stagex_init_yumemi_slave(YumemiSlave *slave, cmplx pos, int type) {
	slave->pos = pos;
	slave->ent.draw_func = draw_yumemi_slave;
	slave->ent.draw_layer = LAYER_BOSS - 1;
	slave->spawn_time = global.frames;

	switch(type) {
		case 0:
			slave->sprites.core = res_sprite("stagex/yumemi_slaves/zero_core");
			slave->sprites.frame = res_sprite("stagex/yumemi_slaves/zero_frame");
			slave->sprites.outer = res_sprite("stagex/yumemi_slaves/zero_outer");
			slave->rotation_factor = 1;
			break;

		case 1:
			slave->sprites.core = res_sprite("stagex/yumemi_slaves/one_core");
			slave->sprites.frame = res_sprite("stagex/yumemi_slaves/one_frame");
			slave->sprites.outer = res_sprite("stagex/yumemi_slaves/one_outer");
			slave->rotation_factor = -1;
			break;

		default: UNREACHABLE;
	}

}

YumemiSlave *stagex_host_yumemi_slave(cmplx pos, int type) {
	YumemiSlave *slave = TASK_HOST_ENT(YumemiSlave);
	stagex_init_yumemi_slave(slave, pos, type);
	INVOKE_TASK(yumemi_slave_fadein, ENT_BOX(slave));
	return slave;
}

void stagex_draw_yumemi_portrait_overlay(SpriteParams *sp) {
	StageXDrawData *draw_data = stagex_get_draw_data();

	sp->sprite = NULL;
	sp->sprite_ptr = res_sprite("dialog/yumemi_misc_code_mask");
	sp->shader = NULL;
	sp->shader_ptr = res_shader("sprite_yumemi_overlay");
	sp->aux_textures[0] = res_texture("stageex/code");
	sp->shader_params = &(ShaderCustomParams) {
		global.frames / 60.0,
		draw_data->codetex_aspect[0],
		draw_data->codetex_aspect[1],
		draw_data->codetex_num_segments,
	};

	r_draw_sprite(sp);
}
