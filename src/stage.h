/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef STAGE_H
#define STAGE_H

#define TIMER(ptr) int *__timep = ptr; int _i;
#define AT(t) if(*__timep == t)
#define FROM_TO(start,end,step) _i = (*__timep - start)/step; if(*__timep >= (start) && *__timep <= (end) && !(*__timep % (step)))


typedef void (*StageRule)(void);

void stage_loop(StageRule start, StageRule end, StageRule draw, StageRule event);

void stage_start();

void stage_logic();
void stage_draw();
void stage_input();

void stage_end();

void apply_bg_shaders();
#endif