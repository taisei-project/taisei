/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include "texture.h"

typedef struct {
	int rows;
	int cols;
	
	int w,h;
	
	int speed;
	
	Texture tex;
} Animation;

void init_animation(Animation *buf, int rows, int cols, int speed, const char *filename);
void draw_animation(int x, int y, int row, const Animation *ani);

#endif