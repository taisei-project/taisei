/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdbool.h>
#include "global.h"

#include "texture.h"
#include "animation.h"
#include "audio.h"
#include "bgm.h"
#include "shader.h"
#include "font.h"
#include "model.h"

typedef struct Resources Resources;

struct Resources {
	Texture *textures;
	Animation *animations;
	Sound *sounds;
	Sound *music;
	Shader *shaders;
	Model *models;
	Bgm_desc *bgm_descriptions;

	FBO fbg[2];
	FBO fsec;
};

extern Resources resources;

void load_resources(bool transient);
void free_resources(bool transient);

void draw_loading_screen(void);
#endif
