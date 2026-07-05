/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "enemy.h"
#include "item.h"
#include "resource/animation.h"
#include "resource/sprite.h"
#include "stageutils.h"

typedef struct BaseEnemyVisual {
	union {
		Sprite *spr;
		Animation *ani;
	};
	float scale;
	float opacity;
	struct {
		cmplxf pos;
		float blendfactor;
	} fakepos;
} BaseEnemyVisual;

typedef struct Enemy3DMoveInParams {
	Camera3D *cam;
	vec3 init_pos_3d;
	int duration;
} Enemy3DMoveInParams;

typedef struct FairyVisual {
	BaseEnemyVisual base;
	BoxedProjectile circle;
	ShaderProgram *shader;
	Texture *noise_texture;
	struct {
		float progress;
		float cloak;
		FloatBits mask_ofs_bits;
	} summon;
} FairyVisual;

typedef struct FairyHandle {
	BoxedEnemy entity;
	FairyVisual *visual;
} FairyHandle;

FairyHandle ecls_spawn_fairy_blue(cmplx pos, const ItemCounts *item_drops);
FairyHandle ecls_spawn_fairy_red(cmplx pos, const ItemCounts *item_drops);
FairyHandle ecls_spawn_big_fairy(cmplx pos, const ItemCounts *item_drops);
FairyHandle ecls_spawn_huge_fairy(cmplx pos, const ItemCounts *item_drops);
FairyHandle ecls_spawn_super_fairy(cmplx pos, const ItemCounts *item_drops);

FairyHandle ecls_fairy_summon(FairyHandle fairy, int duration);
FairyHandle ecls_fairy_3d_move_in(FairyHandle fairy, Camera3D *cam, vec3 init_pos_3d, int duration);

typedef struct SwirlVisual {
	BaseEnemyVisual base;
	ShaderProgram *shader;
} SwirlVisual;

typedef struct SwirlHandle {
	BoxedEnemy entity;
	SwirlVisual *visual;
} SwirlHandle;

SwirlHandle ecls_spawn_swirl(cmplx pos, const ItemCounts *item_drops);

SwirlHandle ecls_swirl_3d_move_in(SwirlHandle swirl, Camera3D *cam, vec3 init_pos_3d, int duration);

/*
 * Old-style spawner wrappers
 */

typedef Enemy *(*EnemySpawner)(cmplx pos, const ItemCounts *item_drops);

Enemy *espawn_swirl(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_fairy_blue(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_fairy_red(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_big_fairy(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_huge_fairy(cmplx pos, const ItemCounts *item_drops);
Enemy *espawn_super_fairy(cmplx pos, const ItemCounts *item_drops);

#define espawn_swirl_box(...)           ENT_BOX(espawn_swirl(__VA_ARGS__))
#define espawn_fairy_blue_box(...)      ENT_BOX(espawn_fairy_blue(__VA_ARGS__))
#define espawn_fairy_red_box(...)       ENT_BOX(espawn_fairy_red(__VA_ARGS__))
#define espawn_big_fairy_box(...)       ENT_BOX(espawn_big_fairy(__VA_ARGS__))
#define espawn_huge_fairy_box(...)      ENT_BOX(espawn_huge_fairy(__VA_ARGS__))
#define espawn_super_fairy_box(...)     ENT_BOX(espawn_super_fairy(__VA_ARGS__))
