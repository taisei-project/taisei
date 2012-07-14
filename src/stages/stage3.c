/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage3.h"
#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stage3_events.h"

static Stage3D bgcontext;

void stage3_start() {
	init_stage3d(&bgcontext);
}

void stage3_end() {
	free_stage3d(&bgcontext);
}

void stage3_draw() {
	set_perspective(&bgcontext, 500, 5000);
	
	draw_stage3d(&bgcontext, 7000);
}

void stage3_loop() {
// 	ShaderRule shaderrules[] = { stage1_fog, stage1_bloom, NULL };
	stage_loop(stage_get(4), stage3_start, stage3_end, stage3_draw, stage3_events, NULL, 5500);
}
