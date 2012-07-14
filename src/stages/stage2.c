/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage2.h"
#include "global.h"
#include "stage.h"
#include "stageutils.h"
#include "stage2_events.h"

static Stage3D bgcontext;

void stage2_start() {
}

void stage2_end() {
}

void stage2_draw() {
}

void stage2_loop() {
// 	ShaderRule shaderrules[] = { stage1_fog, stage1_bloom, NULL };
	stage_loop(stage2_start, stage2_end, stage2_draw, stage2_events, NULL, 5500);
}