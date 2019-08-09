/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_video_postprocess_h
#define IGUARD_video_postprocess_h

#include "taisei.h"

#include "renderer/api.h"

typedef struct VideoPostProcess VideoPostProcess;

VideoPostProcess *video_postprocess_init(void);
void video_postprocess_shutdown(VideoPostProcess *vpp);
Framebuffer *video_postprocess_get_framebuffer(VideoPostProcess *vpp);
Framebuffer *video_postprocess_render(VideoPostProcess *vpp);

#endif // IGUARD_video_postprocess_h
