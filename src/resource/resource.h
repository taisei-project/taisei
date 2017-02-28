/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include "global.h"

#include "texture.h"
#include "animation.h"
#include "audio.h"
#include "bgm.h"
#include "shader.h"
#include "font.h"
#include "model.h"
#include "hashtable.h"

typedef struct Resources Resources;

#define RES_HASHTABLE_SIZE 64

typedef enum ResourceState {
	RS_GfxLoaded = 1,
	RS_SfxLoaded = 2,
	RS_ShaderLoaded = 4,
	RS_ModelsLoaded = 8,
	RS_BgmLoaded = 16,
	RS_FontsLoaded = 32,
} ResourceState;

struct Resources {
	ResourceState state;

	Hashtable *textures;
	Hashtable *animations;
	Hashtable *sounds;
	Hashtable *music;
	Hashtable *shaders;
	Hashtable *models;
	Hashtable *bgm_descriptions;

	FBO fbg[2];
	FBO fsec;
};

extern Resources resources;

void init_resources(void);
void load_resources(void);
void free_resources(void);

void draw_loading_screen(void);

void resources_delete_and_unset_all(Hashtable *ht, HTIterCallback ifunc, void *arg);

#endif
