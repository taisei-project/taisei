/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "hirestime.h"

typedef struct {
    hrtime_t frametimes[120]; // size = number of frames to average
    double fps; // average fps over the last X frames
    hrtime_t frametime; // average frame time over the last X frames;
    hrtime_t last_update_time; // internal; last time the average was recalculated
} FPSCounter;

typedef enum FrameAction {
    RFRAME_SWAP,
    RFRAME_DROP,

    LFRAME_WAIT,
    LFRAME_SKIP,
    LFRAME_STOP,
} FrameAction;

typedef FrameAction (*LogicFrameFunc)(void*);
typedef FrameAction (*RenderFrameFunc)(void*);

uint32_t get_effective_frameskip(void);
void loop_at_fps(LogicFrameFunc logic_frame, RenderFrameFunc render_frame, void *arg, uint32_t fps);
void fpscounter_reset(FPSCounter *fps);
void fpscounter_update(FPSCounter *fps);
