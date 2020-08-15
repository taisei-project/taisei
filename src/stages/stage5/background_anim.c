/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "background_anim.h"
#include "draw.h"

#include "global.h"
#include "stageutils.h"

struct {
    float light_strength;

    float rotshift;
    float omega;
    float rad;
} stagedata;

// background_anim
static void stage5_update(void) {
    stage3d_update(&stage_3d_context);

    TIMER(&global.timer);
    float w = 0.005;

    stagedata.rotshift += stagedata.omega;
    stage_3d_context.crot[0] += stagedata.omega*0.5;
    stagedata.rad += stagedata.omega*20;

    int rot_time = 6350;

    FROM_TO(rot_time, rot_time+50, 1)
        stagedata.omega -= 0.005;

    FROM_TO(rot_time+200, rot_time+250, 1)
        stagedata.omega += 0.005;

    stage_3d_context.cx[0] = stagedata.rad*cos(-w*global.frames);
    stage_3d_context.cx[1] = stagedata.rad*sin(-w*global.frames);
    stage_3d_context.cx[2] = -1700+w*3000/M_PI*global.frames;

    stage_3d_context.crot[2] = stagedata.rotshift-180/M_PI*w*global.frames;

    stagedata.light_strength *= 0.98;

    if(frand() < 0.01)
        stagedata.light_strength = 5+5*frand();
}
