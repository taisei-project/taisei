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
#include "extra.h"

void stage6_update(void) {
    stage3d_update(&stage_3d_context);

    if(fall_over) {
        int t = global.frames - fall_over;
        TIMER(&t);

        FROM_TO(0, 240, 1) {
            stage_3d_context.cx[0] += 0.02*cos(M_PI/180*stage_3d_context.crot[2]+M_PI/2)*_i;
            stage_3d_context.cx[1] += 0.02*sin(M_PI/180*stage_3d_context.crot[2]+M_PI/2)*_i;
        }

        FROM_TO(150, 1000, 1) {
            stage_3d_context.crot[0] -= 0.02*(global.frames-fall_over-150);
            if(stage_3d_context.crot[0] < 0)
                stage_3d_context.crot[0] = 0;
        }

        if(t >= 190)
            stage_3d_context.cx[2] -= fmax(6, 0.05*(global.frames-fall_over-150));

        FROM_TO(300, 470,1) {
            stage_3d_context.cx[0] -= 0.01*cos(M_PI/180*stage_3d_context.crot[2]+M_PI/2)*_i;
            stage_3d_context.cx[1] -= 0.01*sin(M_PI/180*stage_3d_context.crot[2]+M_PI/2)*_i;
        }

    }

    float w = 0.002;
    float f = 1, g = 1;
    if(global.timer > 3273) {
        f = fmax(0, f-0.01*(global.timer-3273));

    }

    if(global.timer > 3628)
        g = fmax(0, g-0.01*(global.timer - 3628));

    stage_3d_context.cx[0] += -230*w*f*sin(w*global.frames-M_PI/2);
    stage_3d_context.cx[1] += 230*w*f*cos(w*global.frames-M_PI/2);
    stage_3d_context.cx[2] += w*f*140/M_PI;

    stage_3d_context.crot[2] += 180/M_PI*g*w;
}
