/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4.h"
#include "stage4_events.h"

#include "stage.h"
#include "stageutils.h"

#include "global.h"

static Stage3D bgcontext;

void stage4_draw() {
	draw_stage3d(&bgcontext, 7000);
}

void stage4_start() {
	init_stage3d(&bgcontext);
}

void stage4_end() {
	free_stage3d(&bgcontext);
}

void stage4_loop() {
// 	ShaderRule shaderrules[] = { stage3_fog, NULL };
	stage_loop(stage_get(4), stage4_start, stage4_end, stage4_draw, stage4_events, NULL, 5550);
}