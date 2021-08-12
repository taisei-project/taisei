/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "plrmodes.h"
#include "youmu_a.h"
#include "youmu_b.h"
#include "dialog/youmu.h"

extern PlayerCharacter character_youmu;

typedef struct YoumuBombBGData {
	Framebuffer *buffer;
	ShaderProgram *shader;
	struct {
		Uniform *petals, *time;
	} uniforms;
	Texture *texture;
} YoumuBombBGData;

double youmu_common_property(Player *plr, PlrProperty prop);
Projectile *youmu_common_shot(cmplx pos, MoveParams move, real dmg, ShaderProgram *shader);

void youmu_common_init_bomb_background(YoumuBombBGData *bg_data);
DECLARE_EXTERN_TASK(youmu_common_bomb_background, { BoxedPlayer plr; YoumuBombBGData *bg_data; });
