/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef STAGE_H
#define STAGE_H

/* taisei's strange macro language.
 * 
 * sorry, I guess it is bad style, but I hardcode everything and in that case
 * you'll find yourself soon in a situation where you have to spread your
 * coherent thoughts over frames using masses of redundant ifs.
 * I've just invented this thingy to keep track of my sanity.
 * 
 */

#include <projectile.h>

#define TIMER(ptr) int *__timep = ptr; int _i = 0, _ni = 0;  _i = _ni = _i;
#define AT(t) if(*__timep == t) 
#define FROM_TO(start,end,step) _ni = _ni; _i = (*__timep - (start))/(step); if(*__timep >= (start) && *__timep <= (end) && !((*__timep - (start)) % (step)))
#define FROM_TO_INT(start, end, step, dur, istep) \
		_i = (*__timep - (start))/(step+dur); _ni = ((*__timep - (start)) % (step+dur))/istep; \
		if(*__timep >= (start) && *__timep <= (end) && (*__timep - (start)) % ((dur) + (step)) <= dur && !((*__timep - (start)) % (istep)))

#define GO_AT(obj, start, end, vel) if(*__timep >= (start) && *__timep <= (end)) (obj)->pos += (vel);
#define GO_TO(obj, p, f) (obj)->pos += (f)*((p) - (obj)->pos);

typedef void (*StageRule)(void);
typedef void (*ShaderRule)(int);

typedef struct StageInfo {
	int id;
	StageRule loop;
	int hidden;
	char *title;
	char *subtitle;
	
	Color titleclr;
} StageInfo;

extern StageInfo stages[];
StageInfo* stage_get(int);

void stage_loop(StageInfo *info, StageRule start, StageRule end, StageRule draw, StageRule event, ShaderRule *shaderrules, int endtime);

void apply_bg_shaders(ShaderRule *shaderrules);

void draw_stage_title(StageInfo *info);
void draw_hud(void);

void stage1_loop(void);
void stage2_loop(void);
void stage3_loop(void);
void stage4_loop(void);
void stage5_loop(void);
void stage6_loop(void);

void stage_pause(void);
void stage_gameover(void);

#endif
