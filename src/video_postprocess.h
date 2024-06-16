/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "renderer/api.h"

typedef struct VideoPostProcess VideoPostProcess;

VideoPostProcess *video_postprocess_init(void);
void video_postprocess_shutdown(VideoPostProcess *vpp);
Framebuffer *video_postprocess_get_framebuffer(VideoPostProcess *vpp);
Framebuffer *video_postprocess_render(VideoPostProcess *vpp);
