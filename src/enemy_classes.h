/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "enemy.h"
#include "item.h"
#include "stageutils.h"

typedef Enemy *(*EnemySpawner)(cmplx pos, const ItemCounts *item_drops);

Enemy *espawn_swirl(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_fairy_blue(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_fairy_red(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_big_fairy(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_huge_fairy(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_super_fairy(cmplx pos, const ItemCounts *item_drops);

#ifdef ENEMY_DEBUG
	#define WRAP_ENEMY_SPAWNER(spawner, ...) \
		_enemy_attach_dbginfo((spawner)(__VA_ARGS__), _DEBUG_INFO_PTR_)

	#define espawn_swirl(...)           WRAP_ENEMY_SPAWNER(espawn_swirl, __VA_ARGS__)
	#define espawn_fairy_blue(...)      WRAP_ENEMY_SPAWNER(espawn_fairy_blue, __VA_ARGS__)
	#define espawn_fairy_red(...)       WRAP_ENEMY_SPAWNER(espawn_fairy_red, __VA_ARGS__)
	#define espawn_big_fairy(...)       WRAP_ENEMY_SPAWNER(espawn_big_fairy, __VA_ARGS__)
	#define espawn_huge_fairy(...)      WRAP_ENEMY_SPAWNER(espawn_huge_fairy, __VA_ARGS__)
	#define espawn_super_fairy(...)     WRAP_ENEMY_SPAWNER(espawn_super_fairy, __VA_ARGS__)
#endif

#define espawn_swirl_box(...)           ENT_BOX(espawn_swirl(__VA_ARGS__))
#define espawn_fairy_blue_box(...)      ENT_BOX(espawn_fairy_blue(__VA_ARGS__))
#define espawn_fairy_red_box(...)       ENT_BOX(espawn_fairy_red(__VA_ARGS__))
#define espawn_big_fairy_box(...)       ENT_BOX(espawn_big_fairy(__VA_ARGS__))
#define espawn_huge_fairy_box(...)      ENT_BOX(espawn_huge_fairy(__VA_ARGS__))
#define espawn_super_fairy_box(...)     ENT_BOX(espawn_super_fairy(__VA_ARGS__))

SpriteParams ecls_anyfairy_sprite_params(
	Enemy *fairy,
	EnemyDrawParams draw_params,
	SpriteParamsBuffer *out_spbuf
);

float ecls_anyenemy_set_scale(Enemy *e, float s);
float ecls_anyenemy_get_scale(Enemy *e);
float ecls_anyenemy_set_opacity(Enemy *e, float o);
float ecls_anyenemy_get_opacity(Enemy *e);

void ecls_anyenemy_fake3dmovein(
	Enemy *e,
	Camera3D *cam,
	vec3 initpos_3d,
	int duration
);

void ecls_anyfairy_summon(Enemy *e, int duration);
